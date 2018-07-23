#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "io/config.h"

#include <QMenuBar>
#include <QAction>
#include <QDebug>

KeySequenceEditor::KeySequenceEditor(QAction* a)
    : QKeySequenceEdit(0), action(a) {
    setKeySequence(action->shortcut());
    connect(this, SIGNAL(editingFinished()), this, SLOT(set_action_shortcut()));
}

void KeySequenceEditor::set_action_shortcut() {
    action->setShortcut(keySequence());
}

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    ui->imgSeqFormatEdit->setText(config.img_seq_formats);
}

PreferencesDialog::~PreferencesDialog() {
    delete ui;
}

void PreferencesDialog::setup_kbd_shortcuts(QMenuBar* menubar) {
    ui->treeWidget->clear();
    QList<QMenu*> menus = menubar->findChildren<QMenu*>();

    // caching for parent-child pairing
    QVector<QTreeWidgetItem*> added_action_items;
    QVector<QAction*> added_actions;

    for (int i=0;i<menus.size();i++) {
        QMenu* menu = menus.at(i);
        QTreeWidgetItem* item = NULL;
        if (menu->parent() != menubar) {
            for (int j=0;j<added_actions.size();j++) {
                if (added_actions.at(j)->menu() == menu) {
                    item = added_action_items.at(j);
                    break;
                }
            }
        }
        if (menu->parent() == menubar || item == NULL) {
            item = new QTreeWidgetItem();
            item->setText(0, menu->title().replace("&", ""));
            ui->treeWidget->addTopLevelItem(item);
        }
        for (int j=0;j<menu->actions().size();j++) {
            QAction* action = menu->actions().at(j);
            if (!action->isSeparator()) {
                QTreeWidgetItem* child = new QTreeWidgetItem();
                child->setText(0, action->text().replace("&", ""));
                item->addChild(child);

                added_actions.append(action);
                added_action_items.append(child);
            }
        }
    }
    for (int i=0;i<added_action_items.size();i++) {
        if (added_actions.at(i)->menu() == NULL) {
            KeySequenceEditor* editor = new KeySequenceEditor(added_actions.at(i));
            ui->treeWidget->setItemWidget(added_action_items.at(i), 1, editor);
        }
    }
}

void PreferencesDialog::on_buttonBox_accepted() {
    config.img_seq_formats = ui->imgSeqFormatEdit->text();
}
