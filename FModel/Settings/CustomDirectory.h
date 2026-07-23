#pragma once
// Ported from FModel/Settings/CustomDirectory.cs.

#include "../Framework/ViewModel.h"

#include <QList>
#include <QString>

class QJsonObject;

namespace FModel::Settings
{
    class CustomDirectory : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // The per-game shortcut list seeded into a directory's settings the first time it is added.
        // Returns owned objects; the caller (DirectorySettings) takes ownership.
        static QList<CustomDirectory*> Default(const QString& gameName);

        explicit CustomDirectory(QObject* parent = nullptr) : ViewModel(parent) {}
        CustomDirectory(QString header, QString path, QObject* parent = nullptr)
            : ViewModel(parent), _header(std::move(header)), _directoryPath(std::move(path)) {}

        const QString& header() const { return _header; }
        void setHeader(const QString& value) { setProperty(_header, value, QStringLiteral("Header")); }

        const QString& directoryPath() const { return _directoryPath; }
        void setDirectoryPath(const QString& value) { setProperty(_directoryPath, value, QStringLiteral("DirectoryPath")); }

        QString toString() const { return _header; }

        QJsonObject toJson() const;
        static CustomDirectory* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    private:
        QString _header;
        QString _directoryPath;
    };
}
