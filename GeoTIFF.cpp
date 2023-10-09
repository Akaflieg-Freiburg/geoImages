

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


qint64 FileFormats::GeoTIFF::readHeader()
{
    // magic bytes
    auto magicBytes = m_file.read(2);
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
        setError( QObject::tr("Invalid TIFF file", "FileFormats::GeoTIFF") );
        return -1;
    }

    // version
    quint16 version = 0;
    m_dataStream >> version;
    if (version == 43)
    {
        setError( QObject::tr("BigTIFF files are not supported", "FileFormats::GeoTIFF") );
        return -1;
    }
    if (version != 42)
    {
        setError( QObject::tr("Unsupported TIFF version", "FileFormats::GeoTIFF") );
        return -1;
    }

    // ifd0Offset
    quint32 result = 0;
    m_dataStream >> result;
    return result;;
}


FileFormats::GeoTIFF::TIFFField FileFormats::GeoTIFF::readTIFFField()
{
    TIFFField field;
    m_dataStream >> field.m_tag;
    m_dataStream >> field.m_type;
    m_dataStream >> field.m_count;

    auto filePos = m_file.pos();

    auto byteSize = field.typeSize()*field.m_count;
    if (byteSize > 4)
    {
        quint32 newPos = 0;
        m_dataStream >> newPos;
        m_file.seek(newPos);
    }


    switch (field.m_type)
    {
    case TIFFField::DT_Ascii:
    {
        auto tmpBytes = m_file.read(field.m_count);
        foreach(auto stringBytes, tmpBytes.split(0))
        {
            field.m_values.append(QString::fromLatin1(stringBytes));
        }
    }
    case TIFFField::DT_Short:
        for (quint32 i = 0; i < field.m_count; ++i)
        {
            quint16 tmpInt = 0;
            m_dataStream >> tmpInt;
            field.m_values.append(tmpInt);
        }
        break;
    case TIFFField::DT_Double:
        for (quint32 i = 0; i < field.m_count; ++i)
        {
            double tmpFloat = qQNaN();
            m_dataStream >> tmpFloat;
            field.m_values.append(tmpFloat);
        }
        break;
    default:
        break;
    }

    m_file.seek(filePos+4);
    return field;
}



bool FileFormats::GeoTIFF::readIfd(qint64 offset)
{
    if (!m_file.seek(offset))
    {
        setError(m_file.errorString());
        return false;
    }

    QVector<TIFFField> m_ifdEntries;

    quint16 deCount = 0;
    m_dataStream >> deCount;
    for (quint16 i = 0; i < deCount; ++i)
    {
        auto field = readTIFFField();
        if ((field.m_tag == 256) || (field.m_tag == 257) || (field.m_tag == 270) || (field.m_tag == 33550) || (field.m_tag == 33922))
        {
            m_ifdEntries.append(field);
        }
    }


    // parser data of ifdEntry
    foreach (auto field, m_ifdEntries)
    {
        qWarning() << field.m_tag << field.m_values;
        if (field.m_tag == 256)
        {
            m_geo.width = field.m_values.last().toInt();
        }
        else if (field.m_tag == 257)
        {
            m_geo.height = field.m_values.last().toInt();
        }
        else if (field.m_tag == 270)
        {
            m_geo.desc = field.m_values.last().toString();
        }
        else if (field.m_tag == 33550)
        {
            m_geo.pixelWidth = field.m_values.at(0).toDouble();
            m_geo.pixelHeight = field.m_values.at(1).toDouble();
        }
        else if (field.m_tag == 33922)
        {
            m_geo.longitute = field.m_values.at(3).toDouble();
            m_geo.latitude = field.m_values.at(4).toDouble();
        }
    }
    return true;
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

    auto ifd0Offset = readHeader();
    if (ifd0Offset <= 0)
    {
        m_file.close();
        return;
    }

    try
    {
        readIfd(ifd0Offset);
        getMeta();
    }
    catch (QString& ex)
    {
        setError(ex);
    }
    m_file.close();
}


void FileFormats::GeoTIFF::getMeta()
{
    if ((m_geo.longitute == 0) || (m_geo.latitude == 0))
    {
        throw QObject::tr("Tag 33922 is not set", "FileFormats::GeoTIFF");
    }
    if ((m_geo.pixelWidth == 0) || (m_geo.pixelHeight == 0))
    {
        throw QObject::tr("Tag 33550 is not set", "FileFormats::GeoTIFF");
    }
    if (m_geo.width == 0)
    {
        throw QObject::tr("Tag 256 is not set", "FileFormats::GeoTIFF");
    }
    if (m_geo.height == 0)
    {
        throw QObject::tr("Tag 257 is not set", "FileFormats::GeoTIFF");
    }

    QGeoCoordinate coord;
    coord.setLongitude(m_geo.longitute);
    coord.setLatitude(m_geo.latitude);
    m_bBox.setTopLeft(coord);
    coord.setLongitude(m_geo.longitute + (m_geo.width - 1) * m_geo.pixelWidth);
    if (m_geo.pixelHeight > 0)
    {
        coord.setLatitude(m_geo.latitude - (m_geo.height - 1) * m_geo.pixelHeight);
    }
    else
    {
        coord.setLatitude(m_geo.latitude + (m_geo.height - 1) * m_geo.pixelHeight);
    }
    m_bBox.setBottomRight(coord);
    m_name = m_geo.desc;
}
