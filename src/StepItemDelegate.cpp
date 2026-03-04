#include "StepItemDelegate.h"
#include "ExchangeRequestCache.h"
#include "Plan.h"
#include "StepItemModel.h"
#include "TradeRequestCache.h"
#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QListView>

namespace planner {

StepItemDelegate::StepItemDelegate(QObject* parent)
    : QStyledItemDelegate{parent}
{}

QWidget* StepItemDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    auto col = static_cast<StepItemColumn>(index.column());
    auto step_model = static_cast<const StepItemModel*>(index.model());
    auto& items = step_model->stepItems();

    switch (col) {
    case StepItemColumn::Amount:
    case StepItemColumn::Cost:
    case StepItemColumn::Gold:
    case StepItemColumn::Time:
    case StepItemColumn::Success:
        return QStyledItemDelegate::createEditor(parent, option, index);
    case StepItemColumn::Type: {
        auto combo = new QComboBox{parent};
        combo->addItems(StepItem::typeList());
        if (step_model->stepPos() == 0)
            qobject_cast<QListView*>(combo->view())
                ->setRowHidden(static_cast<int>(StepItemType::Step), true);
        connect(combo, &QComboBox::activated, this, &StepItemDelegate::commitAndCloseEditor);
        return combo;
    }
    case StepItemColumn::Name:
        if (items[index.row()].type() == StepItemType::Exchange)
            return createCurrencyEdit(parent, step_model);
        if (items[index.row()].type() == StepItemType::Step) {
            auto steps = step_model->plan->stepsName(step_model->stepPos());
            auto combo = new QComboBox{parent};
            combo->addItems(steps);
            connect(combo, &QComboBox::activated, this, &StepItemDelegate::commitAndCloseEditor);
            return combo;
        }
        break;
    case StepItemColumn::Link:
        if (items[index.row()].type() == StepItemType::Trade)
            return createTradeEdit(parent, step_model);
        break;
    case StepItemColumn::CostCurrency: {
        return createCurrencyEdit(parent, step_model);
    }
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void StepItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto col = static_cast<StepItemColumn>(index.column());
    auto step_model = static_cast<const StepItemModel*>(index.model());
    auto& item = step_model->stepItems()[index.row()];

    switch (col) {
    case StepItemColumn::Amount:
    case StepItemColumn::Link:
    case StepItemColumn::Cost:
    case StepItemColumn::CostCurrency:
    case StepItemColumn::Gold:
    case StepItemColumn::Time:
    case StepItemColumn::Success:
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    case StepItemColumn::Type: {
        auto combo = static_cast<QComboBox*>(editor);
        combo->setCurrentIndex(item.data.index());
        return;
    }
    case StepItemColumn::Name:
        if (item.type() == StepItemType::Step) {
            auto combo = static_cast<QComboBox*>(editor);
            combo->setCurrentIndex(index.data(Qt::EditRole).toInt());
            return;
        }
        break;
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void StepItemDelegate::setModelData(QWidget* editor,
                                    QAbstractItemModel* model,
                                    const QModelIndex& index) const
{
    auto col = static_cast<StepItemColumn>(index.column());
    auto step_model = static_cast<StepItemModel*>(model);
    auto& item = step_model->stepItems()[index.row()];

    switch (col) {
    case StepItemColumn::Amount:
    case StepItemColumn::Cost:
    case StepItemColumn::Gold:
    case StepItemColumn::Time:
    case StepItemColumn::Success:
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    case StepItemColumn::Type: {
        auto combo = static_cast<QComboBox*>(editor);
        step_model->setItemType(index, static_cast<StepItemType>(combo->currentIndex()));
        return;
    }
    case StepItemColumn::Name:
        if (item.type() == StepItemType::Exchange) {
            auto edit = static_cast<QLineEdit*>(editor);
            disconnect(edit->completer(),
                       qOverload<const QModelIndex&>(&QCompleter::activated),
                       this,
                       &StepItemDelegate::commitAndCloseCompleterEditor);
            auto selected_index = edit->property("index").toModelIndex();
            model->setData(index, selected_index);
            return;
        }
        if (item.type() == StepItemType::Step) {
            auto combo = static_cast<QComboBox*>(editor);
            model->setData(index, combo->currentIndex());
            return;
        }
        break;
    case StepItemColumn::Link:
        if (item.type() == StepItemType::Trade) {
            auto edit = static_cast<QLineEdit*>(editor);
            disconnect(edit->completer(),
                       qOverload<const QModelIndex&>(&QCompleter::activated),
                       this,
                       &StepItemDelegate::commitAndCloseCompleterEditor);
            auto selected_index = edit->property("index").toModelIndex();
            model->setData(index, selected_index);
            return;
        }
        break;
    case StepItemColumn::CostCurrency: {
        auto edit = static_cast<QLineEdit*>(editor);
        disconnect(edit->completer(),
                   qOverload<const QModelIndex&>(&QCompleter::activated),
                   this,
                   &StepItemDelegate::commitAndCloseCompleterEditor);
        auto selected_index = edit->property("index").toModelIndex();
        model->setData(index, selected_index);
        return;
    }
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}

void StepItemDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& /*index*/) const
{
    auto rect = option.rect;
    auto editor_width = editor->sizeHint().width();
    if (auto edit = qobject_cast<QLineEdit*>(editor); edit && edit->completer())
        editor_width = edit->completer()->popup()->sizeHint().width();

    rect.setWidth(std::max(rect.width(), editor_width));

    editor->setGeometry(rect);
}

void StepItemDelegate::commitAndCloseEditor()
{
    auto editor = qobject_cast<QWidget*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

void StepItemDelegate::commitAndCloseCompleterEditor(const QModelIndex& index)
{
    auto completer = static_cast<QCompleter*>(sender());
    auto edit = completer->widget();
    auto proxy = static_cast<QAbstractProxyModel*>(completer->completionModel());
    edit->setProperty("index", proxy->mapToSource(index));

    emit commitData(edit);
    emit closeEditor(edit);
}

QLineEdit* StepItemDelegate::createCurrencyEdit(QWidget* parent, const StepItemModel* model) const
{
    auto edit = new QLineEdit{parent};

    edit->setCompleter(model->exchange_cache->completer);
    connect(edit->completer(),
            qOverload<const QModelIndex&>(&QCompleter::activated),
            this,
            &StepItemDelegate::commitAndCloseCompleterEditor);

    return edit;
}

QLineEdit* StepItemDelegate::createTradeEdit(QWidget* parent, const StepItemModel* model) const
{
    auto edit = new QLineEdit{parent};

    edit->setCompleter(model->trade_cache->completer);
    connect(edit->completer(),
            qOverload<const QModelIndex&>(&QCompleter::activated),
            this,
            &StepItemDelegate::commitAndCloseCompleterEditor);

    return edit;
}

} // namespace planner
