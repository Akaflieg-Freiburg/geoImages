

/****************************************************************************
 *
 * This file contains code based on Debao Zhang's QtTiffTagViewer
 *
 * https://github.com/dbzhang800/QtTiffTagViewer
 *
 * The code for QtTiffTagViewer is licensed as follows:
 *
 * Copyright (c) 2018 Debao Zhang <hello@debao.me>
 * All right reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#include <QFile>

#include "GeoTIFF.h"

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


void FileFormats::GeoTIFF::readHeader(QIODevice& device, QDataStream& dataStream)
{
    // magic bytes
    auto magicBytes = device.read(2);
    if (magicBytes == "II")
    {
        m_dataStream.setByteOrder(QDataStream::LittleEndian);
    }
    else if (magicBytes == "MM")
    {
        m_dataStream.setByteOrder(QDataStream::BigEndian);
    }
    else
    {
        throw QObject::tr("Invalid TIFF file", "FileFormats::GeoTIFF");
    }

    // version
    quint16 version = 0;
    dataStream >> version;
    if (version == 43)
    {
        throw QObject::tr("BigTIFF files are not supported", "FileFormats::GeoTIFF");
    }
    if (version != 42)
    {
        throw QObject::tr("Unsupported TIFF version", "FileFormats::GeoTIFF");
    }

    // ifd0Offset
    quint32 result = 0;
    dataStream >> result;
    if (!m_file.seek(result))
    {
        throw m_file.errorString();
    }
}


void FileFormats::GeoTIFF::readTIFFField()
{
    quint32 m_count = 0;
    quint16 m_tag = 0;
    quint16 m_type = DT_Undefined;
    QVariantList m_values;

    m_dataStream >> m_tag;
    m_dataStream >> m_type;
    m_dataStream >> m_count;

    int typeSize = 0;
    switch(m_type)
    {
    case DT_Byte:
    case DT_SByte:
    case DT_Ascii:
    case DT_Undefined:
        typeSize = 1;
        break;

    case DT_Short:
    case DT_SShort:
        typeSize = 2;
        break;

    case DT_Long:
    case DT_SLong:
    case DT_Ifd:
    case DT_Float:
        typeSize = 4;
        break;

    case DT_Rational:
    case DT_SRational:
    case DT_Long8:
    case DT_SLong8:
    case DT_Ifd8:
    case DT_Double:
        typeSize = 8;
        break;

    default:
        typeSize = 0;
        break;
    }

    auto filePos = m_file.pos();

    auto byteSize = typeSize*m_count;
    if (byteSize > 4)
    {
        quint32 newPos = 0;
        m_dataStream >> newPos;
        m_file.seek(newPos);
    }


    switch (m_type)
    {
    case DT_Ascii:
    {
        auto tmpBytes = m_file.read(m_count);
        foreach(auto stringBytes, tmpBytes.split(0))
        {
            m_values.append(QString::fromLatin1(stringBytes));
        }
    }
    break;
    case DT_Short:
        for (quint32 i = 0; i < m_count; ++i)
        {
            quint16 tmpInt = 0;
            m_dataStream >> tmpInt;
            m_values.append(tmpInt);
        }
        break;
    case DT_Double:
        for (quint32 i = 0; i < m_count; ++i)
        {
            double tmpFloat = qQNaN();
            m_dataStream >> tmpFloat;
            m_values.append(tmpFloat);
        }
        break;
    default:
        break;
    }
    m_file.seek(filePos+4);

    m_TIFFFields[m_tag] = m_values;
}


bool FileFormats::GeoTIFF::readIFD()
{
    quint16 deCount = 0;
    m_dataStream >> deCount;
    for (quint16 i = 0; i < deCount; ++i)
    {
        readTIFFField();
    }



    return true;
}


void FileFormats::GeoTIFF::interpretGeoData()
{
    quint16 width = 0;
    quint16 height = 0;
    double longitute = 0;
    double latitude = 0;
    double pixelWidth = 0;
    double pixelHeight = 0;
    QString desc;

    if (!m_TIFFFields.contains(256))
    {
        throw QObject::tr("Tag 256 is not set", "FileFormats::GeoTIFF");
    }
    width = m_TIFFFields.value(256).constLast().toInt();

    if (!m_TIFFFields.contains(257))
    {
        throw QObject::tr("Tag 257 is not set", "FileFormats::GeoTIFF");
    }
    height = m_TIFFFields[257].constLast().toInt();

    if (!m_TIFFFields.contains(33922))
    {
        throw QObject::tr("Tag 33922 is not set", "FileFormats::GeoTIFF");
    }
    longitute = m_TIFFFields[33922].at(3).toDouble();
    latitude = m_TIFFFields[33922].at(4).toDouble();

    if (!m_TIFFFields.contains(33550))
    {
        throw QObject::tr("Tag 33550 is not set", "FileFormats::GeoTIFF");
    }
    pixelWidth = m_TIFFFields[33550].at(0).toDouble();
    pixelHeight = m_TIFFFields[33550].at(1).toDouble();

    if (m_TIFFFields.contains(270))
    {
        desc = m_TIFFFields[270].constLast().toString();
    }


    QGeoCoordinate coord;
    coord.setLongitude(longitute);
    coord.setLatitude(latitude);
    m_bBox.setTopLeft(coord);
    coord.setLongitude(longitute + (width - 1) * pixelWidth);
    if (pixelHeight > 0)
    {
        coord.setLatitude(latitude - (height - 1) * pixelHeight);
    }
    else
    {
        coord.setLatitude(latitude + (height - 1) * pixelHeight);
    }
    m_bBox.setBottomRight(coord);
    m_name = desc;
}


/*!
 * Constructs the TiffFile object.
 */

FileFormats::GeoTIFF::GeoTIFF(const QString& fileName)
{
    m_file.setFileName(fileName);
    if (!m_file.open(QFile::ReadOnly))
    {
        setError(m_file.errorString());
        return;
    }
    m_dataStream.setDevice(&m_file);


    try
    {
        readHeader(m_file, m_dataStream);
        readIFD();
        interpretGeoData();
    }
    catch (QString& ex)
    {
        setError(ex);
    }
    m_file.close();
}
