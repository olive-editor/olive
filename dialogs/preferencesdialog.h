#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QKeySequenceEdit>
class QMenuBar;

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    void setup_kbd_shortcuts(QMenuBar* menu);

private:
    Ui::PreferencesDialog *ui;
};

class KeySequenceEditor : public QKeySequenceEdit {
    Q_OBJECT
public:
    KeySequenceEditor(QAction* a);
private slots:
    void set_action_shortcut();
private:
    QAction* action;
};

#endif // PREFERENCESDIALOG_H
