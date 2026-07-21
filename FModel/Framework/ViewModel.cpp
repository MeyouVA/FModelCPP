// Ported from FModel/Framework/ViewModel.cs.
#include "ViewModel.h"

namespace FModel::Framework
{
    // Matches C#'s string.Join(Environment.NewLine, ...). On Windows Environment.NewLine is "\r\n".
    static const QString kNewLine = QStringLiteral("\r\n");

    QString ViewModel::operator[](const QString& propertyName) const
    {
        if (propertyName.isEmpty())
            return error();

        auto it = _validationErrors.constFind(propertyName);
        return it != _validationErrors.constEnd() ? it->join(kNewLine) : QString();
    }

    QString ViewModel::error() const
    {
        return getAllErrors().join(kNewLine);
    }

    QStringList ViewModel::getErrors(const QString& propertyName) const
    {
        if (propertyName.isEmpty())
        {
            QStringList all;
            for (const auto& list : _validationErrors)
                all += list;
            return all;
        }

        auto it = _validationErrors.constFind(propertyName);
        return it != _validationErrors.constEnd() ? *it : QStringList();
    }

    QStringList ViewModel::getAllErrors() const
    {
        QStringList all;
        for (const auto& list : _validationErrors)
            for (const auto& e : list)
                if (!e.isEmpty())
                    all += e;
        return all;
    }

    void ViewModel::addValidationError(const QString& propertyName, const QString& errorMessage)
    {
        _validationErrors[propertyName].append(errorMessage);
    }

    void ViewModel::clearValidationErrors(const QString& propertyName)
    {
        _validationErrors.remove(propertyName);
    }
}
