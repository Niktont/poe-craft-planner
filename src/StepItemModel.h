#ifndef STEPITEMMODEL_H
#define STEPITEMMODEL_H

#include "StepItem.h"
#include <vector>
#include <QAbstractTableModel>

namespace planner {
class ExchangeRequestCache;
class TradeRequestCache;
class ExchangeItemData;
class TradeItemData;
class CustomItemData;
class Plan;
class Step;
class StepItem;
class PlanModel;

enum class StepItemColumn {
    Row,
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

class StepItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit StepItemModel(bool is_resource_model, QObject* parent = nullptr);

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

    static const QString move_mime_poe1;
    static const QString move_mime_poe2;

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

    static std::pair<StepItemModel*, std::vector<StepItem*>> decodeStepItemsMime(
        Game game, const QMimeData& data);

    const bool is_resource_model{};
    size_t stepPos() const { return step_pos; }
    const StepItem* stepItem(const QModelIndex& idx) const;

    void clearTradeRequest(const planner::TradeRequestKey& request);
    void updateTradeName(const planner::TradeRequestKey& request);
    void updateTime(const planner::TradeRequestKey& request);
    void updateTime(const planner::Currency& currency);

    void clearStep(const QUuid& deleted_step);
    void updateStepName(const QUuid& changed_step);
    void updatePlanName(const QUuid& changed_plan);

    void updatePos(size_t new_pos) { step_pos = new_pos; }
    void setStep(planner::Plan* plan, size_t step_pos);
    void updateCosts();

    QModelIndex insertItem(const QModelIndex& idx, planner::StepItemType type);
    QModelIndex duplicateItem(const QModelIndex& idx);
    void copyItem(const QModelIndex& idx);
    QModelIndex pasteItem(const QModelIndex& idx);

    bool haveRegex(const StepItem& item) const;
    void copyRegex(const QModelIndex& idx);
    void copyLink(const QModelIndex& idx);

    void setTradeRequest(int row, const TradeRequestKey& request);
    void deleteSearch(const QModelIndex& idx);
    void setDefaultTime(const QModelIndex& idx);

    void openLink(const QModelIndex& idx, bool need_window);

    Game game() const;

    Step* step();
    const Step* step() const { return const_cast<StepItemModel*>(this)->step(); };

    PlanModel* planModel() const { return plan_model; }

signals:
    void planLinkClicked(const QUuid& plan_id, planner::Game game, bool need_window) const;
    void stepLinkClicked(const QUuid& step_id) const;

private slots:
    void updateRowNumbers(const QModelIndex& idx, int first, int last);

private:
    friend class StepItemDelegate;

    Plan* plan_{};
    size_t step_pos{};
    ExchangeRequestCache* exchange_cache{};
    TradeRequestCache* trade_cache{};
    PlanModel* plan_model{};

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

    QVariant planItemData(double amount,
                          const PlanItemData& plan_item,
                          StepItemColumn col,
                          int role) const;
    bool setPlanItemData(PlanItemData& plan_item, const QVariant& value, const QModelIndex& idx);

    static QVariant formatCostWithRatio(double cost);
    static QVariant formatCost(double cost);
    static QVariant formatGold(double gold);
    static QVariant formatTime(ItemTime time);

    void moveItems(int dest_row, const QMimeData& data);
    void addPlanItems(int dest_row, const QMimeData& data);
};

} // namespace planner

#endif // STEPITEMMODEL_H
