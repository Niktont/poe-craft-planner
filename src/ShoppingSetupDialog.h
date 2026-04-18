#ifndef SHOPPINGSETUPDIALOG_H
#define SHOPPINGSETUPDIALOG_H

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QComboBox;

namespace planner {
class Plan;

class ShoppingSetupDialog : public QDialog
{
    Q_OBJECT
public:
    ShoppingSetupDialog(QWidget* parent = nullptr);

    void openPlan(Plan& plan);

private slots:
    void openShoppingList();

private:
    QComboBox* step_combo;
    QDoubleSpinBox* amount_edit;
    QCheckBox* dependencies_cb;

    Plan* plan{};
};

} // namespace planner

#endif // SHOPPINGSETUPDIALOG_H
