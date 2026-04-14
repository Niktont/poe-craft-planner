#include "SearchesDialog.h"
#include "AppState.h"
#include "Settings.h"
#include "TradeRequestView.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QVBoxLayout>

namespace planner {

SearchesDialog::SearchesDialog(QWidget* parent)
    : QDialog{parent}
{
    request_view = new TradeRequestView{};

    auto layout = new QVBoxLayout{};
    setLayout(layout);

    filter_edit = new QLineEdit{};
    filter_edit->setPlaceholderText(tr("Name filter"));
    filter_edit->setMinimumWidth(200);

    connect(filter_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto fm = filter_edit->fontMetrics();
        auto width = fm.horizontalAdvance(text) + 15;
        filter_edit->resize(width, filter_edit->height());

        request_view->filterName(text);
    });
    connect(filter_edit, &QLineEdit::returnPressed, this, [this] {
        request_view->filterName(filter_edit->text());
    });

    layout->addWidget(filter_edit, 0, Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(request_view);

    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_searches_dialog_geometry);
    if (!geometry.isValid())
        setGeometry(200, 200, 400, 400);
    else
        restoreGeometry(geometry.toByteArray());
}

void SearchesDialog::openGame(Game game_)
{
    if (game_ == Game::Unknown)
        return;

    if (game != game_) {
        game = game_;
        filter_edit->clear();
        request_view->setCache(*AppState::tradeCache(game_));
        if (game == Game::Poe1)
            setWindowTitle(tr("PoE 1 Searches"));
        else
            setWindowTitle(tr("PoE 2 Searches"));
    }
    open();
}

void SearchesDialog::saveState(QSettings& settings) const
{
    settings.setValue(Settings::windows_searches_dialog_geometry, saveGeometry());
    settings.setValue(Settings::windows_searches_view_columns,
                      request_view->horizontalHeader()->saveState());
}

} // namespace planner
