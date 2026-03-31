#include "ShoppingView.h"
#include "ShoppingModel.h"
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHeaderView>
#include <QScrollBar>

using namespace Qt::StringLiterals;

namespace planner {

ShoppingView::ShoppingView(ShoppingModel& model, QWidget* parent)
    : QTableView{parent}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setWordWrap(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setSelectionMode(SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    setModel(&model);
    connect(this, &QTableView::clicked, this, &ShoppingView::indexClicked);

    QPalette p = palette();
    p.setColor(QPalette::Inactive,
               QPalette::Highlight,
               p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive,
               QPalette::HighlightedText,
               p.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(p);

    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.features = QStyleOptionViewItem::HasDisplay;

    auto header = horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    header->setMinimumSectionSize(20);

    auto amount_width = widthForItemText(option, u"100000"_s);
    header->resizeSection(static_cast<int>(ShoppingColumn::Amount), amount_width);

    header->resizeSection(static_cast<int>(ShoppingColumn::Name), 120);

    auto link_width = header->sectionSizeHint(static_cast<int>(ShoppingColumn::Link));
    header->resizeSection(static_cast<int>(ShoppingColumn::Link), link_width);

    verticalHeader()->setDefaultSectionSize(fontMetrics().height());

    header->hide();
    verticalHeader()->hide();
}

QSize ShoppingView::sizeHint() const
{
    auto size = QTableView::sizeHint();
    size.rwidth() = horizontalHeader()->length() + verticalScrollBar()->sizeHint().width()
                    + lineWidth() * 2;
    return size;
}

void ShoppingView::indexClicked(const QModelIndex& idx)
{
    if (idx.column() != static_cast<int>(ShoppingColumn::Link))
        return;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
        auto url = QUrl::fromUserInput(model()->data(idx, Qt::ToolTipRole).toString());
        if (url.isValid())
            QDesktopServices::openUrl(url);
    }
}

int ShoppingView::widthForItemText(QStyleOptionViewItem& option, const QString& text) const
{
    option.text = text;
    return style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, {}).width() + showGrid();
}

void ShoppingView::adjustNameWidth()
{
    auto col = static_cast<int>(ShoppingColumn::Name);
    setColumnWidth(col, std::max(sizeHintForColumn(col), 120));
}

} // namespace planner
