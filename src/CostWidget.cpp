#include "CostWidget.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Step.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

using namespace Qt::StringLiterals;
using namespace std::chrono;

namespace planner {

CostWidget::CostWidget(MainWindow& mw, QWidget* parent)
    : QWidget{parent}
    , mw{&mw}
{
    auto main_layout = new QHBoxLayout{};
    setLayout(main_layout);

    cost_label = new QLabel{};
    layout()->addWidget(cost_label);

    profit_label = new QLabel{};
    layout()->addWidget(profit_label);

    currency_icon_label = new QLabel{};
    layout()->addWidget(currency_icon_label);

    currency_label = new QLabel{};
    layout()->addWidget(currency_label);

    gold_label = new QLabel{};
    layout()->addWidget(gold_label);

    time_label = new QLabel{};
    layout()->addWidget(time_label);

    main_layout->setContentsMargins(5, 0, 0, 0);
    hide();
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

    auto exchange_cache = mw->exchangeCache(game);
    if (auto it = exchange_cache->currencyData(primary); it != exchange_cache->cache.end()) {
        currency_icon_label->setPixmap(exchange_cache->icon(it).pixmap(16));
        currency_label->setText(it->second.name);

        currency_icon_label->show();
        currency_label->show();
    } else {
        currency_icon_label->hide();
        currency_label->hide();
    }

    if (cost.gold > 0.0) {
        gold_label->setText(tr("Gold: %1").arg(QString::number(cost.gold)));
        gold_label->show();
    } else
        gold_label->hide();

    if (cost.time.count() > 0.0) {
        if (cost.time.count() > 180.0)
            time_label->setText(
                tr("Time: %1 min")
                    .arg(QString::number(duration<double, std::ratio<60>>(cost.time).count())));
        else
            time_label->setText(tr("Time: %1 sec").arg(QString::number(cost.time.count())));
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

} // namespace planner
