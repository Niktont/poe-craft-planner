#include "PlanItem.h"
#include "AppState.h"
#include "Database.h"
#include "ImportException.h"
#include "Plan.h"
#include "PlanModel.h"
#include "PlanSearchModel.h"
#include "StepCopyState.h"
#include <algorithm>
#include <QApplication>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyle>

namespace planner {

enum DatabaseColumn {
    Id,
    Name,
    IsFolder,
    Item,
};

PlanItem::PlanItem(QString name, PlanModel& model, PlanItem* parent)
    : id{QUuid::createUuidV7()}
    , name_{std::move(name)}
    , model{&model}
    , parent_{parent}
{}

PlanItem::PlanItem(QUuid id, QSqlQuery& select, PlanModel& model, PlanItem* parent)
    : id{id}
    , model{&model}
    , parent_{parent}
{
    auto name{select.value(Name).toString()};
    bool is_folder = select.value(IsFolder).toBool();
    auto json{QJsonDocument::fromJson(select.value(Item).toByteArray())};

    if (is_folder) {
        const auto folder_o = json.object();
        const auto childs_a = folder_o["childs"].toArray();
        childs.reserve(childs_a.size());
        for (auto child_v : childs_a) {
            auto child_s = child_v.toString();
            auto child_id = QUuid{child_s};
            if (model.plans.contains(child_id))
                continue;

            select.addBindValue(child_s);
            if (!select.exec() || !select.next())
                continue;

            childs.push_back(std::make_unique<PlanItem>(child_id, select, model, this));
        }
        name_ = std::move(name);
    } else {
        plan_ = &model.plans
                     .insert_or_assign(id,
                                       Plan{id, json.object(), *AppState::exchangeCache(model.game)})
                     .first->second;
        plan_->item_ = this;
        plan_->name = std::move(name);
    }
}

PlanItem::PlanItem(Plan* plan, PlanModel& model, PlanItem* parent)
    : plan_{plan}
    , model{&model}
    , parent_{parent}
{
    if (plan) {
        id = plan->id();
        plan->item_ = this;
    }
}

PlanItem::PlanItem(bool is_folder, const QJsonObject& item_o, PlanModel& model, PlanItem* parent)
    : model{&model}
    , parent_{parent}
{
    auto name = item_o["name"].toString();
    if (is_folder) {
        id = QUuid::createUuidV7();
        name_ = name;
        const auto childs_a = item_o["childs"].toArray();
        childs.reserve(childs_a.size());
        for (auto child_v : childs_a) {
            auto child_o = child_v.toObject();
            childs.push_back(
                std::make_unique<PlanItem>(child_o["is_folder"].toBool(), child_o, model, this));
        }
    } else {
        id = QUuid{item_o["id"].toString()};
        if (id.isNull())
            throw ImportException{ImportError::InvalidPlanId};

        auto res = model.import_plans.try_emplace(id,
                                                  id,
                                                  item_o,
                                                  *AppState::exchangeCache(model.game));
        if (!res.second)
            throw ImportException{ImportError::InvalidPlanId};
        plan_ = &res.first->second;
        plan_->item_ = this;
        plan_->name = name;
    }
}

PlanItem::PlanItem(const PlanItem& item)
    : id{QUuid::createUuidV7()}
    , name_{item.name_}
    , model{item.model}
{
    if (item.isFolder()) {
        for (auto& child : item.childs) {
            auto& copy_child = childs.emplace_back(std::make_unique<PlanItem>(*child));
            copy_child->parent_ = this;
        }
        model->changed_folders.emplace(this);
    } else {
        plan_ = &model->plans.try_emplace(id, id, *item.plan_).first->second;
        plan_->item_ = this;
        model->changed_plans.emplace(this, true);
        model->search_model->insertPlan(*plan_);
    }
}

PlanItem& PlanItem::operator=(PlanItem&& item) noexcept
{
    if (plan_ && item.plan_ != plan_ && model)
        model->plans.erase(plan_->id());

    id = item.id;
    plan_ = item.plan_;
    item.plan_ = nullptr;

    name_ = std::move(item.name_);
    childs = std::move(item.childs);

    if (plan_)
        plan_->item_ = this;

    for (auto& child : childs)
        child->parent_ = this;

    return *this;
}

QJsonObject PlanItem::saveJson() const
{
    if (isFolder()) {
        QJsonObject folder_o;
        QJsonArray childs_a;
        for (auto& child : childs)
            childs_a.push_back(child->id.toString());
        folder_o["childs"] = childs_a;
        return folder_o;
    }

    return plan_->saveJson();
}

QJsonObject PlanItem::exportJson(const ExchangeRequestCache& cache,
                                 TradeRequestCache& trade_cache,
                                 std::vector<QUuid>* plans_to_check) const
{
    if (isFolder()) {
        QJsonObject folder_o;
        QJsonArray childs_a;
        for (auto& child : childs)
            childs_a.push_back(child->exportJson(cache, trade_cache, plans_to_check));
        folder_o["is_folder"] = true;
        folder_o["name"] = name_;
        folder_o["childs"] = childs_a;
        return folder_o;
    }

    if (plans_to_check)
        return plan_->exportJson(cache, trade_cache, *plans_to_check);
    else
        return plan_->exportJson(cache, trade_cache);
}

void PlanItem::replacePlan(int row, Plan&& new_plan)
{
    auto& child = childs[row];
    *child->plan_ = std::move(new_plan);
    child->plan_->item_ = child.get();
    child->plan_->setChanged();
}

PlanItem* PlanItem::restoreChild(int row)
{
    auto& child = childs[row];

    auto select = Database::selectPlan(model->game);

    select.addBindValue(child->id.toString());

    if (!select.exec())
        return nullptr;
    if (!select.next())
        return nullptr;

    *child = {child->id, select, *model, this};

    return child.get();
}

void PlanItem::insertCopy(int row, const PlanItem& copy_item)
{
    auto& child = *childs.emplace(childs.begin() + row, std::make_unique<PlanItem>(copy_item));
    child->parent_ = this;
    child->setName(child->name() + PlanModel::tr(" - Copy"));
}

QModelIndex PlanItem::index(int column) const
{
    if (model->root.get() == this)
        return {};

    return model->createIndex(row(), column, this);
}

QVariant PlanItem::data(int column, int role) const
{
    auto col = static_cast<PlanItemColumn>(column);
    if (isFolder()) {
        switch (col) {
        case PlanItemColumn::Name:
            switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return name_;
            case Qt::DecorationRole:
                return qApp->style()->standardIcon(QStyle::SP_DirIcon);
            }
        default:
            return {};
        }
    } else {
        switch (col) {
        case PlanItemColumn::Name:
            switch (role) {
            case Qt::DisplayRole:
                return !plan_->is_changed ? plan_->name : plan_->name + u"*";
            case Qt::EditRole:
                return plan_->name;
            }
            return {};
        case PlanItemColumn::Cost:
            switch (role) {
            case Qt::DecorationRole: {
                if (auto step = plan_->costStep()) {
                    if (auto cost = step->cost(); cost.isValid()) {
                        auto cache = AppState::exchangeCache(model->game);
                        auto it = cache->currencyData(cost.cost_in_primary.currency);
                        if (it != cache->cache.end())
                            return cache->icon(it);
                    }
                }
                return {};
            }
            case Qt::DisplayRole:
                if (auto step = plan_->costStep()) {
                    auto cost = step->cost();
                    if (cost.isValid() && step->results_cost.isValid())
                        return QString{formatCost(cost.cost_in_primary.value) % "/"
                                       % formatCost(step->results_cost.cost_in_primary.value)};
                    else if (cost.isValid())
                        return formatCost(cost.cost_in_primary.value);
                }
                return {};
            }
            return {};
        }
    }
    return {};
}

bool PlanItem::setData(int column, const QVariant& value, int role)
{
    if (role != Qt::EditRole)
        return false;

    auto col = static_cast<PlanItemColumn>(column);
    if (col == PlanItemColumn::Name) {
        auto new_name = value.toString();
        if (!new_name.isEmpty() && new_name != name()) {
            setName(new_name);
            return true;
        }
    }
    return false;
}

int PlanItem::row() const
{
    if (!parent_)
        return 0;

    auto it = std::ranges::find(parent_->childs, this, &std::unique_ptr<PlanItem>::get);
    if (it != parent_->childs.end())
        return std::distance(parent_->childs.begin(), it);

    return -1;
}

QString PlanItem::path() const
{
    if (!parent_ || parent_->name().isEmpty())
        return name();
    return parent_->path() % u'/' % name();
}

QString PlanItem::shortPath() const
{
    if (!parent_ || parent_->name().isEmpty())
        return name();
    return parent_->name() % u'/' % name();
}

bool PlanItem::isDescendant(const PlanItem& item) const
{
    auto parent = item.parent_;

    while (parent && parent != this)
        parent = parent->parent_;

    return parent != nullptr;
}

int PlanItem::isAncestor(const PlanItem& item) const
{
    auto parent = this->parent_;
    auto child = this;
    while (parent && parent != &item) {
        child = parent;
        parent = parent->parent_;
    }
    return parent != nullptr ? child->row() : -1;
}

QModelIndex PlanItem::insertPlan(Plan& child, int row, const QModelIndex& index)
{
    if (row < 0 || row > std::ssize(childs))
        row = std::ssize(childs);

    auto child_it = childs.insert(childs.begin() + row,
                                  std::make_unique<PlanItem>(&child, *model, this));
    model->changed_folders.insert(this);
    model->changed_plans.insert({child_it->get(), true});

    return model->index(row, 0, index);
}

QModelIndex PlanItem::insertFolder(QString folder_name, int row, const QModelIndex& index)
{
    if (row < 0 || row > std::ssize(childs))
        row = std::ssize(childs);

    auto child_it = childs.insert(childs.begin() + row,
                                  std::make_unique<PlanItem>(std::move(folder_name), *model, this));
    model->changed_folders.insert(this);
    model->changed_folders.insert(child_it->get());

    return model->index(row, 0, index);
}

void PlanItem::appendChild(std::unique_ptr<PlanItem> item)
{
    item->parent_ = this;
    childs.push_back(std::move(item));
}

bool PlanItem::checkPlanName(const QString& name) const
{
    for (auto& child : childs) {
        if (child->plan_ && child->plan_->name == name)
            return false;
    }
    return true;
}

bool PlanItem::checkFolderName(const QString& name) const
{
    for (auto& child : childs) {
        if (!child->plan_ && child->name_ == name)
            return false;
    }
    return true;
}

QString PlanItem::name() const
{
    return plan_ ? plan_->name : name_;
}

void PlanItem::setName(QString name)
{
    if (plan_) {
        plan_->name = std::move(name);
        model->search_model->updatePath(*plan_);
    } else {
        name_ = std::move(name);
        for (auto& child : childs) {
            if (child->plan_)
                model->search_model->updatePath(*child->plan_);
        }
    }
}

void PlanItem::setItemChanged(bool new_item)
{
    if (isFolder()) {
        model->changed_folders.emplace(this);
        for (auto& child : childs)
            child->setItemChanged(new_item);
    } else {
        plan_->is_changed = true;
        model->changed_plans.emplace(this, new_item);
    }
}

void PlanItem::setPlanChanged()
{
    model->setPlanChanged(*this);
}

void PlanItem::remove(QSqlQuery& delete_query)
{
    if (isFolder())
        model->changed_folders.erase(this);
    else {
        model->changed_plans.erase(this);
        if (auto it = model->plans.find(plan_->id()); it != model->plans.end()) {
            if (StepCopyState::state.plan_id == it->first)
                StepCopyState::state.game = Game::Unknown;
            model->search_model->removePlan(it->first);
            model->plans.erase(it);
        }
    }
    if (model->item_copy_state == this)
        model->item_copy_state = nullptr;

    delete_query.addBindValue(id.toString());
    delete_query.exec();

    for (auto& child : childs)
        child->remove(delete_query);
}

QString PlanItem::formatCost(double value)
{
    value = std::ceil(value * 10.0) / 10.0;
    return QString::number(value);
}

} // namespace planner
