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


//
// Enums and static helper functions
//

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

// Checks the status of dataStream. If status is OK, this method does nothing. Otherwiese, it throws a QString with a human-readable, translated error message.
void checkForError(QDataStream& dataStream)
{
    switch(dataStream.status())
    {
    case QDataStream::ReadCorruptData:
        throw QObject::tr("Found corrupt data.", "FileFormats::GeoTIFF");
    case QDataStream::ReadPastEnd:
        throw QObject::tr("Read past end of data stream.", "FileFormats::GeoTIFF");
    case QDataStream::WriteFailed:
        throw QObject::tr("Error writing to data stream.", "FileFormats::GeoTIFF");
    case QDataStream::Ok:
        break;
    }
}



//
// Constructors
//

FileFormats::GeoTIFF::GeoTIFF(const QString& fileName)
{
    QFile inFile(fileName);
    if (!inFile.open(QFile::ReadOnly))
    {
        setError(inFile.errorString());
        return;
    }

    readTIFFData(inFile);
}

FileFormats::GeoTIFF::GeoTIFF(QIODevice& device)
{
    readTIFFData(device);
}



//
// Private Methods
//

void FileFormats::GeoTIFF::readTIFFData(QIODevice& device)
{
    QDataStream dataStream(&device);

    try
    {
        // Move to beginning of the data stream
        if (!device.seek(0))
        {
            throw device.errorString();
        }

        // Check magic bytes
        auto magicBytes = device.read(2);
        if (magicBytes == "II")
        {
            dataStream.setByteOrder(QDataStream::LittleEndian);
        }
        else if (magicBytes == "MM")
        {
            dataStream.setByteOrder(QDataStream::BigEndian);
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
        checkForError(dataStream);
        if (!device.seek(result))
        {
            throw device.errorString();
        }

        quint16 tagCount = 0;
        dataStream >> tagCount;
        checkForError(dataStream);
        if (tagCount > 100)
        {
            addWarning( QObject::tr("Found more than 100 tags in the TIFF file. Reading only the first 100.", "FileFormats::GeoTIFF") );
            tagCount = 100;
        }

        for (quint16 i=0; i<tagCount; ++i)
        {
            readTIFFField(device, dataStream);
        }

        interpretGeoData();
    }
    catch (QString& message)
    {
        setError(message);
    }
}

void FileFormats::GeoTIFF::readTIFFField(QIODevice& device, QDataStream& dataStream)
{
    // Read tag, type, and count
    quint16 tag = 0;
    quint16 type = DT_Undefined;
    quint32 count = 0;
    dataStream >> tag;
    dataStream >> type;
    dataStream >> count;
    checkForError(dataStream);

    // Compute the data size of this type
    int typeSize = 0;
    switch(type)
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

    // Save file position and move to the position where the data actually resides.
    auto filePos = device.pos();
    auto byteSize = typeSize*count;
    if (byteSize > 4)
    {
        quint32 newPos = 0;
        dataStream >> newPos;
        checkForError(dataStream);
        if (!device.seek(newPos))
        {
            throw device.errorString();
        }
    }

    // Read data entries from the device
    QVariantList values;
    switch (type)
    {
    case DT_Ascii:
    {
        auto tmpString = device.read(count);
        if (tmpString.size() != count)
        {
            throw QObject::tr("Cannot read data.", "FileFormats::GeoTIFF");
        }
        foreach(auto subStrings, tmpString.split(0))
        {
            values.append(QString::fromLatin1(subStrings));
        }
    }
    break;
    case DT_Short:
        values.reserve(count);
        for (quint32 i = 0; i < count; ++i)
        {
            quint16 tmpInt = 0;
            dataStream >> tmpInt;
            checkForError(dataStream);
            values.append(tmpInt);
        }
        break;
    case DT_Double:
        values.reserve(count);
        for (quint32 i = 0; i < count; ++i)
        {
            double tmpFloat = NAN;
            dataStream >> tmpFloat;
            checkForError(dataStream);
            values.append(tmpFloat);
        }
        break;
    default:
        break;
    }

    // Position the device at the byte following the current entry
    if (!device.seek(filePos+4))
    {
        throw device.errorString();
    }

    m_TIFFFields[tag] = values;
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
