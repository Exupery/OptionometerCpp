#include "ui/MainWindow.h"
#include "ui/DarkTheme.h"
#include "services/ScreenerService.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    qRegisterMetaType<ScreenerParams>("ScreenerParams");
    qRegisterMetaType<ScreenerResult>("ScreenerResult");

    QApplication app(argc, argv);
    app.setApplicationName("Optionometer");
    app.setOrganizationName("Optionometer");

    DarkTheme::apply(&app);

    MainWindow window;
    window.show();

    return app.exec();
}
