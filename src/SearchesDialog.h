#ifndef SEARCHESDIALOG_H
#define SEARCHESDIALOG_H

#include "Game.h"
#include <QDialog>

class QLineEdit;
class QSettings;

namespace planner {
class TradeRequestView;
class MainWindow;

class SearchesDialog : public QDialog
{
    Q_OBJECT
public:
    SearchesDialog(MainWindow& mw);

    void openGame(Game game);

    void saveState(QSettings& settings) const;

private:
    Game game{Game::Unknown};

    QLineEdit* filter_edit;
    TradeRequestView* request_view;

    MainWindow* mw() const;
};

} // namespace planner

#endif // SEARCHESDIALOG_H
