#ifndef STEPITEMVIEW_H
#define STEPITEMVIEW_H

#include <QTableView>

namespace planner {

class StepItemModel;

class StepItemView : public QTableView
{
    Q_OBJECT
public:
    StepItemView(StepItemModel& model, QWidget* parent = nullptr);

    void setOtherView(StepItemView* other_view) { this->other_view = other_view; }

public slots:
    void syncColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    QSize sizeHint() const override;

private slots:
    void resizeColumns(const QModelIndex& top_left,
                       const QModelIndex& bottom_right,
                       const QList<int>& roles);
    void indexClicked(const QModelIndex& idx);

private:
    StepItemModel* stepModel();
    StepItemView* other_view{};

    QAction* add_trade_action;
    QAction* add_exchange_action;
    QAction* add_custom_action;
    QAction* add_step_action;
    QAction* duplicate_action;
    QAction* manage_searches_action;
    QAction* default_time_action;
    QAction* delete_action;

    std::optional<QModelIndex> contextIndex;
    void syncColumn(int col);
    int widthForItemText(QStyleOptionViewItem& option, const QString& text) const;

    void setupColumns();

    QSize totalSize() const;
};

} // namespace planner

#endif // STEPITEMVIEW_H
