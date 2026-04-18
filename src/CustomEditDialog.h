#ifndef CUSTOMEDITDIALOG_H
#define CUSTOMEDITDIALOG_H

#include "CustomExpression.h"
#include <QDialog>

class QLineEdit;
class QPushButton;

namespace planner {

class CustomEditDialog : public QDialog
{
public:
    CustomEditDialog(QWidget* parent = nullptr);

    void openCustomEdit(QString custom_text);

    QString custom_text;
    custom_tree::Expression custom_tree;

private slots:
    void saveCustomString();

private:
    QLineEdit* custom_edit;
    QPushButton* ok_button;
};

} // namespace planner

#endif // CUSTOMEDITDIALOG_H
