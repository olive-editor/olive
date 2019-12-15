#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "widget/viewer/viewer.h"

class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  ExportDialog(QWidget* parent = nullptr);

private:
  ViewerWidget* preview_viewer_;
  QLineEdit* filename_edit_;

private slots:
  void BrowseFilename();
};

#endif // EXPORTDIALOG_H
