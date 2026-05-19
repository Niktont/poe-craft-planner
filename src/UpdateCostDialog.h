#ifndef UPDATECOSTDIALOG_H
#define UPDATECOSTDIALOG_H

#include "Game.h"
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
class ExchangeRequester;
class TradeRequester;

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

private:
    QLabel* progress_label;
    QPushButton* cancel_button;
    QListView* empty_results_view;

    bool isUpdateActive() const { return game_ != Game::Unknown; }
    Game game_{Game::Unknown};

    std::vector<QUuid> dependencies;

    TradeRequester* trade_requester;
    ExchangeRequester* exchange_requester;
    size_t request_count{};

    std::unordered_set<QString> empty_search_results;

    void startUpdate(bool send_requests);

    void checkCurrency(const Currency& currency,
                       QDateTime now,
                       const ExchangeRequestCache& exchange_cache);
    void checkItem(const StepItem& item,
                   QDateTime now,
                   const ExchangeRequestCache& exchange_cache,
                   const TradeRequestCache& trade_cache);
    void requestFailed();
    void parseFailed();

    void updateProgress();

    void calculateCost();
    bool calculateStepCost(const Plan& step_plan, Step& step);
    bool calculateStepCustomCost(bool is_resource_cost, const Plan& step_plan, Step& step);
};

} // namespace planner

#endif // UPDATECOSTDIALOG_H
