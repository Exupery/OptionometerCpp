#include "DarkTheme.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace DarkTheme {

void apply(QApplication* app) {
    app->setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor(30, 30, 30));
    p.setColor(QPalette::WindowText, QColor(212, 212, 212));
    p.setColor(QPalette::Base, QColor(37, 37, 38));
    p.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    p.setColor(QPalette::ToolTipBase, QColor(50, 50, 50));
    p.setColor(QPalette::ToolTipText, QColor(212, 212, 212));
    p.setColor(QPalette::Text, QColor(212, 212, 212));
    p.setColor(QPalette::Button, QColor(45, 45, 48));
    p.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(42, 130, 218));
    p.setColor(QPalette::Highlight, QColor(0, 60, 204));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text,
        QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::ButtonText,
        QColor(128, 128, 128));
    app->setPalette(p);

    app->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #3f3f46;
        }
        QTabBar::tab {
            background: #2d2d30;
            color: #d4d4d4;
            padding: 6px 16px;
            border: 1px solid #3f3f46;
            border-bottom: none;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #1e1e1e;
            border-bottom: 2px solid #007acc;
        }
        QTabBar::tab:hover {
            background: #3e3e42;
        }
        QHeaderView::section {
            background: #2d2d30;
            color: #d4d4d4;
            padding: 4px 8px;
            border: 1px solid #3f3f46;
        }
        QTableView {
            gridline-color: #3f3f46;
            selection-background-color: #003CCC;
            selection-color: white;
        }
        QPushButton {
            padding: 6px 20px;
            border: 1px solid #3f3f46;
            border-radius: 3px;
        }
        QPushButton:hover {
            background: #3e3e42;
        }
        QPushButton:pressed {
            background: #007acc;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            padding: 4px 8px;
            border: 1px solid #3f3f46;
            border-radius: 3px;
            background: #2d2d30;
            color: #d4d4d4;
        }
        QGroupBox {
            border: 1px solid #3f3f46;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 16px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QStatusBar {
            background: #007acc;
            color: white;
        }
    )");
}

} // namespace DarkTheme
