#ifndef WEBVIEWDIALOG_H
#define WEBVIEWDIALOG_H

#include <memory>
#include <QDialog>

#include <QWebEngineProfile>
class QWebEngineView;
class QLineEdit;

namespace planner {

class WebViewDialog : public QDialog
{
    Q_OBJECT
public:
    WebViewDialog(const QString& user_agent, QWidget* parent = nullptr);
    void cleanup();

    QWebEngineView* web_view;
    QLineEdit* url_edit;
    std::unique_ptr<QWebEngineProfile> web_profile;
};

} // namespace planner

#endif // WEBVIEWDIALOG_H
