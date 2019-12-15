#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

#include "widget/viewer/viewer.h"

class ExportDialog : public QDialog
{
public:
  ExportDialog(QWidget* parent = nullptr);

private:
  ViewerWidget* preview_viewer_;
};

#endif // EXPORTDIALOG_H
