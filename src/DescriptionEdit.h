#ifndef DESCRIPTIONEDIT_H
#define DESCRIPTIONEDIT_H

#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QWidget>
class QAction;

namespace planner {
class DescriptionTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit DescriptionTextEdit(QWidget* parent = nullptr);

    void contextMenuEvent(QContextMenuEvent* event) override;
    QAction* finish_editing_action;
};
class DescriptionBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit DescriptionBrowser(QWidget* parent = nullptr);

    void contextMenuEvent(QContextMenuEvent* event) override;
    QAction* start_editing_action;
};

class DescriptionEdit : public QWidget
{
    Q_OBJECT
public:
    DescriptionEdit(QWidget* parent = nullptr);

    DescriptionTextEdit* edit;
    DescriptionBrowser* browser;
    void adjustBrowserSize();
};

} // namespace planner

#endif // DESCRIPTIONEDIT_H
