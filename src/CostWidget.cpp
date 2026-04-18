#include "CostWidget.h"
#include "AppState.h"
#include "ExchangeRequestCache.h"
#include "Settings.h"
#include "Step.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

using namespace Qt::StringLiterals;
using namespace std::chrono;

namespace planner {

CostWidget::CostWidget(QWidget* parent)
    : QWidget{parent}
{
    auto main_layout = new QHBoxLayout{};
    main_layout->setHorizontalSizeConstraint(QLayout::SetFixedSize);
    main_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(main_layout);

    cost_label = new QLabel{this};
    layout()->addWidget(cost_label);

    profit_label = new QLabel{this};
    layout()->addWidget(profit_label);

    currency_icon_label = new QLabel{this};
    layout()->addWidget(currency_icon_label);

    currency_label = new QLabel{this};
    layout()->addWidget(currency_label);

    gold_label = new QLabel{this};
    layout()->addWidget(gold_label);

    time_label = new QLabel{this};
    layout()->addWidget(time_label);

    hide();
}

void CostWidget::hideCurrencyName()
{
    if (!currency_label->text().isEmpty())
        currency_label->setHidden(Settings::get<settings::windows_main_hide_title_currency_name>());
}

void CostWidget::setCost(Game game, const Step* step)
{
    if (!step) {
        hide();
        return;
    }

    Currency primary;
    ItemCost cost = step->cost();
    if (cost.isValid()) {
        primary = cost.cost_in_primary.currency;
        cost_label->setText(tr("C: %1").arg(formatCost(cost.cost_in_primary)));
        cost_label->show();
    } else
        cost_label->hide();

    ItemCost profit = step->results_cost - cost;
    if (profit.isValid()) {
        primary = profit.cost_in_primary.currency;
        profit_label->setText(tr("P: %1").arg(formatCost(profit.cost_in_primary)));
        profit_label->show();
    } else
        profit_label->hide();

    auto exchange_cache = AppState::exchangeCache(game);
    if (auto it = exchange_cache->currencyData(primary); it != exchange_cache->cache.end()) {
        currency_icon_label->setPixmap(exchange_cache->icon(it).pixmap(16));
        currency_label->setText(exchange_cache->name(it));

        currency_icon_label->show();
        if (Settings::get<settings::windows_main_hide_title_currency_name>())
            currency_label->hide();
        else
            currency_label->show();
    } else {
        currency_icon_label->hide();
        currency_label->clear();
        currency_label->hide();
    }

    if (cost.gold > 0.0) {
        if (cost.gold >= 1e6)
            gold_label->setText(tr("Gold: %1 M").arg(formatCeil1(cost.gold / 1e6)));
        else if (cost.gold >= 1e3)
            gold_label->setText(tr("Gold: %1 k").arg(formatCeil1(cost.gold / 1e3)));
        else
            gold_label->setText(tr("Gold: %1").arg(QString::number(cost.gold)));

        gold_label->show();
    } else
        gold_label->hide();

    if (cost.time.count() > 0.0) {
        if (cost.time.count() > 180.0) {
            auto minutes = duration<double, std::ratio<60>>(cost.time).count();
            time_label->setText(tr("Time: %1 min").arg(formatCeil1(minutes)));
        } else
            time_label->setText(tr("Time: %1 sec").arg(formatCeil1(cost.time.count())));
        time_label->show();
    } else
        time_label->hide();

    show();
}

QString CostWidget::formatCost(const CurrencyCost& cost)
{
    auto value = std::ceil(100.0 * cost.value) / 100.0;
    return QString::number(value);
}

QString CostWidget::formatCeil1(double value)
{
    return QString::number(std::ceil(10.0 * value) / 10.0);
}

} // namespace planner
