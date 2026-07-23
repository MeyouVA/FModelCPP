#pragma once
// No direct C# counterpart — this is port-support code.
//
// In C# the whole settings tree is (de)serialised by one reflective JsonConvert.SerializeObject call, so there
// is nothing to write per type. C++ has no reflection, so every settings type carries its own toJson/readJson
// pair; these helpers hold the conversions those pairs would otherwise each repeat, and pin the few places
// where Newtonsoft's defaults are not what Qt's JSON classes do by default:
//
//   * DateTime  -> ISO-8601 *with* the local UTC offset ("2026-07-20T23:42:18.117-04:00"). Newtonsoft writes 7
//     fractional digits, Qt writes 3; both parse the other's output, so the difference is cosmetic and only
//     shows up as a shorter fraction after FModel-C++ rewrites a file written by FModel-C#.
//   * enums     -> integers, which is Newtonsoft's default with no StringEnumConverter in play. That makes the
//     numeric value of every persisted enum part of the file format.
//   * 64-bit integers -> QJsonValue keeps qint64 exact (it is CBOR-backed in Qt 6), so CriwareDecryptionKey
//     survives values above 2^53 that a double-backed JSON writer would round.

#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace FModel::Extensions
{
    inline QJsonValue toJson(const QDateTime& value)
    {
        if (!value.isValid())
            return QJsonValue(QString());

        return value.toString(Qt::ISODateWithMs);
    }

    inline QDateTime dateTimeFromJson(const QJsonValue& value, const QDateTime& fallback = QDateTime())
    {
        if (!value.isString())
            return fallback;

        const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
        return parsed.isValid() ? parsed : fallback;
    }

    // Reads an enum stored as its underlying integer, leaving the default in place when the key is absent.
    template <typename E>
    E enumFromJson(const QJsonObject& json, const QString& key, E fallback)
    {
        const QJsonValue value = json.value(key);
        if (!value.isDouble())
            return fallback;

        return static_cast<E>(value.toInteger(static_cast<qint64>(fallback)));
    }

    template <typename E>
    QJsonValue enumToJson(E value)
    {
        return QJsonValue(static_cast<qint64>(value));
    }

    // bool/int/double/string readers that keep the existing default when a key is missing, matching
    // Newtonsoft's behaviour of simply not touching a property that is absent from the document.
    inline bool boolFromJson(const QJsonObject& json, const QString& key, bool fallback)
    {
        const QJsonValue value = json.value(key);
        return value.isBool() ? value.toBool() : fallback;
    }

    inline int intFromJson(const QJsonObject& json, const QString& key, int fallback)
    {
        const QJsonValue value = json.value(key);
        return value.isDouble() ? value.toInt(fallback) : fallback;
    }

    inline qint64 int64FromJson(const QJsonObject& json, const QString& key, qint64 fallback)
    {
        const QJsonValue value = json.value(key);
        return value.isDouble() ? value.toInteger(fallback) : fallback;
    }

    inline double doubleFromJson(const QJsonObject& json, const QString& key, double fallback)
    {
        const QJsonValue value = json.value(key);
        return value.isDouble() ? value.toDouble(fallback) : fallback;
    }

    inline QString stringFromJson(const QJsonObject& json, const QString& key, const QString& fallback = QString())
    {
        const QJsonValue value = json.value(key);
        return value.isString() ? value.toString() : fallback;
    }
}
