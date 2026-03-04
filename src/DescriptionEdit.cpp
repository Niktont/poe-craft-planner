#include "DescriptionEdit.h"
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolTip>
#include <QVBoxLayout>

namespace planner {
DescriptionTextEdit::DescriptionTextEdit(QWidget* parent)
    : QPlainTextEdit{parent}
{
    finish_editing_action = addAction(tr("Finish Editing"));
    finish_editing_action->setShortcuts({Qt::Key_F2, Qt::CTRL | Qt::Key_E});
    finish_editing_action->setShortcutContext(Qt::WidgetShortcut);
}

void DescriptionTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = createStandardContextMenu(event->pos());
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addSeparator();
    menu->addAction(finish_editing_action);
    menu->popup(event->globalPos());
}
DescriptionBrowser::DescriptionBrowser(QWidget* parent)
    : QTextBrowser{parent}
{
    start_editing_action = addAction(tr("Edit"));
    start_editing_action->setShortcuts({Qt::Key_F2, Qt::CTRL | Qt::Key_E});
    start_editing_action->setShortcutContext(Qt::WidgetShortcut);
}

void DescriptionBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = createStandardContextMenu(event->pos());
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addSeparator();
    menu->addAction(start_editing_action);
    menu->popup(event->globalPos());
}

DescriptionEdit::DescriptionEdit(QWidget* parent)
    : QWidget{parent}
{
    setLayout(new QVBoxLayout{});
    layout()->setContentsMargins(0, 0, 0, 0);

    edit = new DescriptionTextEdit{};
    edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    edit->setPlaceholderText(tr("Description"));
    edit->setFixedWidth(760);
    edit->setFixedHeight(400);

    browser = new DescriptionBrowser{};
    browser->setPlaceholderText(tr("Description"));
    browser->setMaximumSize(edit->maximumSize());
    browser->setFrameShadow(QFrame::Plain);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setLineWrapMode(QTextEdit::FixedPixelWidth);
    browser->setLineWrapColumnOrWidth(browser->maximumWidth()
                                      - browser->verticalScrollBar()->sizeHint().width()
                                      - 2 * browser->frameWidth());
    browser->setOpenExternalLinks(true);

    connect(browser, &QTextBrowser::highlighted, this, [this](const QUrl& url) {
        auto str = url.toDisplayString();
        browser->setToolTip(str);
        if (str.isEmpty())
            QToolTip::hideText();
    });
    connect(browser->start_editing_action, &QAction::triggered, this, [this] {
        browser->hide();
        edit->show();
        edit->setFocus();
    });
    connect(edit->finish_editing_action, &QAction::triggered, this, [this] {
        edit->hide();
        browser->show();
        browser->setMarkdown(edit->toPlainText());
        browser->setFocus();
        adjustBrowserSize();
    });

    auto p = browser->viewport()->palette();
    p.setColor(browser->viewport()->backgroundRole(), {0, 0, 0, 0});
    browser->viewport()->setPalette(p);

    layout()->addWidget(browser);
    layout()->addWidget(edit);
    edit->hide();
}

void DescriptionEdit::adjustBrowserSize()
{
    auto size = browser->document()->size().toSize();
    size.rheight() += 2 * browser->frameWidth();
    size.rwidth() += 2 * browser->frameWidth();
    browser->setMinimumWidth(std::min(size.width(), browser->maximumWidth()));
    browser->setMinimumHeight(std::min(size.height(), browser->maximumHeight()));
}

} // namespace planner
