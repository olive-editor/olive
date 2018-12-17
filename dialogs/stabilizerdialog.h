#ifndef STABILIZERDIALOG_H
#define STABILIZERDIALOG_H

class QVBoxLayout;
class QCheckBox;
class QDialogButtonBox;

#include <QDialog>

class StabilizerDialog : public QDialog
{
public:
    StabilizerDialog(QWidget* parent = 0);
private:
    QVBoxLayout* layout;
    QCheckBox* enable_stab;
    QDialogButtonBox* buttons;
};

#endif // STABILIZERDIALOG_H
