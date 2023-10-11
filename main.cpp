
#include <QCommandLineParser>
#include <QImage>

#include "GeoTIFF.h"

auto main(int argc, char *argv[]) -> int
{
    QCoreApplication const app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Test for geoImages"_qs);
    parser.addHelpOption();
    parser.addPositionalArgument(u"image"_qs, u"GeoTagged Image File"_qs);
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1)
    {
        parser.showHelp(-1);
    }

    const QString& fileName = args[0];
    FileFormats::GeoTIFF const geoTIFF(fileName);
    if (geoTIFF.isValid())
    {
        qWarning() << u"GeoTIFF file %1 is valid"_qs.arg(fileName);
        qWarning() << u"Name"_qs << geoTIFF.name();
        auto rect = geoTIFF.bBox();
        qWarning() << u"Corner coordinates:"_qs
                   << rect.bottomLeft().longitude()
                   << rect.bottomRight().longitude()
                   << rect.bottomLeft().latitude()
                   << rect.topLeft().latitude() ;

        // Quick check if we can read the raster image
        QImage const img(fileName);
        qWarning() << img;
        img.save(u"t.png"_qs);
    }
    else
    {
        qWarning() << u"GeoTIFF file %1 is invalid."_qs.arg(fileName);
        qWarning() << geoTIFF.error();
    }


    return 0;
}
