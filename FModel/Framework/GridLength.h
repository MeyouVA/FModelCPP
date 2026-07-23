#pragma once
// Stand-in for System.Windows.GridLength (WPF), the type of UserSettings.AvalonImageSize.
//
// Like InputKeys.h this exists for format compatibility rather than for layout: WPF's GridLengthConverter
// serialises a GridLength to a *string* ("Auto", "*", "2*", "730"), and that is what lands in AppSettings.json.
// The parse/format pair below reproduces that converter's grammar so settings files round-trip unchanged.
//
// Qt's splitters express the same idea as integer sizes, so the app layer converts at the point of use.

#include <QString>

namespace FModel::Framework
{
    enum class GridUnitType
    {
        Auto,
        Pixel,
        Star
    };

    class GridLength
    {
    public:
        GridLength() : _value(1.0), _unitType(GridUnitType::Auto) {}
        explicit GridLength(double pixels) : _value(pixels), _unitType(GridUnitType::Pixel) {}
        GridLength(double value, GridUnitType unitType) : _value(value), _unitType(unitType) {}

        double value() const { return _unitType == GridUnitType::Auto ? 1.0 : _value; }
        GridUnitType gridUnitType() const { return _unitType; }

        bool isAuto() const { return _unitType == GridUnitType::Auto; }
        bool isAbsolute() const { return _unitType == GridUnitType::Pixel; }
        bool isStar() const { return _unitType == GridUnitType::Star; }

        bool operator==(const GridLength& o) const { return _unitType == o._unitType && value() == o.value(); }
        bool operator!=(const GridLength& o) const { return !(*this == o); }

        // Mirrors GridLength.ToString() / GridLengthConverter.ConvertToString.
        QString toString() const
        {
            switch (_unitType)
            {
                case GridUnitType::Auto:
                    return QStringLiteral("Auto");
                case GridUnitType::Star:
                    return _value == 1.0 ? QStringLiteral("*") : formatNumber(_value) + QStringLiteral("*");
                case GridUnitType::Pixel:
                    break;
            }
            return formatNumber(_value);
        }

        // Mirrors GridLengthConverter.ConvertFromString. Anything unparseable falls back to Auto, which is what
        // a fresh GridLength() is — so a corrupt settings value degrades the same way WPF's would.
        static GridLength fromString(const QString& text)
        {
            const QString trimmed = text.trimmed();
            if (trimmed.isEmpty() || trimmed.compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0)
                return GridLength();

            if (trimmed.endsWith(QLatin1Char('*')))
            {
                const QString head = trimmed.left(trimmed.size() - 1).trimmed();
                if (head.isEmpty())
                    return GridLength(1.0, GridUnitType::Star);

                bool ok = false;
                const double parsed = head.toDouble(&ok);
                return ok ? GridLength(parsed, GridUnitType::Star) : GridLength();
            }

            bool ok = false;
            const double parsed = trimmed.toDouble(&ok);
            return ok ? GridLength(parsed) : GridLength();
        }

    private:
        // WPF formats with the invariant culture and no trailing ".0" for whole numbers ("730", not "730.0").
        static QString formatNumber(double v)
        {
            return QString::number(v, 'g', 17);
        }

        double _value;
        GridUnitType _unitType;
    };
}
