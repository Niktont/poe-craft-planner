#ifndef PLANITEM_H
#define PLANITEM_H

#include <memory>
#include <vector>
#include <QSqlQuery>
#include <QString>
#include <QUuid>
#include <QVariant>

namespace planner {
class Plan;
class PlanModel;
class ExchangeRequestCache;
class TradeRequestCache;

class PlanItem
{
public:
    PlanItem(QString name, PlanModel& model, PlanItem* parent);
    PlanItem(QUuid id, QSqlQuery& select, PlanModel& model, PlanItem* parent);
    PlanItem(Plan* plan, PlanModel& model, PlanItem* parent);
    // Import
    PlanItem(bool is_folder, const QJsonObject& item_o, PlanModel& model, PlanItem* parent);

    explicit PlanItem(const PlanItem& item);
    explicit PlanItem(PlanItem&& item) = delete;
    PlanItem& operator=(const PlanItem& item) = delete;
    PlanItem& operator=(PlanItem&& item) noexcept;

    QJsonObject saveJson() const;
    QJsonObject exportJson(const ExchangeRequestCache& cache,
                           TradeRequestCache& trade_cache,
                           std::vector<QUuid>* plans_to_check) const;

    void replacePlan(int row, Plan&& new_plan);

    PlanItem* restoreChild(int row);
    void insertCopy(int row, const PlanItem& copy_item);

    QModelIndex index(int column = 0) const;

    PlanItem& child(int row) { return *childs[row]; }
    const PlanItem& child(int row) const { return *childs[row]; }

    int childCount() const { return childs.size(); }

    QVariant data(int column, int role) const;
    bool setData(int column, const QVariant& value, int role);

    int row() const;

    QString path() const;
    QString shortPath() const;

    int isAncestor(const PlanItem& item) const;
    bool isDescendant(const PlanItem& item) const;
    bool isDescendantDeleting(const PlanItem& item, int first_deleting, int last_deleting) const;

    PlanItem* parent() { return parent_; }
    const PlanItem* parent() const { return parent_; }

    QModelIndex insertPlan(Plan& child, int row, const QModelIndex& index);
    QModelIndex insertFolder(QString folder_name, int row, const QModelIndex& index);

    void appendChild(std::unique_ptr<PlanItem> item);

    bool checkPlanName(const QString& name) const;
    bool checkFolderName(const QString& name) const;

    Plan* plan() { return plan_; }
    const Plan* plan() const { return plan_; }

    QString name() const;
    void setName(QString name);

    bool isFolder() const { return plan_ == nullptr; }

private:
    QUuid id;
    Plan* plan_{};
    QString name_;
    PlanModel* model{};
    PlanItem* parent_{};
    std::vector<std::unique_ptr<PlanItem>> childs;

    void setItemChanged(bool new_item);
    void setPlanChanged();
    void remove(QSqlQuery& delete_query);

    static QString formatCost(double value);

    friend class PlanModel;
    friend class Plan;
};
} // namespace planner

#endif // PLANITEM_H
