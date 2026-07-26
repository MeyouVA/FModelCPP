#pragma once
// Ported from FModel/ViewModels/Commands/CopyCommand.cs — copies one field of every selected asset to the
// clipboard, one per line.
//
// Deliberate differences from C#:
//   * C#'s `object parameter` is a two-element `object[]` produced by MultiParameterConverter: the trigger
//     string and the selected-items collection. Here it is a QVariantList of the same two elements, with the
//     collection a QVariantList of `GameFile*` (QVariant::fromValue). C# also accepts GameFileViewModel and
//     unwraps its .Asset; that view-model is not ported yet, so only the GameFile arm exists — the unwrap is
//     re-added with it.
//   * The line building is split out into buildText() so it can be tested without a clipboard. C# has no such
//     method; execute() is otherwise the same code in the same order.
//   * `sb.AppendLine()` writes Environment.NewLine, which on the only platform FModel ships for is "\r\n" —
//     kept verbatim, since the value goes straight to the Windows clipboard.

#include <QList>
#include <QString>
#include <QVariant>

#include "../../Framework/ViewModelCommand.h"
// ViewModelCommand holds a QPointer<TContextViewModel>, which needs the complete type — a forward
// declaration is not enough for the base-class instantiation. ApplicationViewModel.h only forward-declares
// the commands in return, so there is no cycle.
#include "../ApplicationViewModel.h"

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    namespace Commands
    {
        class CopyCommand : public Framework::ViewModelCommand<ApplicationViewModel>
        {
        public:
            explicit CopyCommand(ApplicationViewModel* contextViewModel, QObject* parent = nullptr)
                : ViewModelCommand(contextViewModel, parent) {}

            // Declaring the two-argument overload would otherwise hide Command's one-argument entry point,
            // which is what the UI actually calls.
            using ViewModelCommand::execute;
            void execute(ApplicationViewModel* contextViewModel, const QVariant& parameter) override;

            // The switch in C#'s Execute: one line per entry, already trimmed of its trailing newline.
            // An unrecognised trigger appends nothing and so yields an empty string, exactly as C#'s
            // switch with no matching case does.
            static QString buildText(const QString& trigger,
                                     const QList<CUE4Parse::FileProvider::Objects::GameFile*>& entries);
        };
    }
}
