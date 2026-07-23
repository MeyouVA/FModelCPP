#pragma once
// Ported from FModel/Framework/FStatus.cs — the application's status view-model.
//
// Tracks the current EStatusKind and a human-readable Label, and exposes IsReady (true unless the app is
// Loading or Stopping). In the WPF UI this drives the status bar; it derives from ViewModel so its three
// properties raise change notifications. The setters are private (as in C#): status changes go through
// SetStatus / UpdateStatusLabel.

#include <QString>

#include "ViewModel.h"
#include "../Enums.h"

namespace FModel::Framework
{
    class FStatus : public ViewModel
    {
        Q_OBJECT

    public:
        // C#'s ctor takes no arguments; the optional parent is the Qt ownership hook ApplicationViewModel uses.
        explicit FStatus(QObject* parent = nullptr);

        bool isReady() const { return _isReady; }
        EStatusKind kind() const { return _kind; }
        QString label() const { return _label; }

        void setStatus(EStatusKind kind, const QString& label = QString());
        void updateStatusLabel(const QString& label, const QString& prefix = QString());

    private:
        void setIsReady(bool value) { setProperty(_isReady, value, QStringLiteral("IsReady")); }
        void setKind(EStatusKind value);
        void setLabel(const QString& value) { setProperty(_label, value, QStringLiteral("Label")); }

        bool _isReady = false;
        EStatusKind _kind = EStatusKind::Ready;
        QString _label;
    };
}
