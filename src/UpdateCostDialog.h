#ifndef UPDATECOSTDIALOG_H
#define UPDATECOSTDIALOG_H

#include "StepItem.h"
#include "TradeRequestCache.h"
#include <boost/unordered_set.hpp>
#include <queue>
#include <QDialog>
#include <QListView>

class QLabel;
class QPushButton;
class QNetworkReply;

namespace planner {
class MainWindow;
class Plan;
class ExchangeRequestCache;
class Currency;
class Step;

class UpdateCostDialog : public QDialog
{
    Q_OBJECT
public:
    UpdateCostDialog(MainWindow& mw);

signals:
    void costUpdated(planner::Plan* plan);

public slots:
    void updatePlan(planner::Plan* plan);
    void cancelUpdate();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void requestTradeSearch();
    void requestExchangeCost();

private:
    QLabel* progress_label;
    QPushButton* cancel_button;
    QListView* empty_results_view;

    Plan* plan{};

    void parseTradeSearch(Game game, const TradeRequestKey& request, QNetworkReply* reply);
    void parseFetchSearch(Game game,
                          const TradeRequestKey& request,
                          int total,
                          QNetworkReply* reply);
    void parseExchangeCostData(Game game, QNetworkReply* reply);

    std::queue<TradeRequestCache::Cache::const_iterator> trade_requests;
    boost::unordered_set<QString> exchange_requests;

    boost::unordered_set<QString> empty_search_results;

    bool is_active_trade{false};
    bool is_active_exchange{false};

    bool trade_finished{false};
    bool exchange_finished{false};

    MainWindow* mw() const;
    void checkCurrency(const Currency& currency, QDateTime now);
    void checkItem(const StepItem& item, QDateTime now, const TradeRequestCache& trade_cache);
    void clearRequests();
    void requestFailed();
    void parseFailed();

    void updateProgress();
    void calculateCost();
    void calculateStepCost(Step& step);
};

} // namespace planner

#endif // UPDATECOSTDIALOG_H
