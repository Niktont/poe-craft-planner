#ifndef SHOPPINGSETUPDIALOG_H
#define SHOPPINGSETUPDIALOG_H

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QComboBox;

namespace planner {
class MainWindow;
class Plan;

class ShoppingSetupDialog : public QDialog
{
    Q_OBJECT
public:
    ShoppingSetupDialog(MainWindow& mw);

    QComboBox* step_combo;
    QDoubleSpinBox* amount_edit;
    QCheckBox* dependencies_cb;

    void openPlan(Plan& plan);

private:
    Plan* plan{};
    MainWindow* mw() const;
};

} // namespace planner

#endif // SHOPPINGSETUPDIALOG_H
