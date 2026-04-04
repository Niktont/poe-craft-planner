#include "ShoppingDialog.h"
#include "MainWindow.h"
#include "PlanSearchDialog.h"
#include "Settings.h"
#include "ShoppingModel.h"
#include "ShoppingView.h"
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QVBoxLayout>
#ifndef PLANNER_NO_BROWSER
#include "WebViewDialog.h"
#endif

using namespace keyboard_auto_type;

namespace planner {

ShoppingDialog::ShoppingDialog(MainWindow& mw)
    : QDialog{}
    , mw{mw}
{
    model = new ShoppingModel{mw, this};
    view = new ShoppingView{*model};

    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);

    main_layout->addWidget(view, 1);

    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_shopping_dialog_geometry);
    if (!geometry.isValid())
        setGeometry(200, 200, 280, 400);
    else
        restoreGeometry(geometry.toByteArray());

    connect(this, &QDialog::finished, this, [this] {
        this->mw.show();

#ifndef PLANNER_NO_BROWSER
        if (web_view_dialog_was_shown)
            this->mw.web_view_dialog->show();
        web_view_dialog_was_shown = false;
#endif

        if (plan_search_dialog_was_shown)
            this->mw.plan_search_dialog->show();
        plan_search_dialog_was_shown = false;

        removeHotkeys();
    });

    next_item_hk = new QHotkey{this};
    want_hk = new QHotkey{this};
    want_amount_hk = new QHotkey{this};
    have_hk = new QHotkey{this};
    have_amount_hk = new QHotkey{this};
    open_link_hk = new QHotkey{this};

    connect(next_item_hk, &QHotkey::activated, this, &ShoppingDialog::nextItem);
    connect(want_hk, &QHotkey::activated, this, &ShoppingDialog::pasteWant);
    connect(want_amount_hk, &QHotkey::activated, this, &ShoppingDialog::pasteWantAmount);
    connect(have_hk, &QHotkey::activated, this, &ShoppingDialog::pasteHave);
    connect(have_amount_hk, &QHotkey::activated, this, &ShoppingDialog::pasteHaveAmount);
    connect(open_link_hk, &QHotkey::activated, this, &ShoppingDialog::openLink);
}

void ShoppingDialog::openPlan(Plan& plan)
{
    auto it = plan.costStepIt();
    if (it == plan.steps.cend())
        return;

    openPlan(plan, std::distance(plan.steps.cbegin(), it), 1.0, true);
}

void ShoppingDialog::openPlan(Plan& plan, size_t step_pos, double amount, bool include_dependencies)
{
    auto res = model->setPlan(plan, step_pos, amount, include_dependencies);
    if (res) {
#ifndef PLANNER_NO_BROWSER
        web_view_dialog_was_shown = !mw.web_view_dialog->isHidden();
        if (web_view_dialog_was_shown)
            mw.web_view_dialog->hide();
#endif

        plan_search_dialog_was_shown = !mw.plan_search_dialog->isHidden();
        if (plan_search_dialog_was_shown)
            mw.plan_search_dialog->hide();

        mw.hide();

        view->adjustNameWidth();
        view->updateGeometry();
        view->selectRow(0);

        auto width = layout()->contentsRect().width();
        resize(width, height());
        setWindowTitle(tr("%1 - Shopping list").arg(plan.name));

        open();
        registerHotkeys();
    }
}

void ShoppingDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    accept();
}

void ShoppingDialog::nextItem()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid()) {
        if (model->rowCount() == 0)
            return;
        view->setCurrentIndex(model->index(0, 0));
        view->selectRow(0);
    }

    if (idx.row() == model->rowCount() - 1)
        return;

    view->setCurrentIndex(model->index(idx.row() + 1, 0));
    view->selectRow(idx.row() + 1);
}

void ShoppingDialog::pasteWant()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    auto& item = model->item(idx.row());
    if (auto exchange = item.exchange()) {
        auto it = model->exchangeCache()->currencyData(*exchange);
        pasteText(model->exchangeCache()->name(it));
    } else if (auto trade = item.trade()) {
        auto it = model->tradeCache()->requestData(trade->request);
        if (!it->second.regex().isEmpty())
            pasteText(it->second.regex());
    }
}

void ShoppingDialog::pasteWantAmount()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    auto& item = model->item(idx.row());
    pasteText(QString::number(static_cast<int>(std::ceil(item.amount))));
}

void ShoppingDialog::pasteHave()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    auto& item = model->item(idx.row());
    if (auto exchange = item.exchange()) {
        auto [val, it] = model->exchangeCache()->costData(*exchange);
        if (it != model->exchangeCache()->cache.end())
            pasteText(model->exchangeCache()->name(it));
    }
}

void ShoppingDialog::pasteHaveAmount()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    auto& item = model->item(idx.row());
    if (auto exchange = item.exchange()) {
        auto cost = model->exchangeCache()->cost(*exchange);
        if (cost.value != 0.0)
            pasteText(QString::number(static_cast<int>(std::ceil(item.amount * cost.value))));
    }
}

void ShoppingDialog::openLink()
{
    auto idx = view->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    auto& item = model->item(idx.row());
    if (auto trade = item.trade(); trade && model->plan())
        QDesktopServices::openUrl(trade->request.toUrl(model->plan()->game));
}

void ShoppingDialog::registerHotkeys()
{
    next_item_hk->setShortcut(Settings::get<Settings::hotkeys_next_item>(), true);
    want_hk->setShortcut(Settings::get<Settings::hotkeys_paste_want>(), true);
    want_amount_hk->setShortcut(Settings::get<Settings::hotkeys_paste_want_amount>(), true);
    have_hk->setShortcut(Settings::get<Settings::hotkeys_paste_have>(), true);
    have_amount_hk->setShortcut(Settings::get<Settings::hotkeys_paste_have_amount>(), true);
    open_link_hk->setShortcut(Settings::get<Settings::hotkeys_open_link>(), true);
}

void ShoppingDialog::removeHotkeys()
{
    next_item_hk->setRegistered(false);
    want_hk->setRegistered(false);
    want_amount_hk->setRegistered(false);
    have_hk->setRegistered(false);
    have_amount_hk->setRegistered(false);
    open_link_hk->setRegistered(false);
}

void ShoppingDialog::pasteText(QString text)
{
    qApp->clipboard()->setText(text);

    auto shortcut_pressed = (auto_type.get_pressed_modifiers() & AutoType::shortcut_modifier())
                            == AutoType::shortcut_modifier();
    if (shortcut_pressed)
        auto_type.key_press(KeyCode::V);
    else
        auto_type.shortcut(KeyCode::V);
}

} // namespace planner
