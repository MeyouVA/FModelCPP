// Ported from FModel's App.xaml theme (AdonisUI Dark + FModel accent overrides). See Theme.h.
#include "Theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace FModel
{
    namespace
    {
        // --- AdonisUI Dark color scheme (exact values), with FModel's App.xaml overrides. ---
        namespace C
        {
            // Layer backgrounds: 0 = window, 2 = panels/group boxes, 3 = dark insets (lists, inputs, explorer).
            constexpr auto Layer0Bg = "#2A2B34";
            constexpr auto Layer1Bg = "#3D3D4C";
            constexpr auto Layer2Bg = "#32323F";
            constexpr auto Layer3Bg = "#262630";
            constexpr auto Layer4Bg = "#323341";
            constexpr auto Layer1Border = "#4A4A5E";
            constexpr auto Layer2Border = "#272730";
            constexpr auto Layer3Border = "#1F2029";
            constexpr auto Layer4Border = "#414255";
            constexpr auto Foreground = "#F0F0F0";
            constexpr auto DisabledForeground = "#8E8E8E";
            constexpr auto AccentForeground = "#FFFFFF";
            // FModel App.xaml overrides:
            constexpr auto Accent = "#206BD4";            // AccentColor
            constexpr auto AccentInteraction = "#3D7FE0";  // hover (a lighter accent)
            constexpr auto Alert = "#D49220";             // AlertColor
            constexpr auto Error = "#C22B2B";             // ErrorColor
        }
    }

    void applyTheme(QApplication& app)
    {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

        // Base palette so any natively drawn / unstyled widgets are dark too.
        QPalette p;
        p.setColor(QPalette::Window, QColor(C::Layer0Bg));
        p.setColor(QPalette::WindowText, QColor(C::Foreground));
        p.setColor(QPalette::Base, QColor(C::Layer3Bg));
        p.setColor(QPalette::AlternateBase, QColor(C::Layer2Bg));
        p.setColor(QPalette::Text, QColor(C::Foreground));
        p.setColor(QPalette::Button, QColor(C::Layer1Bg));
        p.setColor(QPalette::ButtonText, QColor(C::Foreground));
        p.setColor(QPalette::ToolTipBase, QColor(C::Layer2Bg));
        p.setColor(QPalette::ToolTipText, QColor(C::Foreground));
        p.setColor(QPalette::Highlight, QColor(C::Accent));
        p.setColor(QPalette::HighlightedText, QColor(C::AccentForeground));
        p.setColor(QPalette::Link, QColor("#3377C6"));
        p.setColor(QPalette::PlaceholderText, QColor(C::DisabledForeground));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(C::DisabledForeground));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(C::DisabledForeground));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(C::DisabledForeground));
        app.setPalette(p);

        // Stylesheet for the specific widget look (borders, layered backgrounds, accent selection/hover).
        const QString qss = QStringLiteral(R"QSS(
            QMainWindow, QDialog, QWidget { background-color: %(l0)s; color: %(fg)s; }

            QMenuBar { background-color: %(l0)s; border: none; }
            QMenuBar::item { padding: 5px 10px; background: transparent; }
            QMenuBar::item:selected, QMenuBar::item:pressed { background-color: %(accent)s; color: %(afg)s; }
            QMenu { background-color: %(l2)s; border: 1px solid %(l2b)s; padding: 3px; }
            QMenu::item { padding: 5px 26px 5px 22px; border-radius: 2px; }
            QMenu::item:selected { background-color: %(accent)s; color: %(afg)s; }
            QMenu::separator { height: 1px; background: %(l2b)s; margin: 4px 6px; }

            QTabWidget::pane { border: 1px solid %(l2b)s; background-color: %(l2)s; }
            QTabBar::tab { background-color: %(l0)s; color: %(fg)s; padding: 6px 16px;
                           border: 1px solid %(l2b)s; border-bottom: none; }
            QTabBar::tab:selected { background-color: %(l2)s; }
            QTabBar::tab:hover:!selected { background-color: %(l1)s; }
            QTabBar::tab:!selected { margin-top: 2px; }

            QGroupBox { border: 1px solid %(l2b)s; border-radius: 2px; margin-top: 10px; background-color: %(l2)s; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;
                               left: 8px; padding: 0 4px; color: %(fg)s; }

            QListWidget, QListView, QTreeWidget, QTreeView, QTextEdit, QPlainTextEdit {
                background-color: %(l3)s; border: 1px solid %(l3b)s; color: %(fg)s; }
            QListWidget::item, QTreeWidget::item { padding: 2px; }
            QListWidget::item:hover, QTreeView::item:hover { background-color: %(l1)s; }
            QListWidget::item:selected, QTreeView::item:selected,
            QListWidget::item:selected:active, QTreeView::item:selected:active {
                background-color: %(accent)s; color: %(afg)s; }

            QLineEdit, QComboBox, QSpinBox, QAbstractSpinBox {
                background-color: %(l3)s; border: 1px solid %(l3b)s; padding: 4px; color: %(fg)s; }
            QLineEdit:focus, QComboBox:focus { border: 1px solid %(accent)s; }
            QComboBox::drop-down { border: none; width: 18px; }
            QComboBox QAbstractItemView { background-color: %(l2)s; border: 1px solid %(l2b)s;
                                          selection-background-color: %(accent)s; selection-color: %(afg)s; }

            QPushButton { background-color: %(l1)s; border: 1px solid %(l1b)s; border-radius: 2px;
                          padding: 5px 12px; color: %(fg)s; }
            QPushButton:hover { background-color: %(accenti)s; border-color: %(accent)s; }
            QPushButton:pressed { background-color: %(accent)s; color: %(afg)s; }
            QPushButton:disabled { color: %(dfg)s; border-color: %(l2b)s; }

            QCheckBox { spacing: 6px; }
            QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid %(l1b)s;
                                   border-radius: 2px; background: %(l3)s; }
            QCheckBox::indicator:checked { background: %(accent)s; border-color: %(accent)s; }

            QSplitter::handle { background-color: %(l0)s; }
            QSplitter::handle:horizontal { width: 4px; }
            QSplitter::handle:vertical { height: 4px; }

            QStatusBar { background-color: %(accent)s; color: %(afg)s; }
            QStatusBar::item { border: none; }
            QStatusBar QLabel { color: %(afg)s; background: transparent; }

            QScrollBar:vertical { background: %(l3)s; width: 12px; margin: 0; }
            QScrollBar::handle:vertical { background: %(l1)s; min-height: 24px; border-radius: 3px; }
            QScrollBar::handle:vertical:hover { background: %(l1b)s; }
            QScrollBar:horizontal { background: %(l3)s; height: 12px; margin: 0; }
            QScrollBar::handle:horizontal { background: %(l1)s; min-width: 24px; border-radius: 3px; }
            QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
            QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

            QToolTip { background-color: %(l2)s; color: %(fg)s; border: 1px solid %(l4b)s; padding: 3px; }
        )QSS")
            .replace(QStringLiteral("%(l0)s"), QString::fromLatin1(C::Layer0Bg))
            .replace(QStringLiteral("%(l1)s"), QString::fromLatin1(C::Layer1Bg))
            .replace(QStringLiteral("%(l2)s"), QString::fromLatin1(C::Layer2Bg))
            .replace(QStringLiteral("%(l3)s"), QString::fromLatin1(C::Layer3Bg))
            .replace(QStringLiteral("%(l1b)s"), QString::fromLatin1(C::Layer1Border))
            .replace(QStringLiteral("%(l2b)s"), QString::fromLatin1(C::Layer2Border))
            .replace(QStringLiteral("%(l3b)s"), QString::fromLatin1(C::Layer3Border))
            .replace(QStringLiteral("%(l4b)s"), QString::fromLatin1(C::Layer4Border))
            .replace(QStringLiteral("%(fg)s"), QString::fromLatin1(C::Foreground))
            .replace(QStringLiteral("%(dfg)s"), QString::fromLatin1(C::DisabledForeground))
            .replace(QStringLiteral("%(afg)s"), QString::fromLatin1(C::AccentForeground))
            .replace(QStringLiteral("%(accenti)s"), QString::fromLatin1(C::AccentInteraction))
            .replace(QStringLiteral("%(accent)s"), QString::fromLatin1(C::Accent));

        app.setStyleSheet(qss);
    }
}
