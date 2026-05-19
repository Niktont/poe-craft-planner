#ifndef STEPITEMVIEW_H
#define STEPITEMVIEW_H

#include <QTableView>

namespace planner {
class StepItemModel;
class PlanWidget;

class StepItemView : public QTableView
{
    Q_OBJECT
public:
    StepItemView(StepItemModel& model, PlanWidget& plan_widget, QWidget* parent = nullptr);

    void setOtherView(StepItemView& other_view_) { this->other_view = &other_view_; }

    QSize sizeHint() const override;

    bool hideNotUsedItems();

    void syncSize();
    bool syncColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void resizeColumns(const QModelIndex& top_left,
                       const QModelIndex& bottom_right,
                       const QList<int>& roles);
    void indexClicked(const QModelIndex& idx);
    void openSearch();
    void deleteSearch();

    void syncOnRowCountChange();

private:
    StepItemModel* stepModel() const;
    StepItemView* other_view{};

    QAction* add_trade_action;
    QAction* add_exchange_action;
    QAction* add_custom_action;
    QAction* add_step_action;
    QAction* add_plan_action;
    QAction* duplicate_action;
    QAction* copy_action;
    QAction* paste_action;

    QAction* copy_link_action;
    QAction* copy_regex_action;
    QAction* edit_search_action;
    QAction* delete_search_action;
    QAction* default_time_action;

    QAction* delete_action;

    bool context_menu_shown{};
    bool is_context_idx_valid{};

    PlanWidget& plan_widget;

    bool syncColumn(int col, int min_width);

    int widthForItemText(QStyleOptionViewItem& option, const QString& text) const;
    void setupColumns();
};

} // namespace planner

#endif // STEPITEMVIEW_H
