#ifndef PLAN_H
#define PLAN_H

#include <vector>
#include <QJsonObject>
#include <QString>
#include <QUuid>

#include "Step.h"

namespace planner {

class PlanItem;

class Plan
{
public:
    Plan(QUuid id, QString name);
    Plan(QUuid id, const QJsonObject& plan_o, const ExchangeRequestCache& cache);

    Plan(QUuid id, const Plan& o);
    Plan(const Plan&) = delete;
    Plan& operator=(const Plan&) = delete;
    Plan(Plan&&) = default;
    Plan& operator=(Plan&&) = default;

    QJsonObject saveJson() const;
    QJsonObject exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const;

    QString name;

    Game game;
    QString league;
    using Steps = std::vector<Step>;
    Steps steps;

    bool is_changed{true};

    PlanItem* item() const { return item_; }
    QUuid id() const { return id_; }

    void setChanged();

    Steps::const_iterator findStepIt(const QUuid& step_id) const;
    const Step* findStep(const QUuid& step_id) const;

    const QUuid& finalStepId() const { return final_step; }
    const Step* costStep() const;
    void setFinalStep(const QUuid& step_id)
    {
        if (step_id == final_step)
            return;
        final_step = step_id;
        setChanged();
    }

    QStringList stepsName(size_t last) const;

private:
    QUuid id_;
    QUuid final_step{};

    PlanItem* item_{};
    QUuid changeId();
    friend class PlanModel;
    friend class PlanItem;
};

} // namespace planner
#endif // PLAN_H
