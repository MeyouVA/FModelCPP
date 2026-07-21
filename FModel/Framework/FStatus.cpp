// Ported from FModel/Framework/FStatus.cs.
#include "FStatus.h"

namespace FModel::Framework
{
    FStatus::FStatus()
    {
        setStatus(EStatusKind::Loading);
    }

    void FStatus::setKind(EStatusKind value)
    {
        setProperty(_kind, value, QStringLiteral("Kind"));
        setIsReady(_kind != EStatusKind::Loading && _kind != EStatusKind::Stopping);
    }

    void FStatus::setStatus(EStatusKind kind, const QString& label)
    {
        setKind(kind);
        updateStatusLabel(label);
    }

    void FStatus::updateStatusLabel(const QString& label, const QString& prefix)
    {
        // Loading: "<prefix ?? Kind> <label>" trimmed; otherwise just the kind name.
        if (_kind == EStatusKind::Loading)
        {
            const QString p = prefix.isNull() ? statusKindName(_kind) : prefix;
            setLabel(QStringLiteral("%1 %2").arg(p, label).trimmed());
        }
        else
        {
            setLabel(statusKindName(_kind));
        }
    }
}
