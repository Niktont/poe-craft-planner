#include "HotkeyEdit.h"
#include <QKeyEvent>

namespace planner {

HotkeyEdit::HotkeyEdit(bool is_paste_edit, QWidget* parent)
    : QKeySequenceEdit{parent}
    , is_paste_edit{is_paste_edit}
{
    setMaximumSequenceLength(1);
    setClearButtonEnabled(true);
}

void HotkeyEdit::keyPressEvent(QKeyEvent* event)
{
    auto count = std::popcount(static_cast<size_t>(event->modifiers()));
    if (count > 1) {
        event->accept();
        return;
    }
    if (is_paste_edit && event->modifiers().testFlag(Qt::AltModifier)) {
        event->accept();
        clear();
        return;
    }

    QKeySequenceEdit::keyPressEvent(event);
    // if (event->modifiers().testFlag(Qt::KeypadModifier)) {
    //     auto ks = keySequence();
    //     ks = ks[0].key() | Qt::KeypadModifier;
    //     setKeySequence(ks);
    // }
}

} // namespace planner
