#ifndef SNAPSHOTSDIALOG_H
#define SNAPSHOTSDIALOG_H

#include "Game.h"
#include <QDialog>

class QSettings;
class QLineEdit;
class QComboBox;

namespace planner {
class MainWindow;
class SnapshotView;

class SnapshotsDialog : public QDialog
{
    Q_OBJECT
public:
    SnapshotsDialog(MainWindow& mw);

    void openGame(Game game);

    void saveState(QSettings& settings) const;
    QDialog* create_snapshot_dialog;

private:
    Game game{Game::Unknown};

    QLineEdit* name_edit;

    QComboBox* league_combo;
    QStringList leagues_poe1;
    QStringList leagues_poe2;

    SnapshotView* view;

    MainWindow* mw() const;
};

} // namespace planner

#endif // SNAPSHOTSDIALOG_H
