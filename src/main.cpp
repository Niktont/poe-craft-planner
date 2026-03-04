#include "InitializationDialog.h"
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("CraftPlanner");
    QCoreApplication::setApplicationName("PoE Craft Planner");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QApplication app{argc, argv};

    auto f = app.font();
    f.setFamily("Cambria");
    f.setPixelSize(14);
    app.setFont(f);

    planner::MainWindow mw;

    auto init_dialog = new planner::InitializationDialog{mw};
    init_dialog->open();

    return app.exec();
}
