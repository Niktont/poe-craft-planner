#ifndef SHOPPINGMODEL_H
#define SHOPPINGMODEL_H

#include "ShoppingItem.h"
#include <boost/container/flat_map.hpp>
#include <map>
#include <vector>
#include <QAbstractTableModel>

namespace planner {
class Plan;
class Step;
class ExchangeRequestCache;
class TradeRequestCache;
class TradeItemData;
class PlanItemData;
class StepItem;
class PlanModel;

enum class ShoppingColumn {
    Amount,
    Name,
    Link,

    last = Link,
};

class ShoppingModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ShoppingModel(QObject* parent = nullptr);

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    const ShoppingItem& item(size_t row) const { return items[row]; }

    bool setPlan(const Plan& plan, size_t step_pos, double amount, bool include_dependencies);
    const Plan* plan() const { return plan_; }

    const ExchangeRequestCache* exchangeCache() const { return exchange_cache; }
    const TradeRequestCache* tradeCache() const { return trade_cache; }

private:
    const Plan* plan_{};
    const PlanModel* plan_model{};
    const ExchangeRequestCache* exchange_cache{};
    const TradeRequestCache* trade_cache{};

    bool include_dependencies{true};
    std::vector<ShoppingItem> items;

    using ExchangeItems = std::map<Currency, double, CurrencyIdLess>;
    using TradeItems = std::map<TradeRequestKey, std::pair<double, QString>>;

    struct PlanData
    {
        std::vector<std::pair<const Step*, double>> steps;
        ExchangeItems exchange_items;
        TradeItems trade_items;

        void addStep(const Step* step, double amount);
        void mergeItems(double amount, const PlanData& data);
    };
    std::map<QUuid, PlanData> dependencies;

    bool gatherPlanItems(size_t step_pos, double amount);
    void gatherItems(const Plan& plan, PlanData& data);

    void gatherPlanItem(double amount, const PlanItemData& plan_item, PlanData& data);
    void gatherCurrency(double amount, const Currency& currency, ExchangeItems& exchange_items);
    void gatherTradeItem(double amount,
                         const TradeItemData& trade,
                         ExchangeItems& exchange_items,
                         TradeItems& trade_items);
};

} // namespace planner

#endif // SHOPPINGMODEL_H
