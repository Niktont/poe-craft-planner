#ifndef PLANSEARCHVIEW_H
#define PLANSEARCHVIEW_H

#include "Game.h"
#include <QTableView>

namespace planner {
class PlanSearchModel;

class PlanSearchView : public QTableView
{
    Q_OBJECT
public:
    PlanSearchView(QWidget* parent = nullptr);

    void setGame(Game game);

public slots:
    void filterName(const QString& filter_str);

signals:
    void planClicked(const QUuid& plan_id, planner::Game game, bool need_window) const;

protected:
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void indexClicked(const QModelIndex& idx);

private:
    Game game{Game::Unknown};
    PlanSearchModel* search_model{};
};

} // namespace planner

#endif // PLANSEARCHVIEW_H
