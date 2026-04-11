#include "ShoppingModel.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanModel.h"
#include "TradeRequestCache.h"

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

int ShoppingModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return items.size();
}

int ShoppingModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(ShoppingColumn::last) + 1;
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
    if (amount == 0.0)
        return false;

    beginResetModel();
    items.clear();
    plan_ = &plan;
    include_dependencies = include_dependencies_;

    bool result{false};
    if (plan_->game == Game::Poe1) {
        exchange_cache = mw->exchange_cache_poe1;
        trade_cache = mw->trade_cache_poe1;
        plan_model = mw->plan_model_poe1;
    } else {
        exchange_cache = mw->exchange_cache_poe2;
        trade_cache = mw->trade_cache_poe2;
        plan_model = mw->plan_model_poe2;
    }

    dependencies.clear();
    result = gatherPlanItems(step_pos, amount);
    dependencies.clear();

    endResetModel();
    return result;
}

bool ShoppingModel::gatherPlanItems(size_t step_pos, double amount)
{
    auto step_it = plan_->steps.begin() + step_pos;
    if (step_it >= plan_->steps.end() || step_it->resources.empty())
        return false;

    auto& plan_data = dependencies.try_emplace(plan_->id()).first->second;
    plan_data.addStep(&(*step_it), amount);
    gatherItems(*plan_, plan_data);

    if (plan_data.exchange_items.empty() && plan_data.trade_items.empty())
        return false;

    auto data = exchange_cache->costData();
    if (!data)
        return false;
    auto& cores = data->core_currencies;
    for (auto& core : std::ranges::reverse_view(cores)) {
        if (auto it = plan_data.exchange_items.find(core.currency);
            it != plan_data.exchange_items.end()) {
            items.emplace_back(it->second, decltype(ShoppingItem::data){it->first});
            plan_data.exchange_items.erase(it);
        }
    }

    for (auto& item : plan_data.exchange_items)
        items.emplace_back(item.second, item.first);
    for (auto& item : plan_data.trade_items)
        items.emplace_back(item.second.first, ShoppingItem::Trade{item.second.second, item.first});

    return true;
}

void ShoppingModel::gatherItems(const Plan& plan, PlanData& data)
{
    if (include_dependencies) {
        for (size_t i = 0; i < data.steps.size(); ++i) {
            auto& step = *data.steps[i].first;
            auto step_amount = data.steps[i].second;
            for (auto& item : step.resources) {
                if (item.not_used || item.amount == 0.0)
                    continue;

                if (auto step_item = item.step()) {
                    auto step_it = plan.findStepIt(step_item->step_id);
                    if (step_it != plan.steps.end())
                        data.addStep(&(*step_it), step_amount * item.amount);
                }
            }
        }
    }

    for (auto [step, step_amount] : data.steps) {
        for (auto& item : step->resources) {
            if (item.not_used || item.amount == 0.0)
                continue;

            if (auto plan_item = item.plan(); plan_item && include_dependencies)
                gatherPlanItem(step_amount * item.amount, *plan_item, data);
            else if (auto exchange = item.exchange())
                gatherCurrency(step_amount * item.amount, exchange->currency, data.exchange_items);
            else if (auto trade = item.trade())
                gatherTradeItem(step_amount * item.amount,
                                *trade,
                                data.exchange_items,
                                data.trade_items);
            else if (auto custom = item.custom(); custom && custom->cost.value != 0.0)
                gatherCurrency(step_amount * item.amount * custom->cost.value,
                               custom->cost.currency,
                               data.exchange_items);
        }
    }
}

void ShoppingModel::gatherPlanItem(double amount, const PlanItemData& plan_item, PlanData& data)
{
    auto plan_it = plan_model->plans.find(plan_item.plan_id);
    if (plan_it == plan_model->plans.end())
        return;

    auto final_step = plan_it->second.costStepIt();
    if (final_step == plan_it->second.steps.end())
        return;

    auto [data_it, added] = dependencies.try_emplace(plan_it->first);
    if (added) {
        data_it->second.addStep(&(*final_step), 1.0);
        gatherItems(plan_it->second, data_it->second);
    }

    data.mergeItems(amount, data_it->second);
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

void ShoppingModel::PlanData::addStep(const Step* step, double amount)
{
    auto it = std::ranges::find(steps, step, &std::pair<const Step*, double>::first);
    if (it == steps.end())
        steps.emplace_back(step, amount);
    else
        it->second += amount;
}

void ShoppingModel::PlanData::mergeItems(double amount, const PlanData& data)
{
    if (this == &data)
        return;

    for (auto& item : data.exchange_items) {
        auto [it, added] = exchange_items.emplace(item);
        if (added)
            it->second *= amount;
        else
            it->second += amount * item.second;
    }
    for (auto& item : data.trade_items) {
        auto [it, added] = trade_items.emplace(item);
        if (added)
            it->second.first *= amount;
        else
            it->second.first += amount * item.second.first;
    }
}

} // namespace planner
