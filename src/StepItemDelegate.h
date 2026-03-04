#ifndef STEPITEMDELEGATE_H
#define STEPITEMDELEGATE_H

#include <QStyledItemDelegate>

class QLineEdit;

namespace planner {

class StepItemModel;

class StepItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    StepItemDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

private slots:
    void commitAndCloseEditor();
    void commitAndCloseCompleterEditor(const QModelIndex& index);

private:
    QLineEdit* createCurrencyEdit(QWidget* parent, const StepItemModel* model) const;
    QLineEdit* createTradeEdit(QWidget* parent, const StepItemModel* model) const;
};

} // namespace planner

#endif // STEPITEMDELEGATE_H
