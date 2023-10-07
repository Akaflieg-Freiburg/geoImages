
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
    FileFormats::GeoTiffMeta const geoTiffMeta = FileFormats::GeoTIFF::readMetaData(fileName);

    qWarning() << u"Corner coordinates for image %1:"_qs.arg(fileName)
               << geoTiffMeta.rect.bottomLeft().longitude()
               << geoTiffMeta.rect.bottomRight().longitude()
               << geoTiffMeta.rect.bottomLeft().latitude()
               << geoTiffMeta.rect.topLeft().latitude() ;

    qWarning() << u"Description of the image" << geoTiffMeta.desc;

    // Quick check if we can read the raster image
    QImage const img(fileName);
    qWarning() << img;
    img.save("t.png");

    return 0;
}
