#include "DataUpdater.h"
#include <QCommandLineParser>
#include <QCoreApplication>

using namespace planner;

int main(int argc, char* argv[])
{
    QCoreApplication app{argc, argv};

    QCommandLineParser parser;
    parser.addPositionalArgument("game", "Game for updating", "poe1|poe2");
    parser.addPositionalArgument("data", "Data type", "static|filters|stats");
    parser.parse(app.arguments());

    DataUpdater updater;
    auto game = parser.positionalArguments().at(0);
    updater.game = game == "poe1" ? DataUpdater::Poe1 : DataUpdater::Poe2;
    auto data = parser.positionalArguments().at(1);
    if (data == "static")
        updater.data_type = DataUpdater::Static;
    else if (data == "filters")
        updater.data_type = DataUpdater::Filters;
    else
        updater.data_type = DataUpdater::Stats;

    updater.getData();

    return app.exec();
}
