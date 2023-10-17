/*
 * Copyright © 2017 - 2019 Stefan Kebekus <stefan.kebekus@math.uni-freiburg.de>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GeoTIFF.h"
#include "GeoTIFFTest.h"

QTEST_MAIN(GeoTIFFTest)


void GeoTIFFTest::test()
{

    FileFormats::GeoTIFF test1( QString::fromLatin1(SRC) + u"/testData/GeoTIFF/EDKA.tiff"_qs );
    QVERIFY( test1.isValid() );
    QVERIFY( test1.bBox().topLeft().distanceTo({50.8549, 6.11667}) < 10 ); // Check if bounding box coordinate is within 10m of what we expect
    QVERIFY( test1.bBox().bottomRight().distanceTo({50.771, 6.24919}) < 10 ); // Check if bounding box coordinate is within 10m of what we expect
    QVERIFY( test1.name() == u""_qs );

}
