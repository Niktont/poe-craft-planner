#include "CustomEditDialog.h"
#include "AppState.h"
#include "CustomCalculation.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

CustomEditDialog::CustomEditDialog(QWidget* parent)
    : QDialog{parent}
{
    setWindowTitle(tr("Custom Expression"));

    auto layout = new QHBoxLayout{};
    layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    custom_edit = new QLineEdit{this};
    custom_edit->setPlaceholderText("(1 + 2) | (3 + 4)");
    custom_edit->setMinimumWidth(200);
    connect(custom_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto width = custom_edit->fontMetrics().horizontalAdvance(text) + 15;
        custom_edit->setMinimumWidth(std::max(200, width));
    });

    layout->addWidget(custom_edit, 1);

    ok_button = new QPushButton{tr("Ok"), this};
    connect(ok_button, &QPushButton::clicked, this, &CustomEditDialog::saveCustomString);
    layout->addWidget(ok_button);
}

void CustomEditDialog::openCustomEdit(QString custom_text_)
{
    custom_text = std::move(custom_text_);
    custom_edit->setText(custom_text);

    open();

    auto title_pos = geometry().topLeft() - pos();
    auto new_pos = QCursor::pos() - title_pos;
    move(new_pos);
}

void CustomEditDialog::saveCustomString()
{
    auto text = custom_edit->text();
    auto trimmed_text = text.trimmed();
    if (trimmed_text == custom_text) {
        accept();
        return;
    }

    if (trimmed_text.isEmpty()) {
        custom_text = trimmed_text;
        accept();
        return;
    }

    auto std_text = trimmed_text.toStdString();
    custom_tree = {};
    auto [success, consumed] = AppState::state.custom_calc->parseString(std_text, custom_tree);
    if (!success) {
        if (text != trimmed_text)
            custom_edit->setText(trimmed_text);
        custom_edit->setFocus();
        custom_edit->setCursorPosition(std::distance(std_text.cbegin(), consumed));

        auto msg = new QMessageBox{this};
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->setWindowTitle(tr("Parse Failed"));
        msg->setText(tr("Failed to parse expression."));
        msg->open();
        return;
    }

    custom_text = trimmed_text;

    accept();
}

} // namespace planner
