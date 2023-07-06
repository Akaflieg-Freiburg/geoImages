
#include "geoImage.h"

int main(int argc, char *argv[])
{
    QString fileName = u"test.png"_qs;

    auto rectangle = GeoMaps::GeoImage::readCoordinates(fileName);

    qWarning() << u"Corner coordinates for image %1:"_qs.arg(fileName)
               << rectangle;

    return 0;
}
