// Ported from FModel's App.xaml theme setup: the AdonisUI "Dark" color scheme with FModel's accent overrides.
// AdonisUI is a WPF resource-dictionary theme with no Qt equivalent, so its palette is reproduced here as a Qt
// Fusion QPalette + a global stylesheet. Colors are the exact AdonisUI Dark values; AccentColor/AlertColor/
// ErrorColor use FModel's App.xaml overrides (#206BD4 / #D49220 / #C22B2B).
#pragma once

class QApplication;

namespace FModel
{
    // Applies the FModel dark theme (Fusion style + palette + stylesheet) to the whole application.
    void applyTheme(QApplication& app);
}
