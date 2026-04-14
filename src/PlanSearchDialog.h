#ifndef PLANSEARCHDIALOG_H
#define PLANSEARCHDIALOG_H

#include "Game.h"
#include <QDialog>

class QLineEdit;

namespace planner {
class PlanSearchView;

class PlanSearchDialog : public QDialog
{
    Q_OBJECT
public:
    PlanSearchDialog(QWidget* parent = nullptr);

    void openGame(Game game);

    PlanSearchView* view;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Game game{Game::Unknown};

    QLineEdit* filter_edit;

    QRect last_geometry;
};

} // namespace planner

#endif // PLANSEARCHDIALOG_H
