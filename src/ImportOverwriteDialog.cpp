#include "ImportOverwriteDialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

ImportOverwriteDialog::ImportOverwriteDialog(ImportOverwriteModel& model, QWidget* parent)
    : QDialog{parent}
{
    setWindowTitle(tr("Overwrite plans"));

    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);

    auto button_box = new QDialogButtonBox{};
    auto button = button_box->addButton(QDialogButtonBox::Yes);
    connect(button, &QPushButton::clicked, this, [this] { done(QDialogButtonBox::YesRole); });
    button = button_box->addButton(QDialogButtonBox::No);
    connect(button, &QPushButton::clicked, this, [this] { done(QDialogButtonBox::NoRole); });
    button = button_box->addButton(QDialogButtonBox::Cancel);
    connect(button, &QPushButton::clicked, this, [this] { done(QDialog::Rejected); });
    main_layout->addWidget(button_box);

    main_layout->addWidget(new QLabel{tr("Overwrite existing plans?")});
    auto view = new QListView{};
    view->setModel(&model);
    main_layout->addWidget(view);
}

} // namespace planner
