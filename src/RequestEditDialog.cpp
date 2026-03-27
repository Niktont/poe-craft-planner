#include "RequestEditDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include "TradeRequestCache.h"
#include <QAbstractProxyModel>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
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
    setWindowTitle(tr("Edit Search"));
    setWindowModality(Qt::WindowModal);

    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);

    auto form = new QFormLayout{};
    main_layout->addLayout(form);

    name_edit = new QLineEdit{};
    name_edit->setMaxLength(40);
    name_edit->setPlaceholderText(tr("Search requests are unique, names aren't"));
    connect(name_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkName);
    form->addRow(tr("Name:"), name_edit);

    link_edit = new QLineEdit{};
    connect(link_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkLink);
    form->addRow(tr("Request link:"), link_edit);

    auto query_layout = new QHBoxLayout{};
    query_layout->setContentsMargins(0, 0, 0, 0);
    form->addRow(tr("Query:"), query_layout);

    query_cb = new QCheckBox{};
    query_cb->setEnabled(false);
    query_layout->addWidget(query_cb);

    paste_button = new QPushButton{tr("Paste")};
    connect(paste_button, &QPushButton::clicked, this, &RequestEditDialog::checkQuery);
    query_layout->addWidget(paste_button);

    load_button = new QPushButton{tr("Load")};
    connect(load_button, &QPushButton::clicked, this, &RequestEditDialog::loadQuery);
    load_button->setToolTip(tr("If you are adding a lot of searches, load one every 5+ seconds to "
                               "not exceed rate limits."));
    query_layout->addWidget(load_button);
    query_layout->addStretch(1);

    regex_edit = new QLineEdit{};
    connect(regex_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkChange);
    form->addRow(tr("Regex:"), regex_edit);

    description_edit = new QLineEdit{};
    connect(description_edit, &QLineEdit::editingFinished, this, &RequestEditDialog::checkChange);
    form->addRow(tr("Description:"), description_edit);

    auto buttons = new QDialogButtonBox{};
    ok_button = buttons->addButton(QDialogButtonBox::Ok);
    connect(ok_button, &QPushButton::clicked, this, [this] {
        saveRequest();
        accept();
    });

    save_button = buttons->addButton(QDialogButtonBox::Save);
    connect(save_button, &QPushButton::clicked, this, &RequestEditDialog::saveRequest);

    cancel_button = buttons->addButton(QDialogButtonBox::Cancel);
    connect(cancel_button, &QPushButton::clicked, this, &RequestEditDialog::reject);

    main_layout->addWidget(buttons);

    main_layout->addStretch();

    paste_button->setAutoDefault(false);
    load_button->setAutoDefault(false);
    ok_button->setAutoDefault(false);
    save_button->setAutoDefault(false);
    cancel_button->setAutoDefault(false);

    connect(this, &QDialog::finished, this, &RequestEditDialog::cleanup);
    setFocusPolicy(Qt::ClickFocus);

    auto settings = Settings::get();
    auto size = settings.value(Settings::windows_request_edit_size);
    if (size.isValid())
        resize(size.value<QSize>());
    else
        resize(620, 200);
}

void RequestEditDialog::openGame(Game game_, bool need_clear)
{
    if (game_ >= Game::Unknown)
        return;

    if (need_clear || game != game_)
        clear();

    setGame(game_);

    open();
}

void RequestEditDialog::openRequest(Game game, const TradeRequestKey& request)
{
    if (game >= Game::Unknown)
        return;

    clear();

    edit_request = request;
    setGame(game);

    if (edit_request.isValid()) {
        is_link_valid = true;
        link_edit->setText(edit_request.toUrl(game));
        if (auto it = cache->requestData(edit_request); it != cache->cache.end()) {
            is_name_valid = !it->second.name().isEmpty();
            setQueryValid(!it->second.query().isEmpty());

            name_edit->setText(it->second.name());
            edit_query = it->second.query();
            regex_edit->setText(it->second.regex());
            description_edit->setText(it->second.description().text);

            load_button->setEnabled(!is_query_valid);
        }
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
        link_edit->setPlaceholderText(
            "https://www.pathofexile.com/trade/search/[league]/EBo4ajr4S5");
    } else {
        cache = mw()->trade_cache_poe2;
        link_edit->setPlaceholderText(
            "https://www.pathofexile.com/trade2/search/poe2/[league]/7o6gMy2h5");
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
        is_name_valid = false;
        enableSave(false);
    } else {
        is_name_valid = true;
        if (is_link_valid && is_query_valid)
            enableSave(true);
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

        is_link_valid = false;
        load_button->setEnabled(false);
        enableSave(false);
        return;
    }

    auto& new_request = res.assume_value();
    edit_request = new_request;
    is_link_valid = true;
    load_button->setEnabled(true);

    if (auto it = cache->requestData(edit_request); it != cache->cache.end()) {
        edit_query = it->second.query();
        setQueryValid(!edit_query.isEmpty());
    }

    if (is_name_valid && is_query_valid)
        enableSave(true);
}

void RequestEditDialog::loadQuery()
{
    if (!edit_request.isValid())
        return;
    if (!is_query_valid) {
        if (auto it = cache->requestData(edit_request);
            it != cache->cache.end() && !it->second.query().isEmpty()) {
            setQueryValid(true);
            edit_query = it->second.query();

            if (is_name_valid)
                enableSave(true);
            return;
        }
    }

    load_button->setEnabled(false);
    link_edit->setEnabled(false);

    auto web_view = mw()->web_view;
    web_view->load(edit_request.toUrl(game));
    connect(
        web_view,
        &QWebEngineView::loadFinished,
        this,
        [this, web_view, request = edit_request] {
            web_view->page()->toHtml([this, request](const QString& html) {
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
    QJsonDocument query = QJsonDocument::fromJson(qApp->clipboard()->text().toUtf8());
    if (query.isNull()) {
        enableSave(false);
        setQueryValid(false);
        return;
    }

    edit_query = query;
    setQueryValid(true);

    if (is_name_valid && is_link_valid)
        enableSave(true);
}

void RequestEditDialog::checkChange()
{
    if (is_name_valid && is_link_valid && is_query_valid)
        enableSave(true);
}

void RequestEditDialog::selectRequest(const QModelIndex& proxy_idx)
{
    auto proxy_m = static_cast<QAbstractProxyModel*>(name_edit->completer()->completionModel());
    auto index = proxy_m->mapToSource(proxy_idx);

    auto it = cache->cache.nth(index.row());
    is_name_valid = true;
    is_link_valid = true;
    setQueryValid(!it->second.query().isEmpty());

    edit_request = it->first;
    link_edit->setText(edit_request.toUrl(game));

    edit_query = it->second.query();
    regex_edit->setText(it->second.regex());
    description_edit->setText(it->second.description().text);

    load_button->setEnabled(!is_query_valid);
    enableSave(false);
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

    setQueryValid(true);
    edit_query = json;

    load_button->setEnabled(false);
    link_edit->setEnabled(true);

    if (is_name_valid && is_link_valid)
        enableSave(true);
}

void RequestEditDialog::saveRequest()
{
    if (!is_name_valid || !is_link_valid || !is_query_valid)
        return;

    cache->saveRequest(edit_request,
                       name_edit->text().trimmed(),
                       edit_query,
                       regex_edit->text().trimmed(),
                       description_edit->text().trimmed());
    enableSave(false);
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

void RequestEditDialog::clear()
{
    load_button->setEnabled(false);
    enableSave(false);
    is_name_valid = false;
    is_link_valid = false;
    setQueryValid(false);
    name_edit->clear();
    link_edit->clear();
    regex_edit->clear();
    description_edit->clear();
    edit_request = {};
    edit_query = {};
}

MainWindow* RequestEditDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

void RequestEditDialog::setQueryValid(bool valid)
{
    is_query_valid = valid;
    query_cb->setChecked(valid);
}

void RequestEditDialog::enableSave(bool enable)
{
    ok_button->setEnabled(enable);
    save_button->setEnabled(enable);
}

} // namespace planner
