/***************************************************************************
 *   Copyright (C) 2023 by Stefan Kebekus                                  *
 *   stefan.kebekus@gmail.com                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <QFile>
#include <QGeoRectangle>
#include <QString>
#include <QVariant>
#include <QtEndian>

#include "DataFileAbstract.h"

namespace FileFormats
{

/*! \brief Trip Kit
 *
 *  This class reads GeoTIFF files, as specified here:
 *  https://gis-lab.info/docs/geotiff-1.8.2.pdf
 *
 *  It extracts bounding box coordinates, as well as the name of the file. This
 *  class does not read the raster data. GeoTIFF is a huge and complex standard,
 *  and this class is definitively not able to read all possible valid GeoTIFF
 *  files. We restrict ourselves to files that appear in real-world aviation.
 */

class GeoTIFF : public DataFileAbstract
{
public:
    /*! \brief Constructor
     *
     *  The constructor opens and analyzes the GeoTIFF file. It does not read
     *  the raster data.
     *
     *  \param fileName File name of a GeoTIFF file.
     */
    GeoTIFF(const QString& fileName);



    //
    // Getter Methods
    //

    /*! \brief Name, as specified in the GeoTIFF file
     *
     *  @returns The name or an empty string if no name is specified.
     */
    [[nodiscard]] QString name() const { return m_name; }

    /*! \brief Bounding box, as specified in the GeoTIFF file
     *
     *  @returns Bounding box, which might be invalid
     */
    [[nodiscard]] QGeoRectangle bBox() const { return m_bBox; }

private:
    QGeoRectangle m_bBox;
    QString m_name;

    struct GeoTiffMeta {
        QGeoRectangle rect;
        QString desc;
    };

    class TiffIfdEntry
    {
    public:
        enum DataType {
            DT_Byte = 1,
            DT_Ascii,
            DT_Short,
            DT_Long,
            DT_Rational,
            DT_SByte,
            DT_Undefined,
            DT_SShort,
            DT_SLong,
            DT_SRational,
            DT_Float,
            DT_Double,
            DT_Ifd,
            DT_Long8,
            DT_SLong8,
            DT_Ifd8
        };

    private:
        friend class GeoTIFF;

        [[nodiscard]] auto typeSize() const -> int
        {
            switch (m_type) {
            case TiffIfdEntry::DT_Byte:
            case TiffIfdEntry::DT_SByte:
            case TiffIfdEntry::DT_Ascii:
            case TiffIfdEntry::DT_Undefined:
                return 1;
            case TiffIfdEntry::DT_Short:
            case TiffIfdEntry::DT_SShort:
                return 2;
            case TiffIfdEntry::DT_Long:
            case TiffIfdEntry::DT_SLong:
            case TiffIfdEntry::DT_Ifd:
            case TiffIfdEntry::DT_Float:
                return 4;

            case TiffIfdEntry::DT_Rational:
            case TiffIfdEntry::DT_SRational:
            case TiffIfdEntry::DT_Long8:
            case TiffIfdEntry::DT_SLong8:
            case TiffIfdEntry::DT_Ifd8:
            case TiffIfdEntry::DT_Double:
                return 8;
            default:
                return 0;
            }
        }

        void parserValues(const char *bytes, QDataStream::ByteOrder byteOrder)
        {
            if (m_type == TiffIfdEntry::DT_Ascii)
            {
                int start = 0;
                for (int i = 0; i < m_count; ++i)
                {
                    if (bytes[i] == '\0')
                    {
                        m_values.append(QString::fromLatin1(bytes + start, i - start));
                        start = i + 1;
                    }
                }
                if (bytes[m_count - 1] != '\0')
                {
                    m_values.append(QString::fromLatin1(bytes + start, m_count - start));
                }
                return;
            }
            // To make things simple, save normal integer as qint32 or quint32 here.
            for (qsizetype i = 0; i < m_count; ++i)
            {
                switch (m_type)
                {
                case TiffIfdEntry::DT_Short:
                    m_values.append(static_cast<quint32>(getValueFromBytes<quint16>(bytes + i * 2, byteOrder)));
                    break;
                case TiffIfdEntry::DT_Double:
                    double resultingFloat;
                    if (byteOrder == QDataStream::BigEndian)
                    {
                        std::array<char,8> rbytes;
                        rbytes[0] = bytes[i*8+7];
                        rbytes[1] = bytes[i*8+6];
                        rbytes[2] = bytes[i*8+5];
                        rbytes[3] = bytes[i*8+4];
                        rbytes[4] = bytes[i*8+3];
                        rbytes[5] = bytes[i*8+2];
                        rbytes[6] = bytes[i*8+1];
                        rbytes[7] = bytes[i*8];
                        memcpy( &resultingFloat, rbytes.data(), 8 );
                    }
                    else
                    {
                        memcpy( &resultingFloat, bytes + i * 8, 8 );
                    }
                    m_values.append(resultingFloat);
                    break;
                default:
                    break;
                }
            }
        }

        quint16 m_tag;
        quint16 m_type;
        qsizetype m_count {0};
        QByteArray m_valueOrOffset; // 12 bytes for tiff or 20 bytes for bigTiff
        QVariantList m_values;
    };

    class TiffIfd
    {
        friend class GeoTIFF;

        QVector<TiffIfdEntry> m_ifdEntries;
        qint64 m_nextIfdOffset{ 0 };
    };



    [[nodiscard]] auto getMeta() const -> GeoTiffMeta;

    qint64 readHeader();

    auto readIfd(qint64 offset, TiffIfd *parentIfd = nullptr) -> bool;

    template<typename T> auto getValueFromFile() -> T
    {
        T value {0};
        auto bytesRead = m_file.read(reinterpret_cast<char *>(&value), sizeof(T));
        if (bytesRead != sizeof(T))
        {
            throw QObject::tr("Error reading file.", "FileFormats::GeoTIFF");
        }
        return fixValueByteOrder(value, m_header.byteOrder);
    }

    struct Header
    {
        QDataStream::ByteOrder byteOrder{ QDataStream::LittleEndian };
    } m_header;

    struct Geo
    {
        quint16 width = 0;
        quint16 height = 0;
        double longitute = 0;
        double latitude = 0;
        double pixelWidth = 0;
        double pixelHeight = 0;
        QString desc;
    } m_geo;

    template<typename T> static auto getValueFromBytes(const char *bytes, QDataStream::ByteOrder byteOrder) -> T
    {
        if (byteOrder == QDataStream::LittleEndian)
        {
            return qFromLittleEndian<T>(reinterpret_cast<const uchar *>(bytes));
        }
        return qFromBigEndian<T>(reinterpret_cast<const uchar *>(bytes));
    }

    template<typename T> static auto fixValueByteOrder(T value, QDataStream::ByteOrder byteOrder) -> T
    {
        if (byteOrder == QDataStream::LittleEndian)
        {
            return qFromLittleEndian<T>(value);
        }
        return qFromBigEndian<T>(value);
    }
    QVector<TiffIfd> m_ifds;

    QFile m_file;
    QDataStream m_dataStream;
};

} // namespace FileFormats
