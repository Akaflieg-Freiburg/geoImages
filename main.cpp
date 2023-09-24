
#include <QCommandLineParser>
#include <QImage>

#include "GeoTIFF.h"

auto main(int argc, char *argv[]) -> int
{
    QCoreApplication const app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Test for geoImages");
    parser.addHelpOption();
    parser.addPositionalArgument("image", "GeoTagged Image File");
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1)
    {
        parser.showHelp(-1);
    }

    const QString& fileName = args[0];
    auto rectangle = GeoMaps::GeoTIFF::readCoordinates(fileName);

    qWarning() << u"Corner coordinates for image %1:"_qs.arg(fileName)
               << rectangle.bottomLeft().longitude()
               << rectangle.bottomRight().longitude()
               << rectangle.bottomLeft().latitude()
               << rectangle.topLeft().latitude() ;

    // Quick check if we can read the raster image
    QImage const img(fileName);
    qWarning() << img;
    img.save("t.png");

    QString desc = GeoMaps::GeoTIFF::readDescription(fileName);

    qWarning() << u"Description of the image" << desc;

    return 0;
}
