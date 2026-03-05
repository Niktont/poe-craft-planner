#ifndef STEPITEMMODEL_H
#define STEPITEMMODEL_H

#include "StepItem.h"
#include <vector>
#include <QAbstractTableModel>

namespace planner {

class ExchangeItemData;
class TradeItemData;
class CustomItemData;
class Plan;
class Step;
class StepItem;

enum class StepItemColumn {
    Type,
    Amount,
    Name,
    Link,
    Cost,
    CostCurrency,
    Gold,
    Time,
    Success,

    last = Success,
};

class PlanWidget;
class ExchangeRequestCache;
class TradeRequestCache;
class MainWindow;

class StepItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit StepItemModel(bool is_resource_model, PlanWidget& plan_widget);

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
    bool moveRows(const QModelIndex& sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex& destinationParent,
                  int destinationChild) override;
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    using QAbstractTableModel::index;
    QModelIndex index(int row, StepItemColumn column) const;
    using QAbstractTableModel::sibling;
    QModelIndex sibling(const QModelIndex& idx, StepItemColumn column) const
    {
        return idx.siblingAtColumn(static_cast<int>(column));
    }

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction; }
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex& parent) override;

    const bool is_resource_model{};
    size_t stepPos() const { return step_pos; }
    const StepItem* stepItem(const QModelIndex& idx) const;

    void clearTradeRequest(const planner::TradeRequestKey& request);
    void updateTradeName(const planner::TradeRequestKey& request);
    void updateTime(const planner::TradeRequestKey& request);
    void updateTime(const planner::Currency& currency);
    void updateStepName(const QUuid& changed_step, bool deleted);
    void updatePos(size_t new_pos) { step_pos = new_pos; }
    void setStep(planner::Plan* plan, size_t step_pos);
    void updateCosts();

    void insertItem(const QModelIndex& idx, planner::StepItemType type);
    void duplicateItem(const QModelIndex& idx);

    void openSearch(const QModelIndex& idx);
    void setDefaultTime(const QModelIndex& idx);

private:
    friend class StepItemDelegate;

    Plan* plan{};
    size_t step_pos{};
    ExchangeRequestCache* exchange_cache{};
    TradeRequestCache* trade_cache{};

    Step* step();
    const Step* step() const { return const_cast<StepItemModel*>(this)->step(); };
    std::vector<StepItem>& stepItems();
    const std::vector<StepItem>& stepItems() const;

    void setItemType(const QModelIndex& index, StepItemType type);

    QVariant exchangeItemData(double amount,
                              const ExchangeItemData& exchange,
                              StepItemColumn col,
                              int role) const;
    bool setExchangeItemData(ExchangeItemData& exchange,
                             const QVariant& value,
                             const QModelIndex& idx);

    QVariant tradeItemData(double amount,
                           const TradeItemData& trade,
                           StepItemColumn col,
                           int role) const;
    bool setTradeItemData(TradeItemData& trade, const QVariant& value, const QModelIndex& idx);

    QVariant customItemData(double amount,
                            const CustomItemData& custom,
                            StepItemColumn col,
                            int role) const;
    bool setCustomItemData(CustomItemData& custom, const QVariant& value, const QModelIndex& idx);

    QVariant stepItemData(double amount,
                          const StepItemData& step_item,
                          StepItemColumn col,
                          int role) const;
    bool setStepItemData(StepItemData& step_item, const QVariant& value, const QModelIndex& idx);

    static QVariant formatCostWithRatio(double cost);
    static QVariant formatCost(double cost);
    static QVariant formatGold(double gold);
    static QVariant formatTime(ItemTime time);

    MainWindow* mw() const;
};

} // namespace planner

#endif // STEPITEMMODEL_H
