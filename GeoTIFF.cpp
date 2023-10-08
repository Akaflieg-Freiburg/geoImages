

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
    quint16 version;
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
    quint32 result;
    m_dataStream >> result;
    return result;;
}


bool FileFormats::GeoTIFF::readIfd(qint64 offset)
{
    if (!m_file.seek(offset))
    {
        setError(m_file.errorString());
        return false;
    }

    TiffIfd ifd;

    quint16 deCount;
    m_dataStream >> deCount;
    for (int i = 0; i < deCount; ++i)
    {
        TiffIfdEntry ifdEntry;
        auto &dePrivate = ifdEntry;
        m_dataStream >> dePrivate.m_tag;
        m_dataStream >> dePrivate.m_type;
        m_dataStream >> dePrivate.m_count;
        dePrivate.m_valueOrOffset = m_file.read(4);
        if ((dePrivate.m_tag == 256) || (dePrivate.m_tag == 257) || (dePrivate.m_tag == 270) || (dePrivate.m_tag == 33550) || (dePrivate.m_tag == 33922))
        {
            ifd.m_ifdEntries.append(ifdEntry);
        }
    }
    m_dataStream >> ifd.m_nextIfdOffset;

    // parser data of ifdEntry
    foreach (auto ifdEntry, ifd.m_ifdEntries)
    {
        auto &dePrivate = ifdEntry;

        auto valueBytesCount = dePrivate.m_count * dePrivate.typeSize();
        // skip unknown datatype
        if (valueBytesCount == 0)
        {
            continue;
        }
        QByteArray valueBytes;
        if (valueBytesCount > 4)
        {
            auto valueOffset = getValueFromBytes<quint32>(ifdEntry.m_valueOrOffset, m_dataStream.byteOrder());
            if (!m_file.seek(valueOffset))
            {
                throw QObject::tr("File seek error", "FileFormats::GeoTIFF");
            }
            valueBytes = m_file.read(valueBytesCount);
        }
        else
        {
            valueBytes = dePrivate.m_valueOrOffset;
        }
        dePrivate.parserValues(valueBytes, m_dataStream.byteOrder());
        if (dePrivate.m_tag == 256)
        {
            m_geo.width = dePrivate.m_values.last().toInt();
        }
        else if (dePrivate.m_tag == 257)
        {
            m_geo.height = dePrivate.m_values.last().toInt();
        }
        else if (dePrivate.m_tag == 270)
        {
            m_geo.desc = dePrivate.m_values.last().toString();
        }
        else if (dePrivate.m_tag == 33550)
        {
            m_geo.pixelWidth = dePrivate.m_values.at(0).toDouble();
            m_geo.pixelHeight = dePrivate.m_values.at(1).toDouble();
        }
        else if (dePrivate.m_tag == 33922)
        {
            m_geo.longitute = dePrivate.m_values.at(3).toDouble();
            m_geo.latitude = dePrivate.m_values.at(4).toDouble();
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
