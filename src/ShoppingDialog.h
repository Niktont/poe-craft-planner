#ifndef SHOPPINGDIALOG_H
#define SHOPPINGDIALOG_H

#include <keyboard-auto-type.h>
#include <qhotkey.h>
#include <QDialog>

namespace planner {
class ShoppingView;
class ShoppingModel;
class Plan;

class ShoppingDialog : public QDialog
{
    Q_OBJECT
public:
    ShoppingDialog(QWidget* parent = nullptr);

    void openPlan(const Plan& plan);
    void openPlan(const Plan& plan, size_t step_pos, double amount, bool include_dependencies);

private slots:
    void nextItem();
    void pasteWant();
    void pasteWantAmount();
    void pasteHave();
    void pasteHaveAmount();
    void openLink();

private:
    ShoppingView* view;
    ShoppingModel* model;

    std::set<QWindow*> visible_windows;

    keyboard_auto_type::AutoType auto_type;
    QHotkey* next_item_hk;
    QHotkey* want_hk;
    QHotkey* want_amount_hk;
    QHotkey* have_hk;
    QHotkey* have_amount_hk;
    QHotkey* open_link_hk;

    void registerHotkeys();
    void removeHotkeys();

    void pasteText(QString text);
};

} // namespace planner

#endif // SHOPPINGDIALOG_H
