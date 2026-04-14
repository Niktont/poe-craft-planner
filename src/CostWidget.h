#ifndef COSTWIDGET_H
#define COSTWIDGET_H

#include "Game.h"
#include <QWidget>

class QLabel;

namespace planner {
class Step;
class CurrencyCost;

class CostWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CostWidget(QWidget* parent = nullptr);

    void hideCurrencyName();

public slots:
    void setCost(planner::Game game, const planner::Step* step);

private:
    QLabel* cost_label;
    QLabel* profit_label;
    QLabel* currency_icon_label;
    QLabel* currency_label;
    QLabel* gold_label;
    QLabel* time_label;

    static QString formatCost(const CurrencyCost& cost);
    static QString formatCeil1(double value);
};

} // namespace planner

#endif // COSTWIDGET_H
