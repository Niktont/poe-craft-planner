#include "StepItemModel.h"
#include "AppState.h"
#include "ArithmeticCalculation.h"
#include "ExchangeRequestCache.h"
#include "Plan.h"
#include "PlanModel.h"
#include "Step.h"
#include "StepItemCopyState.h"
#include "TradeRequestCache.h"
#include <boost/container/flat_set.hpp>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFont>
#include <QIODevice>
#include <QMimeData>

using namespace Qt::StringLiterals;

namespace planner {

StepItemModel::StepItemModel(bool is_resource_model, QObject* parent)
    : QAbstractTableModel{parent}
    , is_resource_model{is_resource_model}
{
    connect(this, &QAbstractTableModel::rowsInserted, this, &StepItemModel::updateRowNumbers);
    connect(this, &QAbstractTableModel::rowsRemoved, this, &StepItemModel::updateRowNumbers);
}

QVariant StepItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return {};

    auto col = static_cast<StepItemColumn>(section);
    switch (role) {
    case Qt::DisplayRole:
        switch (col) {
        case StepItemColumn::Row:
            return tr("N");
        case StepItemColumn::Type:
            return tr("T");
        case StepItemColumn::Amount:
            return tr("Num");
        case StepItemColumn::Name:
            return tr("Name");
        case StepItemColumn::Link:
            return tr("Link");
        case StepItemColumn::Cost:
            return tr("Cost");
        case StepItemColumn::CostCurrency:
            return tr("Currency");
        case StepItemColumn::Gold:
            return tr("Gold");
        case StepItemColumn::Time:
            return tr("Time");
        case StepItemColumn::Success:
            return tr("Success");
        }
        return {};
    case Qt::ToolTipRole:
        switch (col) {
        case StepItemColumn::Row:
            return tr("Row");
        case StepItemColumn::Type:
            return tr("Type of item");
        case StepItemColumn::Amount:
            if (is_resource_model)
                return tr("Average amount of items needed for this step");
            return tr("Average amount of items obtained from this step");
        case StepItemColumn::Name:
            return tr("Selector for Exchange and Step items, name for others");
        case StepItemColumn::Link:
            return tr("Selector/link for Trade and Plan items, link for others");
        case StepItemColumn::Cost:
            if (is_resource_model)
                return tr("Cost per item");
            return tr("Cost of single item");
        case StepItemColumn::CostCurrency:
            return tr("Selector for Custom item, cost currency for others");
        case StepItemColumn::Gold:
            return tr("Gold cost/fee per item");
        case StepItemColumn::Time:
            return tr("Time (seconds) spent per item, clear to use default value");
        case StepItemColumn::Success:
            return tr("Total cost of failed items deducted from calculated cost of resources");
        }
        return {};
    }

    return {};
}

int StepItemModel::rowCount(const QModelIndex& parent) const
{
    if (!plan_ || parent.isValid())
        return 0;

    return stepItems().size();
}

int StepItemModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(StepItemColumn::last) + 1;
}

bool StepItemModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (!plan_ || row < 0 || count == 0)
        return false;

    auto& items = stepItems();
    if (row > std::ssize(items))
        return false;

    beginInsertRows(parent, row, row + count - 1);

    items.insert(items.begin() + row, count, {});
    plan_->setChanged();

    endInsertRows();

    return true;
}

bool StepItemModel::moveRows(const QModelIndex& sourceParent,
                             int sourceRow,
                             int count,
                             const QModelIndex& destinationParent,
                             int destinationChild)
{
    if (!plan_ || count == 0 || sourceRow < 0 || destinationChild < 0)
        return false;

    auto& items = stepItems();
    if ((sourceRow + count) > std::ssize(items) || destinationChild > std::ssize(items))
        return false;

    auto first = items.begin() + sourceRow;
    auto last = first + count;
    auto dest = items.begin() + destinationChild;
    int type;
    if (dest < first)
        type = 2;
    else if (dest > last)
        type = 3;
    else
        return false;

    beginMoveRows(sourceParent,
                  sourceRow,
                  sourceRow + count - 1,
                  destinationParent,
                  destinationChild);

    if (type == 2)
        std::rotate(dest, first, last);
    else
        std::rotate(first, last, dest);

    plan_->setChanged();
    endMoveRows();
    return true;
}

bool StepItemModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (!plan_ || row < 0 || count == 0)
        return false;

    auto& items = stepItems();
    if ((row + count) > std::ssize(items))
        return false;

    auto first = items.begin() + row;

    beginRemoveRows(parent, row, row + count - 1);

    items.erase(first, first + count);
    plan_->setChanged();

    endRemoveRows();

    return true;
}

QModelIndex StepItemModel::index(int row, StepItemColumn column) const
{
    return index(row, static_cast<int>(column));
}

QVariant StepItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !plan_)
        return {};

    auto& items = stepItems();
    if (index.row() >= std::ssize(items))
        return {};

    auto& item = items[index.row()];
    auto col = static_cast<StepItemColumn>(index.column());
    switch (col) {
    case planner::StepItemColumn::Row:
        switch (role) {
        case Qt::DisplayRole:
            return index.row() + 1;
        case Qt::TextAlignmentRole:
            return QVariant{Qt::AlignCenter};
        }
        return {};
    case StepItemColumn::Type:
        switch (role) {
        case Qt::DisplayRole: {
            auto str = item.typeStr();
            if (!str.isEmpty())
                return str.front();
            return {};
        }
        case Qt::TextAlignmentRole:
            return QVariant{Qt::AlignCenter};
        }
        return {};
    case StepItemColumn::Amount:
        switch (role) {
        case Qt::DisplayRole:
            return QString::number(item.amount);
        case Qt::EditRole:
            return QString::number(item.amount, 'f', QLocale::FloatingPointShortest);
        case Qt::TextAlignmentRole:
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        }
        break;
    case StepItemColumn::Link:
        switch (role) {
        case Qt::DisplayRole:
            return tr("Link");
        case Qt::FontRole: {
            QFont font;
            font.setUnderline(true);
            return font;
        }
        }
        break;
    case StepItemColumn::Cost:
        if (role == Qt::TextAlignmentRole)
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        break;
    case StepItemColumn::Gold:
        if (role == Qt::TextAlignmentRole)
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        break;
    case StepItemColumn::Time:
        if (role == Qt::TextAlignmentRole)
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        break;
    case StepItemColumn::Success:
        switch (role) {
        case Qt::CheckStateRole:
            if (item.type() == StepItemType::Step || item.type() == StepItemType::Plan)
                return Qt::Unchecked;
            return item.is_success_result ? Qt::Checked : Qt::Unchecked;
        }
        return {};
    default:
        break;
    }

    if (auto exchange = item.exchange())
        return exchangeItemData(item.amount, *exchange, col, role);
    else if (auto trade = item.trade())
        return tradeItemData(item.amount, *trade, col, role);
    else if (auto custom = item.custom())
        return customItemData(item.amount, *custom, col, role);
    else if (auto step = item.step())
        return stepItemData(item.amount, *step, col, role);
    else if (auto plan = item.plan())
        return planItemData(item.amount, *plan, col, role);

    return {};
}

bool StepItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || !plan_)
        return false;

    auto& items = stepItems();
    if (index.row() >= std::ssize(items))
        return false;
    auto& item = items[index.row()];

    auto col = static_cast<StepItemColumn>(index.column());
    if (col == StepItemColumn::Success && role == Qt::CheckStateRole) {
        auto checked = value.value<Qt::CheckState>() == Qt::Checked;
        if ((item.type() == StepItemType::Step || item.type() == StepItemType::Plan) && checked)
            return false;
        if (item.is_success_result != checked) {
            item.is_success_result = checked;
            plan_->setChanged();
            emit dataChanged(index, index, {role});
            return true;
        }
        return false;
    }

    if (role != Qt::EditRole)
        return false;

    switch (col) {
    case StepItemColumn::Amount: {
        auto val = AppState::state.arithmetic_calc->calculate(value.toString());
        if (!val || *val < 0.0 || *val == item.amount)
            return false;
        item.amount = *val;
        plan_->setChanged();
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }
    default:
        break;
    }
    bool res = false;
    if (auto exchange = item.exchange())
        res = setExchangeItemData(*exchange, value, index);
    else if (auto trade = item.trade())
        res = setTradeItemData(*trade, value, index);
    else if (auto custom = item.custom())
        res = setCustomItemData(*custom, value, index);
    else if (auto step = item.step())
        res = setStepItemData(*step, value, index);
    else if (auto plan = item.plan())
        res = setPlanItemData(*plan, value, index);

    if (res)
        plan_->setChanged();

    return res;
}

Qt::ItemFlags StepItemModel::flags(const QModelIndex& index) const
{
    if (!plan_)
        return Qt::NoItemFlags;
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    auto default_flags = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;

    auto col = static_cast<StepItemColumn>(index.column());
    switch (col) {
    case StepItemColumn::Row:
        return default_flags;
    case StepItemColumn::Type:
    case StepItemColumn::Amount:
    case StepItemColumn::Name:
        return default_flags | Qt::ItemIsEditable;
    case StepItemColumn::Link: {
        auto& item = stepItems().at(index.row());
        if (item.type() == StepItemType::Custom || item.type() == StepItemType::Trade
            || item.type() == StepItemType::Plan)
            return default_flags | Qt::ItemIsEditable;
        return default_flags;
    }
    case StepItemColumn::Cost:
    case StepItemColumn::CostCurrency:
    case StepItemColumn::Gold: {
        auto& item = stepItems().at(index.row());
        if (item.type() != StepItemType::Custom)
            return default_flags;
        return default_flags | Qt::ItemIsEditable;
    }
    case StepItemColumn::Time: {
        auto& item = stepItems().at(index.row());
        if (item.type() == StepItemType::Step || item.type() == StepItemType::Plan)
            return default_flags;
        return default_flags | Qt::ItemIsEditable;
    }
    case StepItemColumn::Success:
        return default_flags | Qt::ItemIsUserCheckable;
    }

    return Qt::NoItemFlags;
}

const QString StepItemModel::move_mime_poe1{u"application/x-movestepitem1"};
const QString StepItemModel::move_mime_poe2{u"application/x-movestepitem2"};

QStringList StepItemModel::mimeTypes() const
{
    if (!plan_)
        return {};

    QStringList types;
    types << (plan_->game == Game::Poe1 ? move_mime_poe1 : move_mime_poe2);
    return types;
}

QMimeData* StepItemModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.empty() || !plan_)
        return nullptr;

    auto mime_data = new QMimeData{};

    QByteArray encodedData;
    QDataStream stream{&encodedData, QIODevice::WriteOnly};

    stream << std::bit_cast<size_t>(this);

    auto& items = stepItems();
    boost::container::flat_set<const StepItem*> item_ptrs;
    for (const QModelIndex& index : indexes) {
        if (index.isValid())
            item_ptrs.insert(&items[index.row()]);
    }
    for (auto item : item_ptrs)
        stream << std::bit_cast<size_t>(item);

    mime_data->setData(plan_->game == Game::Poe1 ? move_mime_poe1 : move_mime_poe2, encodedData);
    return mime_data;
}

bool StepItemModel::canDropMimeData(const QMimeData* data,
                                    Qt::DropAction /*action*/,
                                    int /*row*/,
                                    int /*column*/,
                                    const QModelIndex& /*parent*/) const
{
    if (!plan_)
        return false;

    auto& move_mime = plan_->game == Game::Poe1 ? move_mime_poe1 : move_mime_poe2;
    auto& plan_mime = plan_->game == Game::Poe1 ? PlanModel::move_mime_poe1
                                                : PlanModel::move_mime_poe2;
    if (!data->hasFormat(move_mime) && !data->hasFormat(plan_mime))
        return false;

    return true;
}

void planner::StepItemModel::moveItems(int dest_row, const QMimeData& data)
{
    auto [source_model, source_ptrs] = decodeStepItemsMime(plan_->game, data);

    auto& items = stepItems();
    if (source_model == this) {
        for (auto item : source_ptrs) {
            moveRows({}, std::distance(&items.front(), item), 1, {}, dest_row);
        }
    } else {
        for (auto source_item : source_ptrs) {
            beginInsertRows({}, dest_row, dest_row);
            items.insert(items.begin() + dest_row, std::move(*source_item));
            endInsertRows();

            plan_->setChanged();
            ++dest_row;
        }

        auto& source_items = source_model->stepItems();
        for (auto source_item : std::views::reverse(source_ptrs)) {
            auto source_row = std::distance(&source_items.front(), source_item);
            source_model->beginRemoveRows({}, source_row, source_row);
            source_items.erase(source_items.begin() + source_row);
            source_model->endRemoveRows();
        }
    }
}

void StepItemModel::addPlanItems(int dest_row, const QMimeData& data)
{
    auto plans_to_add = PlanModel::decodeMimeToPlans(plan_->game, data);
    if (plans_to_add.empty())
        return;

    auto& items = stepItems();
    auto amount = dest_row > 0 ? items[dest_row - 1].amount : 1.0;

    StepItem base_item;
    base_item.amount = amount;
    base_item.data.emplace<PlanItemData>();

    beginInsertRows({}, dest_row, dest_row + static_cast<int>(plans_to_add.size()) - 1);

    items.insert(items.begin() + dest_row, plans_to_add.size(), base_item);
    for (auto plan_to_add : plans_to_add) {
        items[dest_row].plan()->plan_id = plan_to_add->id();
        ++dest_row;
    }

    plan_->setChanged();

    endInsertRows();
}

bool StepItemModel::dropMimeData(
    const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    int dest_row;
    if (row != -1)
        dest_row = row;
    else
        dest_row = rowCount();

    auto& move_mime = plan_->game == Game::Poe1 ? move_mime_poe1 : move_mime_poe2;
    if (data->hasFormat(move_mime))
        moveItems(dest_row, *data);
    else
        addPlanItems(dest_row, *data);

    return true;
}

std::pair<StepItemModel*, std::vector<StepItem*>> StepItemModel::decodeStepItemsMime(
    Game game, const QMimeData& data)
{
    auto& move_mime = game == Game::Poe1 ? move_mime_poe1 : move_mime_poe2;
    QByteArray encodedData = data.data(move_mime);
    QDataStream stream{&encodedData, QIODevice::ReadOnly};

    size_t source_ptr;
    stream >> source_ptr;
    auto source_model = std::bit_cast<StepItemModel*>(source_ptr);

    std::vector<StepItem*> source_items;
    while (!stream.atEnd()) {
        size_t source_item;
        stream >> source_item;
        source_items.push_back(std::bit_cast<StepItem*>(source_item));
    }

    return {source_model, std::move(source_items)};
}

const StepItem* StepItemModel::stepItem(const QModelIndex& idx) const
{
    if (!plan_ || !idx.isValid())
        return nullptr;

    return &stepItems()[idx.row()];
}

Step* StepItemModel::step()
{
    assert(plan_);
    return &plan_->steps[step_pos];
}

void StepItemModel::updateRowNumbers(const QModelIndex& /*idx*/, int /*first*/, int last)
{
    auto last_row = rowCount() - 1;
    if (last >= last_row)
        return;

    auto top = index(last + 1, StepItemColumn::Row);
    auto bottom = index(last_row, StepItemColumn::Row);
    emit dataChanged(top, bottom, {Qt::DisplayRole});
}

std::vector<StepItem>& StepItemModel::stepItems()
{
    assert(plan_);
    return is_resource_model ? step()->resources : step()->results;
}

const std::vector<StepItem>& StepItemModel::stepItems() const
{
    assert(plan_);
    return is_resource_model ? step()->resources : step()->results;
}

void StepItemModel::setItemType(const QModelIndex& index, StepItemType type)
{
    auto& item = stepItems()[index.row()];
    if (item.type() == type)
        return;

    if (auto custom = item.custom(); custom && type == StepItemType::Trade) {
        auto name = std::move(custom->name);
        auto link = std::move(custom->link);
        auto time = custom->time;
        auto& trade = item.data.emplace<TradeItemData>();
        trade.name = name;
        trade.time = time;
        if (auto key = TradeRequestKey::fromUrl(link, plan_->game))
            trade.request_key = key.assume_value();
    } else if (auto trade = item.trade(); trade && type == StepItemType::Custom) {
        auto name = std::move(trade->name);
        auto key = std::move(trade->request_key);
        auto time = trade_cache->time(*trade);
        auto& custom_data = item.data.emplace<CustomItemData>();
        custom_data.name = name;
        custom_data.link = key.toUrl(plan_->game);
        custom_data.time = time;
        if (auto data = trade_cache->costData(key)) {
            custom_data.cost = data->cost;
            custom_data.gold = data->gold_fee;
        }
    } else
        item.setType(type);

    plan_->setChanged();
    emit dataChanged(index, index.siblingAtColumn(columnCount() - 1));
}

QVariant StepItemModel::exchangeItemData(double amount,
                                         const ExchangeItemData& exchange,
                                         StepItemColumn col,
                                         int role) const
{
    if (exchange.currency.id.isEmpty())
        return {};

    auto& cache = exchange_cache->cache;
    switch (col) {
    case StepItemColumn::Name:
        switch (role) {
        case Qt::DecorationRole:
            if (auto it = exchange_cache->currencyData(exchange.currency); it != cache.end())
                return exchange_cache->icon(it);
            return {};
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (auto it = exchange_cache->currencyData(exchange.currency); it != cache.end())
                return exchange_cache->name(it);
            return {};
        }
        return {};
    case StepItemColumn::Link:
        switch (role) {
        case Qt::ToolTipRole:
            if (auto link = exchange_cache->link(exchange.currency); !link.isEmpty())
                return exchange_cache->link(exchange.currency);
            return {};
        case Qt::ForegroundRole:
            if (exchange_cache->haveLink(exchange.currency))
                return QColor{0x0000EE};
            return {};
        }
        return {};
    case StepItemColumn::Cost:
        switch (role) {
        case Qt::DisplayRole:
            return formatCostWithRatio(exchange_cache->cost(exchange.currency).value);
        case Qt::ToolTipRole:
            if (auto cost = exchange_cache->cost(exchange.currency); cost.value > 0.0)
                return QString::number(amount * cost.value);
            return {};
        }
        return {};
    case StepItemColumn::CostCurrency:
        switch (role) {
        case Qt::DecorationRole:
            if (auto [val, it] = exchange_cache->costCurrency(exchange.currency); it != cache.end())
                return exchange_cache->icon(it);
            return {};
        case Qt::DisplayRole:
            if (auto [val, it] = exchange_cache->costCurrency(exchange.currency); it != cache.end())
                return exchange_cache->name(it);
            return {};
        }
        return {};
    case StepItemColumn::Gold:
        switch (role) {
        case Qt::DisplayRole:
            if (auto it = exchange_cache->currencyData(exchange.currency); it != cache.end())
                return formatGold(it->second.gold_fee);
            return {};
        }
        return {};
    case StepItemColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
            return formatTime(exchange_cache->time(exchange));
        case Qt::EditRole:
            return exchange.time ? QString::number(exchange.time->count()) : QVariant{};
        }
        return {};
    default:
        return {};
    }
}

bool StepItemModel::setExchangeItemData(ExchangeItemData& exchange,
                                        const QVariant& value,
                                        const QModelIndex& idx)
{
    auto col = static_cast<StepItemColumn>(idx.column());
    switch (col) {
    case StepItemColumn::Name: {
        auto selected_index = value.toModelIndex();
        if (selected_index.isValid()) {
            exchange.currency.id = exchange_cache->cache.nth(selected_index.row())->first;
            exchange.currency.cache_pos = selected_index.row();
            emit dataChanged(idx, idx, {Qt::DisplayRole, Qt::DecorationRole});

            auto link_index = sibling(idx, StepItemColumn::Link);
            emit dataChanged(link_index, link_index, {Qt::ForegroundRole});

            auto cost_value_index = sibling(idx, StepItemColumn::Cost);
            emit dataChanged(cost_value_index, cost_value_index, {Qt::DisplayRole});

            auto cost_currency_index = sibling(idx, StepItemColumn::CostCurrency);
            emit dataChanged(cost_currency_index,
                             cost_currency_index,
                             {Qt::DisplayRole, Qt::DecorationRole});

            auto gold_index = sibling(idx, StepItemColumn::Gold);
            auto time_index = sibling(idx, StepItemColumn::Time);
            emit dataChanged(gold_index, time_index, {Qt::DisplayRole});
            return true;
        }
        return false;
    }
    case StepItemColumn::Time: {
        auto str = value.toString();
        if (str.isEmpty()) {
            if (!exchange.time)
                return false;
            exchange.time.reset();
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            return true;
        }

        auto val = AppState::state.arithmetic_calc->calculate(str);
        if (!val || *val < 0.0 || *val == exchange_cache->time(exchange).count())
            return false;
        exchange.time = ItemTime{*val};
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    }
    default:
        return false;
    }
}

QVariant StepItemModel::tradeItemData(double amount,
                                      const TradeItemData& trade,
                                      StepItemColumn col,
                                      int role) const
{
    if (!trade.request_key.isValid()) {
        if (col == StepItemColumn::Name && !trade.name.isEmpty()
            && (role == Qt::DisplayRole || role == Qt::EditRole))
            return trade.name;
        return {};
    }

    switch (col) {
    case StepItemColumn::Amount:
        if (role == Qt::ToolTipRole) {
            if (auto cost_data = trade_cache->costData(trade.request_key))
                return tr("Available: %1").arg(cost_data->total_available);
        }
        return {};
    case StepItemColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return trade_cache->name(trade);
        case Qt::ToolTipRole:
            return trade_cache->description(trade.request_key);
        }
        return {};
    case StepItemColumn::Link:
        switch (role) {
        case Qt::ForegroundRole:
            if (trade.request_key.isValid())
                return QColor{0x0000EE};
            return {};
        case Qt::EditRole:
            if (auto it = trade_cache->requestData(trade.request_key);
                it != trade_cache->cache.end())
                return it->second.name();
            return {};
        case Qt::ToolTipRole:
            if (auto url = trade.request_key.toUrl(plan_->game); !url.isEmpty())
                return url;
            return {};
        }
        return {};
    case planner::StepItemColumn::Cost:
        switch (role) {
        case Qt::DisplayRole:
            return formatCost(trade_cache->costValue(trade.request_key));
        case Qt::ToolTipRole:
            if (auto val = trade_cache->costValue(trade.request_key); val != 0.0)
                return QString::number(amount * val);
            return {};
        }
        return {};
    case planner::StepItemColumn::CostCurrency:
        switch (role) {
        case Qt::DisplayRole:
            if (auto currency_data = trade_cache->costCurrency(trade.request_key))
                return exchange_cache->name(*currency_data);
            return {};
        case Qt::DecorationRole:
            if (auto currency_data = trade_cache->costCurrency(trade.request_key))
                return currency_data->icon;
            return {};
        }
        return {};
    case planner::StepItemColumn::Gold:
        switch (role) {
        case Qt::DisplayRole:
            return formatGold(trade_cache->goldFee(trade.request_key));
        }
        return {};
    case StepItemColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
            return formatTime(trade_cache->time(trade));
        case Qt::EditRole:
            return trade.time ? QString::number(trade.time->count()) : QVariant{};
        }
        return {};
    default:
        return {};
    }
}

bool StepItemModel::setTradeItemData(TradeItemData& trade,
                                     const QVariant& value,
                                     const QModelIndex& idx)
{
    auto col = static_cast<StepItemColumn>(idx.column());
    switch (col) {
    case StepItemColumn::Name: {
        if (auto name = value.toString(); name != trade.name) {
            if (name != trade_cache->name(trade)) {
                trade.name = name;
                emit dataChanged(idx, idx, {Qt::DisplayRole});
            } else
                trade.name = name;
            return true;
        }
        return false;
    }
    case StepItemColumn::Link: {
        auto selected_index = value.toModelIndex();
        if (selected_index.isValid()) {
            trade.request_key = trade_cache->cache.nth(selected_index.row())->first;
            emit dataChanged(idx, idx, {Qt::ForegroundRole});

            auto name_index = sibling(idx, StepItemColumn::Name);
            emit dataChanged(name_index, name_index, {Qt::DisplayRole});

            auto cost_value_index = sibling(idx, StepItemColumn::Cost);
            emit dataChanged(cost_value_index, cost_value_index, {Qt::DisplayRole});

            auto cost_currency_index = sibling(idx, StepItemColumn::CostCurrency);
            emit dataChanged(cost_currency_index,
                             cost_currency_index,
                             {Qt::DisplayRole, Qt::DecorationRole});

            auto gold_index = sibling(idx, StepItemColumn::Gold);
            emit dataChanged(gold_index, gold_index, {Qt::DisplayRole});

            auto time_index = sibling(idx, StepItemColumn::Time);
            emit dataChanged(time_index, time_index, {Qt::DisplayRole});

            return true;
        }
        return false;
    }
    case StepItemColumn::Time: {
        auto str = value.toString();
        if (str.isEmpty()) {
            if (!trade.time)
                return false;
            trade.time.reset();
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            return true;
        }

        auto val = AppState::state.arithmetic_calc->calculate(str);
        if (!val || *val < 0.0 || *val == trade_cache->time(trade).count())
            return false;
        trade.time = ItemTime{*val};
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    }
    default:
        return false;
    }
}

QVariant StepItemModel::customItemData(double amount,
                                       const CustomItemData& custom,
                                       StepItemColumn col,
                                       int role) const
{
    auto& cache = exchange_cache->cache;
    switch (col) {
    case StepItemColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return custom.name;
        }
        return {};
    case StepItemColumn::Link:
        switch (role) {
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return custom.link;
        case Qt::ForegroundRole:
            if (QUrl::fromUserInput(custom.link).isValid())
                return QColor{0x0000EE};
            return {};
        }
        return {};
    case planner::StepItemColumn::Cost:
        switch (role) {
        case Qt::DisplayRole:
            return formatCost(custom.cost.value);
        case Qt::EditRole:
            return custom.cost.value;
        case Qt::ToolTipRole:
            return custom.cost.value != 0.0 ? QString::number(amount * custom.cost.value)
                                            : QVariant{};
        }
        return {};
    case planner::StepItemColumn::CostCurrency:
        switch (role) {
        case Qt::DecorationRole:
            if (auto it = exchange_cache->currencyData(custom.cost.currency); it != cache.end())
                return exchange_cache->icon(it);
            return {};
        case Qt::EditRole:
        case Qt::DisplayRole:
            if (auto it = exchange_cache->currencyData(custom.cost.currency); it != cache.end())
                return exchange_cache->name(it);
            return {};
        }
        return {};
    case planner::StepItemColumn::Gold:
        switch (role) {
        case Qt::DisplayRole:
            return formatGold(custom.gold);
        case Qt::EditRole:
            return custom.gold;
        }
        return {};
    case planner::StepItemColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
            return formatTime(custom.time);
        case Qt::EditRole:
            return QString::number(custom.time.count());
        }
        return {};
    default:
        return {};
    }
}

bool StepItemModel::setCustomItemData(CustomItemData& custom,
                                      const QVariant& value,
                                      const QModelIndex& idx)
{
    auto col = static_cast<StepItemColumn>(idx.column());
    switch (col) {
    case StepItemColumn::Name:
        custom.name = value.toString();
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    case StepItemColumn::Link:
        custom.link = value.toString();
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    case StepItemColumn::Cost:
        if (auto val = value.toDouble(); val >= 0.0) {
            custom.cost.value = val;
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            return true;
        }
        return false;
    case StepItemColumn::CostCurrency: {
        auto selected_index = value.toModelIndex();
        if (selected_index.isValid()) {
            custom.cost.currency.id = exchange_cache->cache.nth(selected_index.row())->first;
            custom.cost.currency.cache_pos = selected_index.row();
            emit dataChanged(idx, idx, {Qt::DisplayRole, Qt::DecorationRole});
            return true;
        }
        return false;
    }
    case StepItemColumn::Gold:
        if (auto val = value.toDouble(); val >= 0.0) {
            custom.gold = val;
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            return true;
        }
        return false;
    case StepItemColumn::Time: {
        auto val = AppState::state.arithmetic_calc->calculate(value.toString());
        if (!val || *val < 0.0 || *val == custom.time.count())
            return false;
        custom.time = ItemTime{*val};
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    }
    default:
        return false;
    }
}

QVariant StepItemModel::stepItemData(double amount,
                                     const StepItemData& step_item,
                                     StepItemColumn col,
                                     int role) const
{
    auto step_it = plan_->findStepIt(step_item.step_id);
    if (step_it == plan_->steps.end())
        return {};

    auto& cache = exchange_cache->cache;
    switch (col) {
    case StepItemColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
            return step_it->name;
        case Qt::EditRole:
            return std::distance(plan_->steps.cbegin(), step_it);
        }
        return {};
    case StepItemColumn::Link:
        switch (role) {
        case Qt::ForegroundRole:
            return QColor{0x0000EE};
        }
        return {};
    case StepItemColumn::Cost:
        switch (role) {
        case Qt::DisplayRole:
            if (auto cost = step_it->costCurrency(); cost.currency.isValid())
                return formatCost(cost.value);
            return {};
        case Qt::ToolTipRole:
            if (auto cost = step_it->costCurrency(); cost.currency.isValid() && cost.value > 0.0)
                return QString::number(amount * cost.value);
            return {};
        }
        return {};
    case StepItemColumn::CostCurrency:
        switch (role) {
        case Qt::DecorationRole: {
            auto cost = step_it->costCurrency();
            if (auto it = exchange_cache->currencyData(cost.currency); it != cache.end())
                return exchange_cache->icon(it);
            return {};
        }
        case Qt::DisplayRole: {
            auto cost = step_it->costCurrency();
            if (auto it = exchange_cache->currencyData(cost.currency); it != cache.end())
                return exchange_cache->name(it);
            return {};
        }
        }
        return {};
    case StepItemColumn::Gold:
        switch (role) {
        case Qt::DisplayRole:
            return formatGold(step_it->costGold());
        }
        return {};
    case StepItemColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
            return formatTime(step_it->costTime());
        }
        return {};
    default:
        return {};
    }
}

bool StepItemModel::setStepItemData(StepItemData& step_item,
                                    const QVariant& value,
                                    const QModelIndex& idx)
{
    auto col = static_cast<StepItemColumn>(idx.column());
    if (col != StepItemColumn::Name)
        return false;

    auto new_pos = value.toULongLong();
    if (new_pos >= plan_->steps.size() || new_pos >= step_pos)
        return false;

    step_item.step_id = plan_->steps[new_pos].id;
    auto right_idx = sibling(idx, StepItemColumn::Time);
    emit dataChanged(idx, right_idx);
    return true;
}

QVariant StepItemModel::planItemData(double amount,
                                     const PlanItemData& plan_item,
                                     StepItemColumn col,
                                     int role) const
{
    auto& plans = plan_model->plans;
    auto plan_it = plans.find(plan_item.plan_id);
    if (plan_it == plans.end()) {
        if (col == StepItemColumn::Name && !plan_item.name.isEmpty()
            && (role == Qt::DisplayRole || role == Qt::EditRole))
            return plan_item.name;
        return {};
    }

    auto& cache = exchange_cache->cache;
    switch (col) {
    case StepItemColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return !plan_item.name.isEmpty() ? plan_item.name : plan_it->second.name;
        }
        return {};
    case StepItemColumn::Link:
        switch (role) {
        case Qt::EditRole:
            return plan_it->second.name;
        case Qt::ForegroundRole:
            return QColor{0x0000EE};
        }
        return {};
    case StepItemColumn::Cost:
        switch (role) {
        case Qt::DisplayRole: {
            if (auto cost_step = plan_it->second.costStep()) {
                if (auto cost = cost_step->costCurrency(); cost.currency.isValid())
                    return formatCost(cost.value);
            }
            return {};
        }
        case Qt::ToolTipRole:
            if (auto cost_step = plan_it->second.costStep()) {
                if (auto cost = cost_step->costCurrency();
                    cost.currency.isValid() && cost.value > 0.0)
                    return QString::number(amount * cost.value);
            }
            return {};
        }
        return {};
    case StepItemColumn::CostCurrency:
        switch (role) {
        case Qt::DecorationRole:
            if (auto cost_step = plan_it->second.costStep()) {
                auto cost = cost_step->costCurrency();
                if (auto it = exchange_cache->currencyData(cost.currency); it != cache.end())
                    return exchange_cache->icon(it);
            }
            return {};
        case Qt::DisplayRole:
            if (auto cost_step = plan_it->second.costStep()) {
                auto cost = cost_step->costCurrency();
                if (auto it = exchange_cache->currencyData(cost.currency); it != cache.end())
                    return exchange_cache->name(it);
            }
            return {};
        }
        return {};
    case StepItemColumn::Gold:
        switch (role) {
        case Qt::DisplayRole:
            if (auto cost_step = plan_it->second.costStep()) {
                return formatGold(cost_step->costGold());
            }
            return {};
        }
        return {};
    case StepItemColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
            if (auto cost_step = plan_it->second.costStep()) {
                return formatTime(cost_step->costTime());
            }
            return {};
        }
        return {};
    default:
        return {};
    }
}

bool StepItemModel::setPlanItemData(PlanItemData& plan_item,
                                    const QVariant& value,
                                    const QModelIndex& idx)
{
    auto col = static_cast<StepItemColumn>(idx.column());
    switch (col) {
    case StepItemColumn::Name:
        if (auto name = value.toString(); name != plan_item.name) {
            plan_item.name = name;
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            return true;
        }
        return false;
    case StepItemColumn::Link:
        if (auto plan_id = value.toUuid(); plan_id != plan_item.plan_id) {
            plan_item.plan_id = plan_id;
            auto name_idx = index(idx.row(), StepItemColumn::Name);
            auto time_idx = index(idx.row(), StepItemColumn::Time);
            emit dataChanged(name_idx, time_idx);
            return true;
        }
        return false;
    default:
        return false;
    }
}

QVariant StepItemModel::formatCostWithRatio(double cost)
{
    if (cost <= 0.0)
        return {};
    if (cost < 1.0) {
        auto rate = std::ceil(10.0 / cost) / 10.0;
        return u"1/%1"_s.arg(QString::number(rate));
    }
    cost = std::ceil(10.0 * cost) / 10.0;
    return QString::number(cost);
}

QVariant StepItemModel::formatCost(double cost)
{
    if (cost <= 0.0)
        return {};
    cost = std::ceil(10.0 * cost) / 10.0;
    return QString::number(cost);
}

QVariant StepItemModel::formatGold(double gold)
{
    if (gold <= 0.0)
        return {};
    if (gold >= 1e6)
        gold = std::ceil(gold / 1e5) * 1e5;
    else
        gold = std::ceil(gold);

    return QString::number(gold);
}

QVariant StepItemModel::formatTime(ItemTime time)
{
    auto seconds = time.count();
    if (seconds <= 0.0)
        return {};
    seconds = std::ceil(10.0 * seconds) / 10.0;
    return QString::number(seconds, 'g', 5);
}

void StepItemModel::setStep(Plan* plan, size_t step_pos_)
{
    beginResetModel();
    plan_ = plan;
    this->step_pos = step_pos_;
    if (plan) {
        if (plan->game == Game::Poe1) {
            exchange_cache = AppState::state.exchange_cache_poe1;
            trade_cache = AppState::state.trade_cache_poe1;
            plan_model = AppState::state.plan_model_poe1;
        } else {
            exchange_cache = AppState::state.exchange_cache_poe2;
            trade_cache = AppState::state.trade_cache_poe2;
            plan_model = AppState::state.plan_model_poe2;
        }
    }
    endResetModel();
}

void StepItemModel::updateCosts()
{
    if (!plan_)
        return;
    auto& items = stepItems();
    if (items.empty())
        return;

    auto top_left = index(0, StepItemColumn::Cost);
    auto bottom_right = index(std::ssize(items) - 1, StepItemColumn::Time);
    emit dataChanged(top_left, bottom_right);
}

void StepItemModel::clearStep(const QUuid& deleted_step)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto step = items[i].step(); step && step->step_id == deleted_step) {
            auto idx = index(i, StepItemColumn::Name);
            auto right_idx = sibling(idx, StepItemColumn::Time);
            emit dataChanged(idx, right_idx);
        }
    }
}

void StepItemModel::updateStepName(const QUuid& changed_step)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto step = items[i].step(); step && step->step_id == changed_step) {
            auto idx = index(i, StepItemColumn::Name);
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

void StepItemModel::updatePlanName(const QUuid& changed_plan)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto plan = items[i].plan();
            plan && plan->name.isEmpty() && plan->plan_id == changed_plan) {
            auto idx = index(i, StepItemColumn::Name);
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

QModelIndex StepItemModel::insertItem(const QModelIndex& idx, StepItemType type)
{
    if (!plan_)
        return {};

    auto& items = stepItems();
    auto row = idx.row() < 0 || idx.row() > std::ssize(items) ? std::ssize(items) : idx.row();

    beginInsertRows({}, row, row);

    items.insert(items.begin() + row, 1, {});
    items[row].setType(type);
    if (row > 0)
        items[row].amount = items[row - 1].amount;

    if (auto step = items[row].step(); step && step_pos > 0)
        step->step_id = plan_->steps[step_pos - 1].id;
    plan_->setChanged();

    endInsertRows();

    return index(row, StepItemColumn::Amount);
}

QModelIndex StepItemModel::duplicateItem(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return {};

    auto& items = stepItems();

    auto pos = idx.row() + 1;
    beginInsertRows({}, pos, pos);
    items.emplace(items.begin() + pos, items[idx.row()]);
    endInsertRows();
    plan_->setChanged();

    return index(pos, StepItemColumn::Amount);
}

void StepItemModel::copyItem(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return;

    StepItemCopyState::state = {plan_->game, stepItems()[idx.row()]};
}

QModelIndex StepItemModel::pasteItem(const QModelIndex& idx)
{
    if (!plan_ || !StepItemCopyState::haveCopy(plan_->game))
        return {};

    auto row = idx.isValid() ? idx.row() : rowCount();
    auto& items = stepItems();
    beginInsertRows({}, row, row);
    items.emplace(items.begin() + row, StepItemCopyState::state.item);
    endInsertRows();
    plan_->setChanged();

    return index(row, StepItemColumn::Amount);
}

bool StepItemModel::haveRegex(const StepItem& item) const
{
    if (auto trade = item.trade()) {
        auto it = trade_cache->requestData(trade->request_key);
        if (it != trade_cache->cache.end() && !it->second.regex().isEmpty())
            return true;
    }
    return false;
}

void StepItemModel::copyRegex(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return;

    if (auto trade = stepItems()[idx.row()].trade()) {
        if (auto it = trade_cache->requestData(trade->request_key);
            it != trade_cache->cache.end() && !it->second.regex().isEmpty())
            qApp->clipboard()->setText(it->second.regex());
    }
}

void StepItemModel::copyLink(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return;

    if (auto trade = stepItems()[idx.row()].trade()) {
        if (trade->request_key.isValid())
            qApp->clipboard()->setText(trade->request_key.toUrl(plan_->game));
    }
}

void StepItemModel::setTradeRequest(int row, const TradeRequestKey& request)
{
    if (!plan_)
        return;
    auto& items = stepItems();
    if (row > std::ssize(items))
        return;

    if (auto trade = items[row].trade()) {
        if (trade->request_key != request) {
            trade->request_key = request;
            auto idx = index(row, StepItemColumn::Name);
            emit dataChanged(idx, sibling(idx, StepItemColumn::Time));
        }
    }
}

void StepItemModel::deleteSearch(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return;

    if (auto trade = stepItems()[idx.row()].trade()) {
        auto request = trade->request_key;
        trade_cache->deleteRequest(request);
    }
}

void StepItemModel::setDefaultTime(const QModelIndex& idx)
{
    if (!plan_ || !idx.isValid())
        return;

    auto& items = stepItems();
    if (auto trade = items[idx.row()].trade())
        trade_cache->setDefaultTime(trade->request_key, trade->time);
    else if (auto exchange = items[idx.row()].exchange())
        exchange_cache->setDefaultTime(exchange->currency, exchange->time);
}

void StepItemModel::openLink(const QModelIndex& idx, bool need_window)
{
    auto& item = stepItems()[idx.row()];
    if (auto step = item.step())
        emit stepLinkClicked(step->step_id);
    else if (auto plan = item.plan())
        emit planLinkClicked(plan->plan_id, this->plan_->game, need_window);
    else {
        auto url = QUrl::fromUserInput(data(idx, Qt::ToolTipRole).toString());
        if (url.isValid())
            QDesktopServices::openUrl(url);
    }
}

Game StepItemModel::game() const
{
    return plan_ ? plan_->game : Game::Unknown;
}

void StepItemModel::clearTradeRequest(const TradeRequestKey& request)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto trade = items[i].trade(); trade && trade->request_key == request) {
            trade->request_key = {};
            plan_->setChanged();
            auto idx = index(i, StepItemColumn::Name);
            auto time_idx = sibling(idx, StepItemColumn::Time);
            emit dataChanged(idx, time_idx);
        }
    }
}

void StepItemModel::updateTradeName(const TradeRequestKey& request)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto trade = items[i].trade();
            trade && trade->name.isEmpty() && trade->request_key == request) {
            auto idx = index(i, StepItemColumn::Name);
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

void StepItemModel::updateTime(const TradeRequestKey& request)
{
    if (!plan_)
        return;

    auto& items = stepItems();
    bool is_valid = request.isValid();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto trade = items[i].trade();
            trade && !trade->time && (!is_valid || trade->request_key == request)) {
            auto idx = index(i, StepItemColumn::Time);
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

void StepItemModel::updateTime(const Currency& currency)
{
    if (!plan_)
        return;

    auto& items = stepItems();

    bool is_valid = currency.isValid();
    for (size_t i = 0; i < items.size(); ++i) {
        if (auto exchange = items[i].exchange();
            exchange && !exchange->time && (!is_valid || exchange->currency.id == currency.id)) {
            auto idx = index(i, StepItemColumn::Time);
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
    }
}

} // namespace planner
