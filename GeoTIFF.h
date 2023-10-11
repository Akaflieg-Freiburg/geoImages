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
     *  the raster data and is therefore lightweight.
     *
     *  \param fileName File name of a GeoTIFF file.
     */
    GeoTIFF(const QString& fileName);

    /*! \brief Constructor
     *
     *  The constructor opens and analyzes the GeoTIFF file. It does not read
     *  the raster data and is therefore lightweight.
     *
     *  \param device Device from which the GeoTIFF is read. The device must be opened and seekable. The device will not be closed by this method.
     */
    GeoTIFF(QIODevice& device);


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



    //
    // Static methods
    //

    /*! \brief Mime type for files that can be opened by this class
     *
     *  @returns Name of mime type
     */
    [[nodiscard]] static QStringList mimeTypes() { return {u"image/tiff"_qs}; }

private:
    QGeoRectangle m_bBox;
    QString m_name;

    /* This methods reads the TIFF header data from the device.
     * On success, it set the correct endianness in the datastream and
     * positions the device at the beginning of the first IFD. On failure,
     * it throws a QString with a human-readable, translated error message.
     *
     * @param device QIODevice from which the TIFF header will be read. This device must be seekable.
     *
     * @param dataStream QDataStream connected to the device
     */
    void readTIFFData(QIODevice& device);

    /* This methods reads the TIFF header data from the device.
     * On success, it set the correct endianness in the datastream and
     * positions the device at the beginning of the first IFD. On failure,
     * it throws a QString with a human-readable, translated error message.
     *
     * @param device QIODevice from which the TIFF header will be read. This device must be open and seekable.
     *
     * @param dataStream QDataStream connected to the device.
     */
 //   static void readHeader(QIODevice& device, QDataStream& dataStream);

    /* This methods reads a TIFF directory from the device.
     * On success, it fills the member m_TIFFFields with tags and data. On failure,
     * it throws a QString with a human-readable, translated error message.
     *
     * @param device QIODevice from which the TIFF header will be read. This device must be open, seekable, and positioned to
     * the beginning of the directory structure.
     *
     * @param dataStream QDataStream that is connected to the device and has the correct endianness set.
     */
//    void readIFD(QIODevice& device, QDataStream& dataStream);

    /* This methods reads a single TIFF field from the device.
     * On success, it adds an entry to the member m_TIFFFields and positions the device on the byte following the structure. On failure,
     * it throws a QString with a human-readable, translated error message.
     *
     * @param device QIODevice from which the TIFF header will be read. This device must be open, seekable, and positioned to
     * the beginning of the TIFF field structure.
     *
     * @param dataStream QDataStream that is connected to the device and has the correct endianness set.
     */
    void readTIFFField(QIODevice& device, QDataStream& dataStream);

    void interpretGeoData();

    QMap<quint16, QVariantList> m_TIFFFields;

};

} // namespace FileFormats
