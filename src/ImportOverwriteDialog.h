#ifndef IMPORTOVERWRITEDIALOG_H
#define IMPORTOVERWRITEDIALOG_H

#include "ImportOverwriteModel.h"
#include <QDialog>

namespace planner {

class ImportOverwriteDialog : public QDialog
{
    Q_OBJECT
public:
    ImportOverwriteDialog(ImportOverwriteModel& model, QWidget* parent = nullptr);
};

} // namespace planner

#endif // IMPORTOVERWRITEDIALOG_H
