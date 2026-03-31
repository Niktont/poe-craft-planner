#ifndef PLANSEARCHDIALOG_H
#define PLANSEARCHDIALOG_H

#include "Game.h"
#include <QDialog>

class QLineEdit;

namespace planner {
class MainWindow;
class PlanSearchView;

class PlanSearchDialog : public QDialog
{
    Q_OBJECT
public:
    PlanSearchDialog(MainWindow& mw);

    void openGame(Game game);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Game game{Game::Unknown};

    QLineEdit* filter_edit;
    PlanSearchView* view;

    QRect last_geometry;

    MainWindow& mw() const;
};

} // namespace planner

#endif // PLANSEARCHDIALOG_H
