#ifndef SHOPPINGVIEW_H
#define SHOPPINGVIEW_H

#include <QTableView>

namespace planner {
class ShoppingModel;

class ShoppingView : public QTableView
{
    Q_OBJECT
public:
    ShoppingView(ShoppingModel& model, QWidget* parent = nullptr);

    QSize sizeHint() const override;

    void adjustNameWidth();

private slots:
    void indexClicked(const QModelIndex& idx);

private:
    int widthForItemText(QStyleOptionViewItem& option, const QString& text) const;
};

} // namespace planner

#endif // SHOPPINGVIEW_H
