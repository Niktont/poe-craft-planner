#ifndef SHOPPINGMODEL_H
#define SHOPPINGMODEL_H

#include "ShoppingItem.h"
#include <map>
#include <vector>
#include <QAbstractTableModel>

namespace planner {
class Plan;
class Step;
class ExchangeRequestCache;
class TradeRequestCache;
class MainWindow;
class TradeItemData;
class StepItem;

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
    explicit ShoppingModel(MainWindow& mw, QObject* parent = nullptr);

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& /*parent*/ = {}) const override { return items.size(); }
    int columnCount(const QModelIndex& /*parent*/ = {}) const override
    {
        return static_cast<int>(ShoppingColumn::last) + 1;
    }

    QVariant data(const QModelIndex& index, int role) const override;
    const ShoppingItem& item(size_t row) const { return items[row]; }

    bool setPlan(Plan* plan);
    Plan* plan() const { return plan_; }

    ExchangeRequestCache* exchangeCache() const { return exchange_cache; }
    TradeRequestCache* tradeCache() const { return trade_cache; }

private:
    Plan* plan_{};
    ExchangeRequestCache* exchange_cache{};
    TradeRequestCache* trade_cache{};

    std::vector<ShoppingItem> items;

    bool gatherPlanItems();

    using ExchangeItems = std::map<Currency, double, CurrencyIdLess>;
    using TradeItems = std::map<TradeRequestKey, std::pair<double, QString>>;
    void gatherStepItems(double amount,
                         std::vector<Step>::const_iterator step_it,
                         ExchangeItems& exchange_items,
                         TradeItems& trade_items);

    void gatherItem(double amount,
                    ptrdiff_t pos,
                    const StepItem& item,
                    ExchangeItems& exchange_items,
                    TradeItems& trade_items);
    void gatherCurrency(double amount, const Currency& currency, ExchangeItems& exchange_items);
    void gatherTradeItem(double amount,
                         const TradeItemData& trade,
                         ExchangeItems& exchange_items,
                         TradeItems& trade_items);

    MainWindow* mw;
};

} // namespace planner

#endif // SHOPPINGMODEL_H
