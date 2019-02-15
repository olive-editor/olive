#include "menuhelper.h"

#include "oliveglobal.h"

#include "ui/focusfilter.h"

#include "io/config.h"

#include "panels/panels.h"

#include "mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>

MenuHelper Olive::MenuHelper;

void MenuHelper::make_new_menu(QMenu *parent) {
    parent->addAction(tr("&Project"), Olive::Global.data(), SLOT(new_project()), QKeySequence("Ctrl+N"))->setProperty("id", "newproj");
    parent->addSeparator();
    parent->addAction(tr("&Sequence"), panel_project, SLOT(new_sequence()), QKeySequence("Ctrl+Shift+N"))->setProperty("id", "newseq");
    parent->addAction(tr("&Folder"), panel_project, SLOT(new_folder()))->setProperty("id", "newfolder");
}

void MenuHelper::make_inout_menu(QMenu *parent) {
    parent->addAction(tr("Set In Point"), &Olive::FocusFilter, SLOT(set_in_point()), QKeySequence("I"))->setProperty("id", "setinpoint");
    parent->addAction(tr("Set Out Point"), &Olive::FocusFilter, SLOT(set_out_point()), QKeySequence("O"))->setProperty("id", "setoutpoint");
    parent->addSeparator();
    parent->addAction(tr("Reset In Point"), &Olive::FocusFilter, SLOT(clear_in()))->setProperty("id", "resetin");
    parent->addAction(tr("Reset Out Point"), &Olive::FocusFilter, SLOT(clear_out()))->setProperty("id", "resetout");
    parent->addAction(tr("Clear In/Out Point"), &Olive::FocusFilter, SLOT(clear_inout()), QKeySequence("G"))->setProperty("id", "clearinout");
}

void MenuHelper::make_clip_functions_menu(QMenu *parent) {
    parent->addAction(tr("Add Default Transition"), panel_timeline, SLOT(add_transition()), QKeySequence("Ctrl+Shift+D"))->setProperty("id", "deftransition");
    parent->addAction(tr("Link/Unlink"), panel_timeline, SLOT(toggle_links()), QKeySequence("Ctrl+L"))->setProperty("id", "linkunlink");
    parent->addAction(tr("Enable/Disable"), panel_timeline, SLOT(toggle_enable_on_selected_clips()), QKeySequence("Shift+E"))->setProperty("id", "enabledisable");
    parent->addAction(tr("Nest"), panel_timeline, SLOT(nest()))->setProperty("id", "nest");
}

void MenuHelper::make_edit_functions_menu(QMenu *parent) {
    parent->addAction(tr("Cu&t"), &Olive::FocusFilter, SLOT(cut()), QKeySequence("Ctrl+X"))->setProperty("id", "cut");
    parent->addAction(tr("Cop&y"), &Olive::FocusFilter, SLOT(copy()), QKeySequence("Ctrl+C"))->setProperty("id", "copy");
    parent->addAction(tr("&Paste"), Olive::Global.data(), SLOT(paste()), QKeySequence("Ctrl+V"))->setProperty("id", "paste");
    parent->addAction(tr("Paste Insert"), Olive::Global.data(), SLOT(paste_insert()), QKeySequence("Ctrl+Shift+V"))->setProperty("id", "pasteinsert");
    parent->addAction(tr("Duplicate"), &Olive::FocusFilter, SLOT(duplicate()), QKeySequence("Ctrl+D"))->setProperty("id", "duplicate");
    parent->addAction(tr("Delete"), &Olive::FocusFilter, SLOT(delete_function()), QKeySequence("Del"))->setProperty("id", "delete");
    parent->addAction(tr("Ripple Delete"), panel_timeline, SLOT(ripple_delete()), QKeySequence("Shift+Del"))->setProperty("id", "rippledelete");
    parent->addAction(tr("Split"), panel_timeline, SLOT(split_at_playhead()), QKeySequence("Ctrl+K"))->setProperty("id", "split");
}

void MenuHelper::set_bool_action_checked(QAction *a) {
    if (!a->data().isNull()) {
        bool* variable = reinterpret_cast<bool*>(a->data().value<quintptr>());
        a->setChecked(*variable);
    }
}

void MenuHelper::set_int_action_checked(QAction *a, const int& i) {
    if (!a->data().isNull()) {
        a->setChecked(a->data() == i);
    }
}

#include <QPushButton>
void MenuHelper::set_button_action_checked(QAction *a) {
    a->setChecked(reinterpret_cast<QPushButton*>(a->data().value<quintptr>())->isChecked());
}

void MenuHelper::toggle_bool_action() {
    QAction* action = static_cast<QAction*>(sender());
    bool* variable = reinterpret_cast<bool*>(action->data().value<quintptr>());
    *variable = !(*variable);
    update_ui(false);
}

void MenuHelper::set_titlesafe_from_menu() {
    double tsa = static_cast<QAction*>(sender())->data().toDouble();

    if (qIsNaN(tsa)) {

        // disable title safe area
        config.show_title_safe_area = false;

    } else {

        // using title safe area
        config.show_title_safe_area = true;

        // are we using the default area aspect ratio, or a specific one
        if (qIsNull(tsa)) {

            // default title safe area
            config.use_custom_title_safe_ratio = false;

        } else {

            // using a specific aspect ratio
            config.use_custom_title_safe_ratio = true;

            if (tsa < 0.0) {

               // set a custom title safe area
               QString input;
               bool invalid = false;
               QRegExp arTest("[0-9.]+:[0-9.]+");

               do {
                   if (invalid) {
                       QMessageBox::critical(Olive::MainWindow, tr("Invalid aspect ratio"), tr("The aspect ratio '%1' is invalid. Please try again.").arg(input));
                   }

                   input = QInputDialog::getText(Olive::MainWindow, tr("Enter custom aspect ratio"), tr("Enter the aspect ratio to use for the title/action safe area (e.g. 16:9):"));
                   invalid = !arTest.exactMatch(input) && !input.isEmpty();
               } while (invalid);

               if (!input.isEmpty()) {
                   QStringList inputList = input.split(':');
                   config.custom_title_safe_ratio = inputList.at(0).toDouble()/inputList.at(1).toDouble();
               }

           } else {

               // specified tsa is a specific custom aspect ratio
               config.custom_title_safe_ratio = tsa;
           }

        }

    }

    panel_sequence_viewer->viewer_widget->update();
}

void MenuHelper::set_autoscroll() {
    QAction* action = static_cast<QAction*>(sender());
    config.autoscroll = action->data().toInt();
}

void MenuHelper::menu_click_button() {
    reinterpret_cast<QPushButton*>(static_cast<QAction*>(sender())->data().value<quintptr>())->click();
}

void MenuHelper::set_timecode_view() {
    QAction* action = static_cast<QAction*>(sender());
    config.timecode_view = action->data().toInt();
    update_ui(false);
}
