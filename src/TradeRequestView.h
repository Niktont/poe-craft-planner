#ifndef TRADEREQUESTVIEW_H
#define TRADEREQUESTVIEW_H

#include <QTableView>

namespace planner {
class TradeRequestCache;
class MainWindow;

class TradeRequestView : public QTableView
{
    Q_OBJECT
public:
    TradeRequestView(MainWindow& mw, QWidget* parent = nullptr);

public:
    void setCache(TradeRequestCache& cache);

public slots:
    void filterName(const QString& filter_str);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void indexClicked(const QModelIndex& idx);
    void deleteSearch();

private:
    TradeRequestCache* cache{};
    MainWindow* mw;

    QAction* edit_action;
    QAction* add_action;
    QAction* delete_action;
};

} // namespace planner

#endif // TRADEREQUESTVIEW_H
