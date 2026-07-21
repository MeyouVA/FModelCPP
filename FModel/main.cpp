// Ported from FModel/App.xaml(.cs) — the application entry point (minimal bootstrap for now).
// App.xaml.cs does a lot more (single-instance mutex, Serilog, culture, crash handlers, DI-ish service wiring);
// those arrive with their layers. For the shell slice this just creates the QApplication and shows MainWindow.
#include <QApplication>

#include "MainWindow.h"
#include "Theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FModel"));
    QApplication::setOrganizationName(QStringLiteral("FModel"));

    FModel::applyTheme(app);

    FModel::MainWindow window;
    window.show();

    return QApplication::exec();
}
