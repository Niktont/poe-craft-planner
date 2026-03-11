#ifndef SHOPPINGDIALOG_H
#define SHOPPINGDIALOG_H

#include <QDialog>

namespace planner {
class ShoppingView;
class ShoppingModel;
class MainWindow;
class Plan;

class ShoppingDialog : public QDialog
{
    Q_OBJECT
public:
    ShoppingDialog(MainWindow& mw);

    void openPlan(Plan* plan);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    ShoppingView* view;
    ShoppingModel* model;
    MainWindow* mw;
    bool web_view_dialog_was_visible{false};
};

} // namespace planner

#endif // SHOPPINGDIALOG_H
