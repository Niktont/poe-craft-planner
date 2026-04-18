#include "InitializationDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include <QApplication>
#include <QCommandLineParser>

using namespace planner;
int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("CraftPlanner");
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QLocale::setDefault(QLocale::C);

    QApplication app{argc, argv};

    auto f = app.font();
    f.setFamily("Cambria");
    f.setPixelSize(14);
    app.setFont(f);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption offline_o{"offline"};
    parser.addOption(offline_o);

    parser.process(app);

    Settings::offline_mode = parser.isSet(offline_o);
    Settings::initCache();

    QObject parent;
    MainWindow mw{parent};

    auto init_dialog = new InitializationDialog{mw};
    init_dialog->open();

    return app.exec();
}
