#ifndef SEARCHESDIALOG_H
#define SEARCHESDIALOG_H

#include "Game.h"
#include <QDialog>

class QLineEdit;
class QSettings;

namespace planner {
class TradeRequestView;

class SearchesDialog : public QDialog
{
    Q_OBJECT
public:
    SearchesDialog(QWidget* parent = nullptr);

    void openGame(Game game);

    void saveState(QSettings& settings) const;

private:
    Game game{Game::Unknown};

    QLineEdit* filter_edit;
    TradeRequestView* request_view;
};

} // namespace planner

#endif // SEARCHESDIALOG_H
