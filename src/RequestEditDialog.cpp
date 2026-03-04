#include "RequestEditDialog.h"
#include "MainWindow.h"
#include "TradeRequestCache.h"
#include <QAbstractProxyModel>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCompleter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace planner {

RequestEditDialog::RequestEditDialog(MainWindow& mw)
    : QDialog{&mw}
{
    setWindowTitle(tr("Manage Searches"));
    setWindowModality(Qt::WindowModal);

    auto main_layot = new QVBoxLayout{};
    setLayout(main_layot);
    setMinimumWidth(550);
    load_label = new QLabel{tr("Loading query...")};
    main_layot->addWidget(load_label);
    load_label->hide();

    auto button_layout = new QHBoxLayout{};
    main_layot->addLayout(button_layout);

    load_button = new QPushButton{tr("Load query")};
    connect(load_button, &QPushButton::clicked, this, &RequestEditDialog::loadQuery);
    button_layout->addWidget(load_button);
    load_button->setAutoDefault(false);

    save_button = new QPushButton{tr("Save")};
    connect(save_button, &QPushButton::clicked, this, &RequestEditDialog::saveRequest);
    button_layout->addWidget(save_button);
    save_button->setAutoDefault(false);

    button_layout->addStretch(1);

    auto label = new QLabel{tr("Search name:")};
    layout()->addWidget(label);

    name_edit = new QLineEdit{};
    name_edit->setMaxLength(40);
    name_edit->setPlaceholderText(tr("Search requests are unique, names aren't"));
    connect(name_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkName);
    layout()->addWidget(name_edit);

    label = new QLabel{tr("Search link:")};
    layout()->addWidget(label);

    link_edit = new QLineEdit{};
    connect(link_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkLink);
    layout()->addWidget(link_edit);

    label = new QLabel{tr("Query:")};
    layout()->addWidget(label);

    query_edit = new QLineEdit{};
    connect(query_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkQuery);
    layout()->addWidget(query_edit);

    main_layot->addStretch();

    connect(this, &QDialog::finished, this, &RequestEditDialog::cleanup);
    setFocusPolicy(Qt::ClickFocus);
}

void RequestEditDialog::openGame(Game game_)
{
    if (game_ >= Game::Unknown)
        return;

    if (game != game_) {
        load_button->setEnabled(false);
        save_button->setEnabled(false);
        is_name_valid = false;
        is_link_valid = false;
        is_query_valid = false;
        name_edit->clear();
        link_edit->clear();
        query_edit->clear();
        edit_request = {};
        edit_query = {};
    }
    setGame(game_);

    open();
}

void RequestEditDialog::openRequest(const TradeRequestKey& request, Game game)
{
    if (game >= Game::Unknown)
        return;

    load_button->setEnabled(false);
    save_button->setEnabled(false);
    is_name_valid = false;
    is_link_valid = false;
    is_query_valid = false;

    edit_request = request;
    setGame(game);

    if (edit_request.isValid()) {
        link_edit->setText(edit_request.toUrl(game));
        is_link_valid = true;
        if (auto it = cache->requestData(edit_request); it != cache->cache.end()) {
            name_edit->setText(it->second.name());
            is_name_valid = !it->second.name().isEmpty();

            edit_query = it->second.query();
            query_edit->setText(edit_query.toJson(QJsonDocument::Compact));
            is_query_valid = !it->second.query().isEmpty();
        } else {
            name_edit->clear();
            query_edit->clear();
            edit_query = {};
        }
    } else {
        name_edit->clear();
        link_edit->clear();
        query_edit->clear();
        edit_query = {};
    }

    open();
}

void RequestEditDialog::closeEvent(QCloseEvent* event)
{
    cleanup();
    event->accept();
}

void RequestEditDialog::setGame(Game game_)
{
    game = game_;
    if (game == Game::Poe1) {
        cache = mw()->trade_cache_poe1;
        link_edit->setPlaceholderText("https://pathofexile.com/trade/search/[league]/EBo4ajr4S5");
    } else {
        cache = mw()->trade_cache_poe2;
        link_edit->setPlaceholderText(
            "https://pathofexile.com/trade2/search/poe2/[league]/7o6gMy2h5");
    }

    name_edit->setCompleter(cache->completer);
    connect(name_edit->completer(),
            qOverload<const QModelIndex&>(&QCompleter::activated),
            this,
            &RequestEditDialog::selectRequest);
}

void RequestEditDialog::checkName()
{
    auto name = name_edit->text().trimmed();
    if (name.isEmpty()) {
        save_button->setEnabled(false);
        is_name_valid = false;
    } else {
        is_name_valid = true;
        if (is_link_valid && is_query_valid)
            save_button->setEnabled(true);
    }
}

void RequestEditDialog::checkLink()
{
    auto link = link_edit->text().trimmed();
    auto res = TradeRequestKey::fromUrl(link, game);
    if (!res) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Invalid Link"));
        msg.addButton(QMessageBox::Ok);
        if (res.error() == TradeRequestKey::ParseError::GameMismatch) {
            if (game == Game::Poe1)
                msg.setText(tr("This link is not for PoE 1."));
            else
                msg.setText(tr("This link is not for PoE 2."));
        } else {
            msg.setText(tr("Failed to parse link."));
        }

        msg.exec();
        load_button->setEnabled(false);
        save_button->setEnabled(false);
        is_link_valid = false;
        return;
    }
    auto& new_request = res.assume_value();
    edit_request = new_request;
    is_link_valid = true;
    load_button->setEnabled(true);
    if (is_name_valid && is_query_valid)
        save_button->setEnabled(true);
}

void RequestEditDialog::loadQuery()
{
    if (!edit_request.isValid())
        return;
    if (!is_query_valid) {
        if (auto it = cache->requestData(edit_request);
            it != cache->cache.end() && !it->second.query().isEmpty()) {
            edit_query = it->second.query();
            is_query_valid = true;
            query_edit->setText(edit_query.toJson(QJsonDocument::Compact));
            if (is_name_valid)
                save_button->setEnabled(true);
            return;
        }
    }

    load_button->setEnabled(false);
    link_edit->setEnabled(false);
    load_label->show();

    auto web_view = mw()->web_view;
    web_view->load(edit_request.toUrl(game));
    connect(
        web_view,
        &QWebEngineView::loadFinished,
        this,
        [this, web_view, request = edit_request] {
            web_view->page()->toHtml([this, request](const QString& html) {
                load_label->hide();
                if (request == edit_request)
                    findQuery(html);
                else
                    link_edit->setEnabled(true);
            });
        },
        Qt::SingleShotConnection);
}

void RequestEditDialog::checkQuery()
{
    QJsonDocument query = QJsonDocument::fromJson(query_edit->text().toUtf8());
    if (query.isNull()) {
        save_button->setEnabled(false);
        is_query_valid = false;
        return;
    }
    edit_query = query;
    is_query_valid = true;
    if (is_name_valid && is_link_valid)
        save_button->setEnabled(true);
}

void RequestEditDialog::selectRequest(const QModelIndex& proxy_i)
{
    auto proxy_m = static_cast<QAbstractProxyModel*>(name_edit->completer()->completionModel());
    auto index = proxy_m->mapToSource(proxy_i);

    auto it = cache->cache.nth(index.row());
    edit_request = it->first;
    link_edit->setText(edit_request.toUrl(game));

    edit_query = it->second.query();
    query_edit->setText(edit_query.toJson(QJsonDocument::Compact));

    is_name_valid = true;
    is_link_valid = true;
    is_query_valid = !it->second.query().isEmpty();
    load_button->setEnabled(!is_query_valid);
    save_button->setEnabled(false);
}

void RequestEditDialog::findQuery(const QString& html)
{
    auto pos = html.lastIndexOf("[\"trade\"]");
    if (pos == -1) {
        queryLoadFailed();
        return;
    }
    pos = html.indexOf("t({", pos);
    if (pos == -1) {
        queryLoadFailed();
        return;
    }
    pos += 2;
    auto end_pos = html.indexOf(");", pos);
    if (end_pos == -1) {
        queryLoadFailed();
        return;
    }
    QStringView json_str{html.begin() + pos, html.begin() + end_pos};
    QJsonDocument json{QJsonDocument::fromJson(json_str.toUtf8())};
    auto json_o{json.object()};
    QJsonObject state_o{json_o["state"].toObject()};
    if (state_o.empty()) {
        queryLoadFailed();
        return;
    }

    auto status = state_o["status"].toString();
    if (status.isEmpty()) {
        queryLoadFailed();
        return;
    }

    QJsonObject status_o;
    status_o["option"] = status;
    state_o["status"] = status_o;

    QJsonObject sort_o;
    sort_o["price"] = "asc";
    json_o = {};
    json_o["query"] = state_o;
    json_o["sort"] = sort_o;
    json.setObject(json_o);

    edit_query = json;
    query_edit->setText(edit_query.toJson(QJsonDocument::Compact));

    load_button->setEnabled(false);
    link_edit->setEnabled(true);
    is_query_valid = true;
    if (is_name_valid && is_link_valid)
        save_button->setEnabled(true);
}

void RequestEditDialog::saveRequest()
{
    if (!is_name_valid || !is_link_valid || !is_query_valid)
        return;

    cache->saveRequest(edit_request, {name_edit->text().trimmed(), edit_query});
    save_button->setEnabled(false);
}

void RequestEditDialog::cleanup()
{
    disconnect(name_edit->completer(),
               qOverload<const QModelIndex&>(&QCompleter::activated),
               this,
               &RequestEditDialog::selectRequest);
}

void RequestEditDialog::queryLoadFailed()
{
    QMessageBox msg;
    msg.setWindowTitle(tr("Query Loading Failed"));
    msg.addButton(QMessageBox::Ok);
    msg.setText(tr("Failed to load query. Trade website don't load query without logging in. "
                   "Consider input query manually."));
    msg.exec();

    load_button->setEnabled(true);
    link_edit->setEnabled(true);
}

MainWindow* RequestEditDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

} // namespace planner
