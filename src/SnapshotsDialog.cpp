#include "SnapshotsDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include "SnapshotModel.h"
#include "SnapshotView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

SnapshotsDialog::SnapshotsDialog(MainWindow& mw)
    : QDialog{&mw}
{
    create_snapshot_dialog = new QDialog{this};
    create_snapshot_dialog->setWindowTitle(tr("Create Snapshot"));

    auto create_dialog_layout = new QVBoxLayout{};
    create_dialog_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    create_snapshot_dialog->setLayout(create_dialog_layout);

    auto create_form = new QFormLayout{};
    create_dialog_layout->addLayout(create_form);

    name_edit = new QLineEdit{};
    create_form->addRow(tr("Name:"), name_edit);

    league_combo = new QComboBox{};
    league_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    create_form->addRow(tr("League:"), league_combo);

    auto buttons = new QDialogButtonBox{};
    auto ok_button = buttons->addButton(QDialogButtonBox::Ok);
    connect(ok_button, &QPushButton::clicked, this, [this] {
        if (this->mw()->snapshots(game)->createSnapshot(name_edit->text(),
                                                        league_combo->currentText()))
            create_snapshot_dialog->accept();
        else
            QMessageBox::information(create_snapshot_dialog,
                                     tr("Snapshot creation failed"),
                                     tr("Failed to create snapshot."));
    });
    auto cancel_button = buttons->addButton(QDialogButtonBox::Cancel);
    connect(cancel_button, &QPushButton::clicked, create_snapshot_dialog, &QDialog::reject);
    create_dialog_layout->addWidget(buttons);

    auto layout = new QVBoxLayout{};
    layout->setHorizontalSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    view = new SnapshotView{*this, *mw.snapshots_poe1};
    layout->addWidget(view);

    auto settings = Settings::get();
    auto columns_state = settings.value(Settings::windows_snapshots_view_columns);
    if (columns_state.isValid())
        view->horizontalHeader()->restoreState(columns_state.toByteArray());

    auto size = settings.value(Settings::windows_snapshots_dialog_size);
    if (!size.isValid())
        resize(400, 400);
    else
        resize(size.toSize());
}

void SnapshotsDialog::openGame(Game game_)
{
    if (game_ == Game::Unknown)
        return;

    if (game != game_) {
        game = game_;

        auto& leagues = game == Game::Poe1 ? leagues_poe1 : leagues_poe2;
        if (leagues.empty()) {
            for (auto& league : mw()->exchangeCache(game)->cost_cache)
                leagues.push_back(league.first);
        }
        league_combo->clear();
        league_combo->addItems(leagues);
        league_combo->setCurrentText(Settings::currentLeague(game));

        view->setModel(mw()->snapshots(game));
        if (game == Game::Poe1)
            setWindowTitle(tr("PoE 1 Snapshots"));
        else
            setWindowTitle(tr("PoE 2 Snapshots"));
    }
    open();
}

void SnapshotsDialog::saveState(QSettings& settings) const
{
    settings.setValue(Settings::windows_snapshots_dialog_size, size());
    settings.setValue(Settings::windows_snapshots_view_columns,
                      view->horizontalHeader()->saveState());
}

MainWindow* SnapshotsDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

} // namespace planner
