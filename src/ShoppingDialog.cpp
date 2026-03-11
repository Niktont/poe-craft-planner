#include "ShoppingDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include "ShoppingModel.h"
#include "ShoppingView.h"
#include <QCloseEvent>
#include <QVBoxLayout>

namespace planner {

ShoppingDialog::ShoppingDialog(MainWindow& mw)
    : QDialog{}
    , mw{&mw}
{
    model = new ShoppingModel{mw, this};
    view = new ShoppingView{*model};
    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);

    main_layout->addWidget(view, 1);

    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_shopping_dialog_geometry);
    if (!geometry.isValid())
        setGeometry(200, 200, 280, 400);
    else
        restoreGeometry(geometry.toByteArray());

    connect(this, &QDialog::finished, this, [this] {
        this->mw->show();
        if (web_view_dialog_was_visible)
            this->mw->web_view_dialog->show();
        web_view_dialog_was_visible = false;
    });
}

void ShoppingDialog::openPlan(Plan* plan)
{
    auto res = model->setPlan(plan);
    if (res) {
        web_view_dialog_was_visible = mw->web_view_dialog->isVisible();
        if (web_view_dialog_was_visible)
            mw->web_view_dialog->hide();

        mw->hide();

        view->adjustNameWidth();
        view->updateGeometry();

        auto width = layout()->contentsRect().width();
        resize(width, height());
        setWindowTitle(tr("%1 - Shopping list").arg(plan->name));

        open();
    }
}

void ShoppingDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    accept();
}

} // namespace planner
