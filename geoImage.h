/***************************************************************************
 *   Copyright (C) 2022 by Stefan Kebekus                                  *
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

#include <QGeoRectangle>
#include <QImage>
#include <QScopedPointer>
#include <QVector>
#include <QVariant>
#include <QExplicitlySharedDataPointer>

class QByteArray;
class TiffIfdEntryPrivate;
class TiffIfdPrivate;
class TiffFilePrivate;

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

    TiffIfdEntry();
    TiffIfdEntry(const TiffIfdEntry &other);
    ~TiffIfdEntry();

    quint16 tag() const;
    QString tagName() const;
    quint16 type() const;
    QString typeName() const;
    quint64 count() const;
    QByteArray valueOrOffset() const;
    QVariantList values() const;
    bool isValid() const;

private:
    friend class TiffFilePrivate;
    QExplicitlySharedDataPointer<TiffIfdEntryPrivate> d;
};

class TiffIfd
{
public:
    TiffIfd();
    TiffIfd(const TiffIfd &other);
    ~TiffIfd();

    QVector<TiffIfdEntry> ifdEntries() const;
    QVector<TiffIfd> subIfds() const;
    qint64 nextIfdOffset() const;
    bool isValid() const;

private:
    friend class TiffFilePrivate;
    QExplicitlySharedDataPointer<TiffIfdPrivate> d;
};

class TiffFile
{
public:
    enum ByteOrder { LittleEndian, BigEndian };

    TiffFile(const QString &filePath);
    ~TiffFile();

    QString errorString() const;
    bool hasError() const;

    // header information
    QByteArray headerBytes() const;
    bool isBigTiff() const;
    ByteOrder byteOrder() const;
    int version() const;
    qint64 ifd0Offset() const;

    QGeoRectangle getRect() const;

    // ifds
    QVector<TiffIfd> ifds() const;

private:
    QScopedPointer<TiffFilePrivate> d;
};


namespace GeoMaps
{

    class GeoImage
    {
    public:
        /*! \brief Reads coordinates from a georeferenced image file
         *
         *  @param path File path for a georeferenced image file
         *
         *  @return Coordinates of the image corners. If no valid georeferencing data was
         *  found, an invalid QGeoRectangle is returned
         */
        static QGeoRectangle readCoordinates(const QString& path);
    };

} // namespace GeoMaps
