#ifndef PLANSEARCHVIEW_H
#define PLANSEARCHVIEW_H

#include "Game.h"
#include <QTableView>

namespace planner {
class MainWindow;
class PlanSearchModel;

class PlanSearchView : public QTableView
{
    Q_OBJECT
public:
    PlanSearchView(MainWindow& mw, QWidget* parent = nullptr);

    void setGame(Game game);

public slots:
    void filterName(const QString& filter_str);

private slots:
    void indexClicked(const QModelIndex& idx);

private:
    MainWindow& mw;

    Game game{Game::Unknown};
    PlanSearchModel* search_model{};
};

} // namespace planner

#endif // PLANSEARCHVIEW_H
