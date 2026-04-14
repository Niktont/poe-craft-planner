#ifndef APPSTATE_H
#define APPSTATE_H

#include "Game.h"

namespace planner {
class MainWindow;
class TradeRequestManager;
class ExchangeRequestManager;

class PlanModel;
class ExchangeRequestCache;
class TradeRequestCache;
class SnapshotModel;

class RequestEditDialog;
class UpdateCostDialog;
class ShoppingDialog;
class CustomEditDialog;
class PlanSearchDialog;
#ifndef PLANNER_NO_BROWSER
class WebViewDialog;
#endif

class AppState
{
public:
    MainWindow* mw{};

    PlanModel* plan_model_poe1{};
    PlanModel* plan_model_poe2{};

    TradeRequestManager* trade_manager{};
    TradeRequestCache* trade_cache_poe1{};
    TradeRequestCache* trade_cache_poe2{};

    ExchangeRequestManager* exchange_manager{};
    ExchangeRequestCache* exchange_cache_poe1{};
    ExchangeRequestCache* exchange_cache_poe2{};

    SnapshotModel* snapshots_poe1{};
    SnapshotModel* snapshots_poe2{};

    RequestEditDialog* request_edit_dialog{};
    UpdateCostDialog* update_cost_dialog{};
    ShoppingDialog* shopping_dialog{};
    CustomEditDialog* custom_edit_dialog{};
    PlanSearchDialog* plan_search_dialog{};
#ifndef PLANNER_NO_BROWSER
    WebViewDialog* web_view_dialog;
#endif

    static PlanModel* planModel(Game game)
    {
        return game == Game::Poe1 ? state.plan_model_poe1 : state.plan_model_poe2;
    }
    static TradeRequestCache* tradeCache(Game game)
    {
        return game == Game::Poe1 ? state.trade_cache_poe1 : state.trade_cache_poe2;
    }
    static ExchangeRequestCache* exchangeCache(Game game)
    {
        return game == Game::Poe1 ? state.exchange_cache_poe1 : state.exchange_cache_poe2;
    }
    static SnapshotModel* snapshots(Game game)
    {
        return game == Game::Poe1 ? state.snapshots_poe1 : state.snapshots_poe2;
    }

    static AppState state;
};

} // namespace planner

#endif // APPSTATE_H
