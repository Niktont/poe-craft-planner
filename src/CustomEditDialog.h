#ifndef CUSTOMEDITDIALOG_H
#define CUSTOMEDITDIALOG_H

#include "CustomCalculation.h"
#include <QDialog>

class QLineEdit;
class QPushButton;

namespace planner {
class Plan;
class StepItemsWidget;

class CustomEditDialog : public QDialog
{
public:
    CustomEditDialog(QWidget* parent);

    void openCustomEdit(Plan* plan, size_t step_pos, StepItemsWidget* items_widget);

    CustomCalculation calc;

private slots:
    void saveCustomString();

private:
    Plan* plan{};
    size_t step_pos{};
    bool is_resource_calc{};
    StepItemsWidget* items_widget{};

    QLineEdit* custom_edit;
    QPushButton* ok_button;
};

} // namespace planner

#endif // CUSTOMEDITDIALOG_H
