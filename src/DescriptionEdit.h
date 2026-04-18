#ifndef DESCRIPTIONEDIT_H
#define DESCRIPTIONEDIT_H

#include "Game.h"
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QWidget>

class QAction;

namespace planner {
class DescriptionEdit;

class DescriptionTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit DescriptionTextEdit(QWidget* parent = nullptr);

    QAction* finish_editing_action;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

    bool canInsertFromMimeData(const QMimeData* source) const override;

    void insertFromMimeData(const QMimeData* source) override;

private:
    void insertPlanLink(Game game, const QMimeData& source);
    void insertStepItem(Game game, const QMimeData& source);
};
class DescriptionBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit DescriptionBrowser(QWidget* parent = nullptr);
    QAction* start_editing_action;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

private:
    DescriptionEdit* widget() const;
};

class DescriptionEdit : public QWidget
{
    Q_OBJECT
public:
    DescriptionEdit(QWidget* parent = nullptr);

    DescriptionTextEdit* edit;
    DescriptionBrowser* browser;
    void adjustBrowserSize();

signals:
    void planLinkClicked(const QUuid& id, planner::Game game, bool need_window) const;

private slots:
    void displayTooltip(const QUrl& url);
    void handleAnchorClicked(const QUrl& url);

    friend class DescriptionBrowser;
};

} // namespace planner

#endif // DESCRIPTIONEDIT_H
