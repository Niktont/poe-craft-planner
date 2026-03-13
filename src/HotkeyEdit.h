#ifndef HOTKEYEDIT_H
#define HOTKEYEDIT_H

#include <QKeySequenceEdit>

namespace planner {

class HotkeyEdit : public QKeySequenceEdit
{
    Q_OBJECT
public:
    HotkeyEdit(bool is_paste_edit, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool is_paste_edit;
};

} // namespace planner

#endif // HOTKEYEDIT_H
