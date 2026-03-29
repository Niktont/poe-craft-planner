#include "ShoppingModel.h"
#include "MainWindow.h"
#include "Plan.h"

namespace planner {

ShoppingModel::ShoppingModel(MainWindow& mw, QObject* parent)
    : QAbstractTableModel{parent}
    , mw{&mw}
{}

QVariant ShoppingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return {};

    auto col = static_cast<ShoppingColumn>(section);
    switch (col) {
    case ShoppingColumn::Amount:
        return tr("Num");
    case ShoppingColumn::Name:
        return tr("Name");
    case ShoppingColumn::Link:
        return tr("Link");
    }
    return {};
}

QVariant ShoppingModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= std::ssize(items))
        return {};

    auto& item = items[index.row()];
    auto col = static_cast<ShoppingColumn>(index.column());
    switch (col) {
    case ShoppingColumn::Amount:
        switch (role) {
        case Qt::DisplayRole:
            return QString::number(std::ceil(item.amount));
        case Qt::TextAlignmentRole:
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        }
        return {};
    case ShoppingColumn::Name:
        if (auto exchange = item.exchange()) {
            switch (role) {
            case Qt::DecorationRole:
                if (auto it = exchange_cache->currencyData(*exchange);
                    it != exchange_cache->cache.end())
                    return exchange_cache->icon(it);
                return {};
            case Qt::DisplayRole:
                if (auto it = exchange_cache->currencyData(*exchange);
                    it != exchange_cache->cache.end())
                    return exchange_cache->name(it);
                return {};
            }
        } else if (auto trade = item.trade()) {
            switch (role) {
            case Qt::DisplayRole:
                if (!trade->name.isEmpty())
                    return trade->name;
                else if (auto it = trade_cache->requestData(trade->request);
                         it != trade_cache->cache.end())
                    return it->second.name();
            }
        }
        return {};
    case ShoppingColumn::Link:
        if (auto trade = item.trade()) {
            switch (role) {
            case Qt::DisplayRole:
                return tr("Link");
            case Qt::ToolTipRole:
                if (auto url = trade->request.toUrl(plan_->game); !url.isEmpty())
                    return url;
                return {};
            case Qt::ForegroundRole:
                return QColor{0x0000EE};
            case Qt::FontRole: {
                QFont font;
                font.setUnderline(true);
                return font;
            }
            }
        }
        return {};
    }
    return {};
}

bool ShoppingModel::setPlan(Plan& plan, size_t step_pos, double amount, bool include_dependencies_)
{
    beginResetModel();
    items.clear();
    plan_ = &plan;
    include_dependencies = include_dependencies_;

    bool result{false};
    if (plan_->game == Game::Poe1) {
        exchange_cache = mw->exchange_cache_poe1;
        trade_cache = mw->trade_cache_poe1;
    } else {
        exchange_cache = mw->exchange_cache_poe2;
        trade_cache = mw->trade_cache_poe2;
    }
    result = gatherPlanItems(step_pos, amount);

    endResetModel();
    return result;
}

bool ShoppingModel::gatherPlanItems(size_t step_pos, double amount)
{
    auto step_it = plan_->steps.begin() + step_pos;
    if (step_it >= plan_->steps.end() || step_it->resources.empty())
        return false;

    ExchangeItems exchange_items;
    TradeItems trade_items;
    gatherStepItems(amount, step_it, exchange_items, trade_items);
    if (exchange_items.empty() && trade_items.empty())
        return false;

    auto league_it = exchange_cache->currentLeagueData();
    if (league_it == exchange_cache->cost_cache.end())
        return false;
    auto& cores = league_it->second.core_currencies;
    for (auto& core : std::ranges::reverse_view(cores)) {
        if (auto it = exchange_items.find(core.currency); it != exchange_items.end()) {
            items.emplace_back(it->second, decltype(ShoppingItem::data){it->first});
            exchange_items.erase(it);
        }
    }

    for (auto& item : exchange_items)
        items.emplace_back(item.second, item.first);
    for (auto& item : trade_items)
        items.emplace_back(item.second.first, ShoppingItem::Trade{item.second.second, item.first});

    return true;
}

void ShoppingModel::gatherStepItems(double amount,
                                    std::vector<Step>::const_iterator step_it,
                                    ExchangeItems& exchange_items,
                                    TradeItems& trade_items)
{
    if (step_it->resources.empty())
        return;

    auto pos = std::distance(plan_->steps.cbegin(), step_it);
    for (auto& item : step_it->resources) {
        if (!item.not_used)
            gatherItem(amount, pos, item, exchange_items, trade_items);
    }
}

void ShoppingModel::gatherItem(double amount,
                               ptrdiff_t pos,
                               const StepItem& item,
                               ExchangeItems& exchange_items,
                               TradeItems& trade_items)
{
    if (item.amount == 0.0)
        return;

    if (auto step_item = item.step()) {
        if (!include_dependencies)
            return;

        if (auto dep_it = plan_->findStepIt(step_item->step_id); dep_it != plan_->steps.end()) {
            auto dep_pos = std::distance(plan_->steps.cbegin(), dep_it);
            if (dep_pos < pos)
                gatherStepItems(amount * item.amount, dep_it, exchange_items, trade_items);
        }
    } else if (auto exchange = item.exchange())
        gatherCurrency(amount * item.amount, exchange->currency, exchange_items);
    else if (auto trade = item.trade())
        gatherTradeItem(amount * item.amount, *trade, exchange_items, trade_items);
    else if (auto custom = item.custom(); custom && custom->cost.value != 0.0)
        gatherCurrency(amount * item.amount * custom->cost.value,
                       custom->cost.currency,
                       exchange_items);
}

void ShoppingModel::gatherCurrency(double amount,
                                   const Currency& currency,
                                   ExchangeItems& exchange_items)
{
    if (!currency.isValid())
        return;

    auto res = exchange_items.try_emplace(currency, amount);
    if (!res.second)
        res.first->second += amount;

    if (!exchange_cache->isCore(currency)) {
        auto cost = exchange_cache->cost(currency);
        if (!cost.currency.isValid())
            return;

        cost.value *= amount;
        res = exchange_items.try_emplace(cost.currency, cost.value);
        if (!res.second)
            res.first->second += cost.value;
    }
}

void ShoppingModel::gatherTradeItem(double amount,
                                    const TradeItemData& trade,
                                    ExchangeItems& exchange_items,
                                    TradeItems& trade_items)
{
    if (!trade.request_key.isValid())
        return;

    auto res = trade_items.try_emplace(trade.request_key, amount, trade.name);
    if (!res.second)
        res.first->second.first += amount;

    if (auto cost_data = trade_cache->costData(trade.request_key))
        gatherCurrency(amount * cost_data->cost.value, cost_data->cost.currency, exchange_items);
}
} // namespace planner
