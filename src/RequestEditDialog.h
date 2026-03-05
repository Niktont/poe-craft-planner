#ifndef REQUESTEDITDIALOG_H
#define REQUESTEDITDIALOG_H

#include "TradeRequestCache.h"
#include <QDialog>
#include <QJsonDocument>

class QLineEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QCheckBox;

namespace planner {

class MainWindow;

class RequestEditDialog : public QDialog
{
    Q_OBJECT
public:
    RequestEditDialog(MainWindow& mw);

public slots:
    void openGame(planner::Game game);
    void openRequest(const planner::TradeRequestKey& request, planner::Game game);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void checkName();
    void checkLink();
    void loadQuery();
    void checkQuery();

    void selectRequest(const QModelIndex& proxy_i);
    void saveRequest();
    void deleteRequest();
    void cleanup();

private:
    QLabel* load_label;
    QLineEdit* name_edit;
    QLineEdit* link_edit;
    QLineEdit* query_edit;

    QPushButton* load_button;
    QPushButton* save_button;
    QPushButton* delete_button;

    TradeRequestKey edit_request;
    QJsonDocument edit_query;

    TradeRequestCache* cache;
    Game game{Game::Unknown};
    MainWindow* mw() const;

    bool is_name_valid{false};
    bool is_link_valid{false};
    bool is_query_valid{false};

    void setGame(planner::Game game);
    void findQuery(const QString& html);
    void queryLoadFailed();
    void clear();
};

} // namespace planner

#endif // REQUESTEDITDIALOG_H
