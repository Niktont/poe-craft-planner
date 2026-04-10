#include "DescriptionEdit.h"
#include "PlanModel.h"
#include "StepItemModel.h"
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolTip>
#include <QUuid>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace planner {
static constexpr auto plan_link_scheme_poe1{"plan1"_L1};
static constexpr auto plan_link_scheme_poe2{"plan2"_L1};

static const QString plan_link_poe1{u"[%1](plan1://%2)"_s};
static const QString plan_link_poe2{u"[%1](plan2://%2)"_s};

static QUuid idFromLink(const QUrl& url)
{
    return QUuid::fromString(url.host());
}

DescriptionTextEdit::DescriptionTextEdit(QWidget* parent)
    : QPlainTextEdit{parent}
{
    finish_editing_action = addAction(tr("Finish Editing"));
    finish_editing_action->setShortcuts({Qt::Key_F2, Qt::ControlModifier | Qt::Key_E});
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

bool DescriptionTextEdit::canInsertFromMimeData(const QMimeData* source) const
{
    if (source->hasFormat(PlanModel::move_mime_poe1) || source->hasFormat(PlanModel::move_mime_poe2)
        || source->hasFormat(StepItemModel::move_mime_poe1)
        || source->hasFormat(StepItemModel::move_mime_poe2))
        return true;

    return QPlainTextEdit::canInsertFromMimeData(source);
}

void DescriptionTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (source->hasFormat(PlanModel::move_mime_poe1))
        return insertPlanLink(Game::Poe1, source);
    if (source->hasFormat(PlanModel::move_mime_poe2))
        return insertPlanLink(Game::Poe2, source);

    if (source->hasFormat(StepItemModel::move_mime_poe1))
        return insertStepItem(Game::Poe1, source);
    if (source->hasFormat(StepItemModel::move_mime_poe2))
        return insertStepItem(Game::Poe2, source);

    QPlainTextEdit::insertFromMimeData(source);
}

void DescriptionTextEdit::insertPlanLink(Game game, const QMimeData* source)
{
    auto plans = PlanModel::decodeMimeToPlans(game, source);
    if (plans.empty())
        return;

    auto& link_format = game == Game::Poe1 ? plan_link_poe1 : plan_link_poe2;
    auto cursor = textCursor();
    cursor.beginEditBlock();

    cursor.insertText(
        link_format.arg(plans[0]->name, plans[0]->id().toString(QUuid::WithoutBraces)));
    for (auto plan : std::views::drop(plans, 1))
        cursor.insertText(u", "_s
                          % link_format.arg(plan->name, plan->id().toString(QUuid::WithoutBraces)));

    cursor.endEditBlock();
}

void DescriptionTextEdit::insertStepItem(Game game, const QMimeData* source)
{
    auto [step_model, step_items] = StepItemModel::decodeStepItemsMime(game, source);
    QStringList links;
    auto& plans = step_model->planModel()->plans;
    auto& link_format = game == Game::Poe1 ? plan_link_poe1 : plan_link_poe2;
    for (auto item : step_items) {
        if (auto plan = item->plan()) {
            if (auto it = plans.find(plan->plan_id); it != plans.end()) {
                links.push_back(link_format.arg(!plan->name.isEmpty() ? plan->name : it->second.name,
                                                plan->plan_id.toString(QUuid::WithoutBraces)));
            }
        }
    }
    if (links.empty())
        return;

    auto cursor = textCursor();
    cursor.insertText(links.join(u", "_s));
}

DescriptionBrowser::DescriptionBrowser(QWidget* parent)
    : QTextBrowser{parent}
{
    start_editing_action = addAction(tr("Edit"));
    start_editing_action->setShortcuts({Qt::Key_F2, Qt::ControlModifier | Qt::Key_E});
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

DescriptionEdit::DescriptionEdit(PlanModel& plan_model_poe1,
                                 PlanModel& plan_model_poe2,
                                 QWidget* parent)
    : QWidget{parent}
    , plan_model_poe1{plan_model_poe1}
    , plan_model_poe2{plan_model_poe2}
{
    setLayout(new QVBoxLayout{});
    layout()->setContentsMargins(0, 0, 0, 0);

    edit = new DescriptionTextEdit{};
    edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    edit->setPlaceholderText(tr("Description"));
    edit->setFixedWidth(760);
    edit->setFixedHeight(400);

    browser = new DescriptionBrowser{};
    browser->setOpenLinks(false);
    browser->setOpenExternalLinks(false);
    browser->setPlaceholderText(tr("Description"));
    browser->setMaximumSize(edit->maximumSize());
    browser->setFrameShadow(QFrame::Plain);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setLineWrapMode(QTextEdit::FixedPixelWidth);
    browser->setLineWrapColumnOrWidth(browser->maximumWidth()
                                      - browser->verticalScrollBar()->sizeHint().width()
                                      - 2 * browser->frameWidth());

    connect(browser, &QTextBrowser::highlighted, this, &DescriptionEdit::displayTooltip);
    connect(browser, &QTextBrowser::anchorClicked, this, &DescriptionEdit::handleAnchorClicked);

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
    size.rwidth() += browser->verticalScrollBar()->sizeHint().width() + 2 * browser->frameWidth();

    browser->setFixedWidth(std::min(size.width(), edit->maximumWidth()));
    browser->setFixedHeight(std::min(size.height(), edit->maximumHeight()));
}

void DescriptionEdit::displayTooltip(const QUrl& url)
{
    QString tooltip;
    if (url.scheme() == plan_link_scheme_poe1) {
        if (auto it = plan_model_poe1.plans.find(idFromLink(url)); it != plan_model_poe1.plans.end())
            tooltip = it->second.name;
    } else if (url.scheme() == plan_link_scheme_poe2) {
        if (auto it = plan_model_poe2.plans.find(idFromLink(url)); it != plan_model_poe2.plans.end())
            tooltip = it->second.name;
    } else
        tooltip = url.toDisplayString();

    browser->setToolTip(tooltip);
    if (tooltip.isEmpty())
        QToolTip::hideText();
}

void DescriptionEdit::handleAnchorClicked(const QUrl& url)
{
    if (url.scheme() == plan_link_scheme_poe1) {
        emit planLinkClicked(idFromLink(url), Game::Poe1);
        return;
    }
    if (url.scheme() == plan_link_scheme_poe2) {
        emit planLinkClicked(idFromLink(url), Game::Poe2);
        return;
    }
    if (url.isRelative()) {
        if (url.hasFragment())
            browser->scrollToAnchor(url.fragment());
        return;
    }

    bool is_file_scheme = url.scheme() == "file"_L1 || url.scheme() == "qrc"_L1;
    if (!is_file_scheme) {
        QDesktopServices::openUrl(url);
        return;
    }
    // browser->setSource(url);
}

} // namespace planner
