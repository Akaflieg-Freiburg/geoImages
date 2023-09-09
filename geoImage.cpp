#include <QImage>
#include <QScopedPointer>
#include <QVector>
#include <QVariant>
#include <QExplicitlySharedDataPointer>
#include "geoImage.h"
#include <QFile>
#include <QLoggingCategory>
#include <QtEndian>
#include <QSharedData>
#include <algorithm>
#include <QGeoRectangle>

class QByteArray;
class TiffIfdEntryPrivate;
class TiffIfdPrivate;
class TiffFilePrivate;

class TiffIfdEntry
{
public:
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

    TiffIfdEntry();
    TiffIfdEntry(const TiffIfdEntry &other);
    ~TiffIfdEntry();

    [[nodiscard]] auto tag() const -> quint16;
    [[nodiscard]] auto tagName() const -> QString;
    [[nodiscard]] auto type() const -> quint16;
    [[nodiscard]] auto typeName() const -> QString;
    [[nodiscard]] auto count() const -> quint64;
    [[nodiscard]] auto valueOrOffset() const -> QByteArray;
    [[nodiscard]] auto values() const -> QVariantList;
    [[nodiscard]] auto isValid() const -> bool;

private:
    friend class TiffFilePrivate;
    QExplicitlySharedDataPointer<TiffIfdEntryPrivate> d;
};

class TiffIfd
{
public:
    TiffIfd();
    TiffIfd(const TiffIfd &other);
    ~TiffIfd();

    [[nodiscard]] auto ifdEntries() const -> QVector<TiffIfdEntry>;
    [[nodiscard]] auto subIfds() const -> QVector<TiffIfd>;
    [[nodiscard]] auto nextIfdOffset() const -> qint64;
    [[nodiscard]] auto isValid() const -> bool;

private:
    friend class TiffFilePrivate;
    QExplicitlySharedDataPointer<TiffIfdPrivate> d;
};

class TiffFile
{
public:
    enum ByteOrder { LittleEndian, BigEndian };

    TiffFile(const QString &filePath);
    ~TiffFile();

    [[nodiscard]] auto errorString() const -> QString;
    [[nodiscard]] auto hasError() const -> bool;

    // header information
    [[nodiscard]] auto headerBytes() const -> QByteArray;
    [[nodiscard]] auto isBigTiff() const -> bool;
    [[nodiscard]] auto byteOrder() const -> ByteOrder;
    [[nodiscard]] auto version() const -> int;
    [[nodiscard]] auto ifd0Offset() const -> qint64;

    [[nodiscard]] auto getRect() const -> QGeoRectangle;

    // ifds
    [[nodiscard]] auto ifds() const -> QVector<TiffIfd>;

private:
    QScopedPointer<TiffFilePrivate> d;
};


QString err_256_not_set = QString::fromLatin1("Tag 256 is not set");
QString err_257_not_set = QString::fromLatin1("Tag 257 is not set");
QString err_33550_not_set = QString::fromLatin1("Tag 33550 is not set");
QString err_33922_not_set = QString::fromLatin1("Tag 33922 is not set");
QString err_file_read = QString::fromLatin1("File read error");
QString err_seek_pos = QString::fromLatin1("Fail to seek pos: ");

auto GeoMaps::GeoImage::readCoordinates(const QString &path) -> QGeoRectangle
{
    TiffFile const tiff(path);

    try
    {
        return tiff.getRect();
    }
    catch (QString& ex)
    {
        qWarning() << " " << ex;
    }

    return {}; // Return a default-constructed (hence invalid) QGeoRectangle
}

namespace  {

template<typename T> inline auto getValueFromBytes(const char *bytes, TiffFile::ByteOrder byteOrder) -> T
{
    if (byteOrder == TiffFile::LittleEndian)
    {
        return qFromLittleEndian<T>(reinterpret_cast<const uchar *>(bytes));
    }
    return qFromBigEndian<T>(reinterpret_cast<const uchar *>(bytes));
}

template<typename T> inline auto fixValueByteOrder(T value, TiffFile::ByteOrder byteOrder) -> T
{
    if (byteOrder == TiffFile::LittleEndian)
    {
        return qFromLittleEndian<T>(value);
    }
    return qFromBigEndian<T>(value);
}

}

class TiffIfdEntryPrivate : public QSharedData
{
public:
    TiffIfdEntryPrivate() = default;
    TiffIfdEntryPrivate(const TiffIfdEntryPrivate &other)
        : QSharedData(other)
        , tag(other.tag)
        , type(other.type)
        , count(other.count)
        , valueOrOffset(other.valueOrOffset)
    {
    }
    ~TiffIfdEntryPrivate() = default;

    auto typeSize() const -> int
    {
        switch (type) {
        case TiffIfdEntry::DT_Byte:
        case TiffIfdEntry::DT_SByte:
        case TiffIfdEntry::DT_Ascii:
        case TiffIfdEntry::DT_Undefined:
            return 1;
        case TiffIfdEntry::DT_Short:
        case TiffIfdEntry::DT_SShort:
            return 2;
        case TiffIfdEntry::DT_Long:
        case TiffIfdEntry::DT_SLong:
        case TiffIfdEntry::DT_Ifd:
        case TiffIfdEntry::DT_Float:
            return 4;

        case TiffIfdEntry::DT_Rational:
        case TiffIfdEntry::DT_SRational:
        case TiffIfdEntry::DT_Long8:
        case TiffIfdEntry::DT_SLong8:
        case TiffIfdEntry::DT_Ifd8:
        case TiffIfdEntry::DT_Double:
            return 8;
        default:
            return 0;
        }
    }

    void parserValues(const char *bytes, TiffFile::ByteOrder byteOrder);

    quint16 tag;
    quint16 type;
    quint64 count{ 0 };
    QByteArray valueOrOffset; // 12 bytes for tiff or 20 bytes for bigTiff
    QVariantList values;
};

void TiffIfdEntryPrivate::parserValues(const char *bytes, TiffFile::ByteOrder byteOrder)
{
    // To make things simple, save normal integer as qint32 or quint32 here.
    for (int i = 0; i < count; ++i) {
        QVariant const variant;
        switch (type) {
        case TiffIfdEntry::DT_Short:
            values.append(static_cast<quint32>(getValueFromBytes<quint16>(bytes + i * 2, byteOrder)));
            break;
        case TiffIfdEntry::DT_Double:
            double f;
            if (byteOrder == TiffFile::BigEndian)
            {
                char rbytes[8];
                rbytes[0] = bytes[i*8+7];
                rbytes[1] = bytes[i*8+6];
                rbytes[2] = bytes[i*8+5];
                rbytes[3] = bytes[i*8+4];
                rbytes[4] = bytes[i*8+3];
                rbytes[5] = bytes[i*8+2];
                rbytes[6] = bytes[i*8+1];
                rbytes[7] = bytes[i*8];
                memcpy( &f, rbytes, 8 );
            }
            else
            {
                memcpy( &f, bytes + i * 8, 8 );
            }
            values.append(f);
            break;
        default:
            break;
        }
    }
}

/*!
 * \class TiffIfdEntry
 */

TiffIfdEntry::TiffIfdEntry()
    : d(new TiffIfdEntryPrivate)
{
}

TiffIfdEntry::TiffIfdEntry(const TiffIfdEntry &other) = default;

TiffIfdEntry::~TiffIfdEntry() = default;

auto TiffIfdEntry::tag() const -> quint16
{
    return d->tag;
}

auto TiffIfdEntry::type() const -> quint16
{
    return d->type;
}

auto TiffIfdEntry::count() const -> quint64
{
    return d->count;
}

auto TiffIfdEntry::valueOrOffset() const -> QByteArray
{
    return d->valueOrOffset;
}

auto TiffIfdEntry::values() const -> QVariantList
{
    return d->values;
}

auto TiffIfdEntry::isValid() const -> bool
{
    return d->count != 0U;
}

class TiffIfdPrivate : public QSharedData
{
public:
    TiffIfdPrivate() = default;
    TiffIfdPrivate(const TiffIfdPrivate &other) = default;
    ~TiffIfdPrivate() = default;

    auto hasIfdEntry(quint16 tag) -> bool;
    auto ifdEntry(quint16 tag) const -> TiffIfdEntry;

    QVector<TiffIfdEntry> ifdEntries;
    QVector<TiffIfd> subIfds;
    qint64 nextIfdOffset{ 0 };
};

auto TiffIfdPrivate::hasIfdEntry(quint16 tag) -> bool
{
    return ifdEntry(tag).isValid();
}

auto TiffIfdPrivate::ifdEntry(quint16 tag) const -> TiffIfdEntry
{
    auto it = std::find_if(ifdEntries.cbegin(), ifdEntries.cend(),
                           [tag](const TiffIfdEntry &de) { return tag == de.tag(); });
    if (it == ifdEntries.cend())
    {
        return {};
    }
    return *it;
}

/*!
 * \class TiffIfd
 */

TiffIfd::TiffIfd()
    : d(new TiffIfdPrivate)
{
}

TiffIfd::TiffIfd(const TiffIfd &other) = default;

TiffIfd::~TiffIfd() = default;

auto TiffIfd::ifdEntries() const -> QVector<TiffIfdEntry>
{
    return d->ifdEntries;
}

auto TiffIfd::subIfds() const -> QVector<TiffIfd>
{
    return d->subIfds;
}

auto TiffIfd::nextIfdOffset() const -> qint64
{
    return d->nextIfdOffset;
}

auto TiffIfd::isValid() const -> bool
{
    return !d->ifdEntries.isEmpty();
}

class TiffFilePrivate
{
public:
    TiffFilePrivate();
    void setError(const QString &errorString);
    auto readHeader() -> bool;
    auto readIfd(qint64 offset, TiffIfd *parentIfd = nullptr) -> bool;

    template<typename T> auto getValueFromFile() -> T
    {
        T v{ 0 };
        auto bytesRead = file.read(reinterpret_cast<char *>(&v), sizeof(T));
        if (bytesRead != sizeof(T))
        {
            throw err_file_read;
        }
        return fixValueByteOrder(v, header.byteOrder);
    }

    struct Header
    {
        QByteArray rawBytes;
        TiffFile::ByteOrder byteOrder{ TiffFile::LittleEndian };
        quint16 version{ 42 };
        qint64 ifd0Offset{ 0 };

        [[nodiscard]] auto isBigTiff() const -> bool { return version == 43; }
    } header;

    struct Geo
    {
        quint16 width = 0;
        quint16 height = 0;
        double longitute = 0;
        double latitude = 0;
        double pixelWidth = 0;
        double pixelHeight = 0;
    } geo;

    QVector<TiffIfd> ifds;

    QFile file;
    QString errorString;
    bool hasError{ false };
};

TiffFilePrivate::TiffFilePrivate() = default;

void TiffFilePrivate::setError(const QString &errorString)
{
    hasError = true;
    this->errorString = errorString;
}

auto TiffFilePrivate::readHeader() -> bool
{
    auto headerBytes = file.peek(8);
    if (headerBytes.size() != 8)
    {
        setError(QStringLiteral("Invalid tiff file"));
        return false;
    }

    // magic bytes
    auto magicBytes = headerBytes.left(2);
    if (magicBytes == QByteArray("II"))
    {
        header.byteOrder = TiffFile::LittleEndian;
    }
    else if (magicBytes == QByteArray("MM"))
    {
        header.byteOrder = TiffFile::BigEndian;
    }
    else
    {
        setError(QStringLiteral("Invalid tiff file"));
        return false;
    }

    // version
    header.version = getValueFromBytes<quint16>(headerBytes.data() + 2, header.byteOrder);
    if (header.version != 42 && header.version != 43)
    {
        setError(QStringLiteral("Invalid tiff file: Unknown version"));
        return false;
    }
    header.rawBytes = file.read(header.isBigTiff() ? 16 : 8);

    // ifd0Offset
    if (!header.isBigTiff())
    {
        header.ifd0Offset =
            getValueFromBytes<quint32>(header.rawBytes.data() + 4, header.byteOrder);
    }
    else
    {
        header.ifd0Offset = getValueFromBytes<qint64>(header.rawBytes.data() + 8, header.byteOrder);
    }

    return true;
}

auto TiffFilePrivate::readIfd(qint64 offset, TiffIfd * /*parentIfd*/) -> bool
{
    if (!file.seek(offset))
    {
        setError(file.errorString());
        return false;
    }

    TiffIfd ifd;

    if (!header.isBigTiff())
    {
        auto const deCount = getValueFromFile<quint16>();
        for (int i = 0; i < deCount; ++i)
        {
            TiffIfdEntry ifdEntry;
            auto &dePrivate = ifdEntry.d;
            dePrivate->tag = getValueFromFile<quint16>();
            dePrivate->type = getValueFromFile<quint16>();
            dePrivate->count = getValueFromFile<quint32>();
            dePrivate->valueOrOffset = file.read(4);
            if ((dePrivate->tag == 256) || (dePrivate->tag == 257) || (dePrivate->tag == 33550) || (dePrivate->tag == 33922))
            {
                ifd.d->ifdEntries.append(ifdEntry);
            }
        }
        ifd.d->nextIfdOffset = getValueFromFile<quint32>();
    }
    else
    {
        auto const deCount = getValueFromFile<quint64>();
        for (quint64 i = 0; i < deCount; ++i)
        {
            TiffIfdEntry ifdEntry;
            auto &dePrivate = ifdEntry.d;
            dePrivate->tag = getValueFromFile<quint16>();
            dePrivate->type = getValueFromFile<quint16>();
            dePrivate->count = getValueFromFile<quint64>();
            dePrivate->valueOrOffset = file.read(8);
            if ((dePrivate->tag == 256) || (dePrivate->tag == 257) || (dePrivate->tag == 33550) || (dePrivate->tag == 33922))
            {
                ifd.d->ifdEntries.append(ifdEntry);
            }
        }
        ifd.d->nextIfdOffset = getValueFromFile<qint64>();
    }

    // parser data of ifdEntry
    foreach (auto de, ifd.ifdEntries())
    {
        auto &dePrivate = de.d;

        auto valueBytesCount = dePrivate->count * dePrivate->typeSize();
        // skip unknown datatype
        if (valueBytesCount == 0)
        {
            continue;
        }
        QByteArray valueBytes;
        if (!header.isBigTiff() && valueBytesCount > 4)
        {
            auto valueOffset = getValueFromBytes<quint32>(de.valueOrOffset(), header.byteOrder);
            if (!file.seek(valueOffset))
            {
                throw err_seek_pos + QString::number(valueOffset);
            }
            valueBytes = file.read(valueBytesCount);
        }
        else if (header.isBigTiff() && valueBytesCount > 8)
        {
            auto valueOffset = getValueFromBytes<quint64>(de.valueOrOffset(), header.byteOrder);
            if (!file.seek(valueOffset))
            {
                throw err_seek_pos + QString::number(valueOffset);
            }
            valueBytes = file.read(valueBytesCount);
        }
        else
        {
            valueBytes = dePrivate->valueOrOffset;
        }
        dePrivate->parserValues(valueBytes, header.byteOrder);
        if (dePrivate->tag == 256)
        {
            geo.width = dePrivate->values.last().toInt();
        } else if (dePrivate->tag == 257)
        {
            geo.height = dePrivate->values.last().toInt();
        } else if (dePrivate->tag == 33550)
        {
            geo.pixelWidth = dePrivate->values.at(0).toDouble();
            geo.pixelHeight = dePrivate->values.at(1).toDouble();
        } else if (dePrivate->tag == 33922)
        {
            geo.longitute = dePrivate->values.at(3).toDouble();
            geo.latitude = dePrivate->values.at(4).toDouble();
        }
    }

    return true;
}

/*!
 * \class TiffFile
 */

/*!
 * Constructs the TiffFile object.
 */

TiffFile::TiffFile(const QString &filePath)
    : d(new TiffFilePrivate)
{
    d->file.setFileName(filePath);
    if (!d->file.open(QFile::ReadOnly))
    {
        d->hasError = true;
        d->errorString = d->file.errorString();
    }

    if (!d->readHeader())
    {
        return;
    }

    d->readIfd(d->header.ifd0Offset);
}

TiffFile::~TiffFile() = default;

auto TiffFile::getRect() const -> QGeoRectangle
{
    if ((d->geo.longitute == 0) || (d->geo.latitude == 0))
    {
        throw err_33922_not_set;
    }
    if ((d->geo.pixelWidth == 0) || (d->geo.pixelHeight == 0))
    {
        throw err_33550_not_set;
    }
    if (d->geo.width == 0)
    {
        throw err_256_not_set;
    }
    if (d->geo.height == 0)
    {
        throw err_257_not_set;
    }

    QGeoRectangle rect;
    QGeoCoordinate coord;
    coord.setLongitude(d->geo.longitute);
    coord.setLatitude(d->geo.latitude);
    rect.setTopLeft(coord);
    coord.setLongitude(d->geo.longitute + (d->geo.width - 1) * d->geo.pixelWidth);
    coord.setLatitude(d->geo.latitude + (d->geo.height - 1) * d->geo.pixelHeight);
    rect.setBottomRight(coord);
    return rect;
}

auto TiffFile::headerBytes() const -> QByteArray
{
    return d->header.rawBytes;
}

auto TiffFile::isBigTiff() const -> bool
{
    return d->header.isBigTiff();
}

auto TiffFile::byteOrder() const -> TiffFile::ByteOrder
{
    return d->header.byteOrder;
}

auto TiffFile::version() const -> int
{
    return d->header.version;
}

auto TiffFile::ifd0Offset() const -> qint64
{
    return d->header.ifd0Offset;
}

auto TiffFile::ifds() const -> QVector<TiffIfd>
{
    return d->ifds;
}

auto TiffFile::errorString() const -> QString
{
    return d->errorString;
}

auto TiffFile::hasError() const -> bool
{
    return d->hasError;
}

