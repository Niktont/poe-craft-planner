#ifndef UPDATECOSTDIALOG_H
#define UPDATECOSTDIALOG_H

#include "Game.h"
#include "HashFunctions.h"
#include <boost/unordered_set.hpp>
#include <QDialog>
#include <QListView>

class QLabel;
class QPushButton;
class QNetworkReply;

namespace planner {
class Plan;
class PlanItem;
class ExchangeRequestCache;
class Currency;
class Step;
class StepItem;
class TradeRequestKey;
class TradeRequestCache;

class UpdateCostDialog : public QDialog
{
    Q_OBJECT
public:
    UpdateCostDialog(QWidget* parent = nullptr);

    void updatePlanItem(PlanItem& item, bool send_requests);
    void updatePlan(Plan& plan, bool send_requests);

signals:
    void costUpdated(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

public slots:
    void cancelUpdate();

private slots:
    void requestTradeSearch();
    void requestExchangeCost();

private:
    QLabel* progress_label;
    QPushButton* cancel_button;
    QListView* empty_results_view;

    bool isUpdateActive() const { return game_ != Game::Unknown; }
    Game game_{Game::Unknown};

    std::vector<QUuid> dependencies;

    void parseTradeSearch(Game game, const TradeRequestKey& request, QNetworkReply* reply);
    void parseFetchSearch(Game game,
                          const TradeRequestKey& request,
                          int total,
                          QNetworkReply* reply);
    void parseExchangeCostData(Game game, QNetworkReply* reply);

    std::set<TradeRequestKey> trade_requests;
    boost::unordered_set<QString> exchange_requests;

    boost::unordered_set<QString> empty_search_results;

    bool is_active_trade{false};
    bool is_active_exchange{false};

    bool trade_finished{false};
    bool exchange_finished{false};

    void startUpdate(bool send_requests);

    void checkCurrency(const Currency& currency,
                       QDateTime now,
                       const ExchangeRequestCache& exchange_cache);
    void checkItem(const StepItem& item,
                   QDateTime now,
                   const ExchangeRequestCache& exchange_cache,
                   const TradeRequestCache& trade_cache);
    void clearRequests();
    void requestFailed();
    void parseFailed();

    void updateProgress();
    void calculateCost();
    bool calculateStepCost(const Plan& step_plan, Step& step);
    bool calculateStepCustomCost(bool is_resource_cost, const Plan& step_plan, Step& step);
};

} // namespace planner

#endif // UPDATECOSTDIALOG_H
