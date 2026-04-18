#include "PlanSearchDialog.h"
#include "PlanSearchView.h"
#include "Settings.h"
#include <QLineEdit>
#include <QVBoxLayout>

namespace planner {

PlanSearchDialog::PlanSearchDialog(QWidget* parent)
    : QDialog{parent}
    , view{new PlanSearchView{this}}
    , filter_edit{new QLineEdit{this}}
{
    auto layout = new QVBoxLayout{};
    setLayout(layout);

    filter_edit->setMinimumWidth(200);
    connect(filter_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        view->filterName(text);
    });
    connect(filter_edit, &QLineEdit::returnPressed, this, [this] {
        view->filterName(filter_edit->text());
    });

    layout->addWidget(filter_edit);
    layout->addWidget(view);

    auto settings = Settings::get();
    auto size = settings.value(Settings::windows_plan_search_dialog_size);
    if (size.isValid())
        resize(size.value<QSize>());
}

void PlanSearchDialog::openGame(Game game_)
{
    if (game_ == Game::Unknown)
        return;

    if (game != game_) {
        game = game_;
        filter_edit->clear();
        view->setGame(game);
        if (game == Game::Poe1)
            setWindowTitle(tr("PoE 1 Plans"));
        else
            setWindowTitle(tr("PoE 2 Plans"));
    }

    if (!last_geometry.isNull())
        setGeometry(last_geometry);
    show();
}

void PlanSearchDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
    last_geometry = geometry();
}

} // namespace planner
