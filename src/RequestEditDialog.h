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
    void openGame(planner::Game game, bool need_clear = false);
    void openRequest(planner::Game game, const planner::TradeRequestKey& request);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void checkName();
    void checkLink();
    void loadQuery();
    void checkQuery();
    void checkChange();

    void selectRequest(const QModelIndex& proxy_idx);
    void saveRequest();
    void cleanup();

private:
    QLineEdit* name_edit;
    QLineEdit* link_edit;
    QLineEdit* regex_edit;
    QLineEdit* description_edit;

    QCheckBox* query_cb;
    QPushButton* paste_button;
    QPushButton* load_button;

    QPushButton* ok_button;
    QPushButton* save_button;
    QPushButton* cancel_button;

    TradeRequestKey edit_request;
    QJsonDocument edit_query;

    TradeRequestCache* cache;
    Game game{Game::Unknown};
    MainWindow* mw() const;

    bool is_name_valid{false};
    bool is_link_valid{false};
    bool is_query_valid{false};

    void setQueryValid(bool valid);
    void enableSave(bool enable);

    void setGame(planner::Game game);
    void findQuery(const QString& html);
    void queryLoadFailed();
    void clear();
};

} // namespace planner

#endif // REQUESTEDITDIALOG_H
