#include "menuhelper.h"

#include "oliveglobal.h"

#include "mainwindow.h"

MenuHelper Olive::MenuHelper;

void MenuHelper::make_new_menu(QMenu *parent) {
    parent->addAction(tr("&Project"), Olive::Global.data(), SLOT(new_project()), QKeySequence("Ctrl+N"))->setProperty("id", "newproj");
    parent->addSeparator();
    parent->addAction(tr("&Sequence"), Olive::MainWindow, SLOT(new_sequence()), QKeySequence("Ctrl+Shift+N"))->setProperty("id", "newseq");
    parent->addAction(tr("&Folder"), Olive::MainWindow, SLOT(new_folder()))->setProperty("id", "newfolder");
}

void MenuHelper::make_inout_menu(QMenu *parent) {
    parent->addAction(tr("Set In Point"), Olive::MainWindow, SLOT(set_in_point()), QKeySequence("I"))->setProperty("id", "setinpoint");
    parent->addAction(tr("Set Out Point"), Olive::MainWindow, SLOT(set_out_point()), QKeySequence("O"))->setProperty("id", "setoutpoint");
    parent->addSeparator();
    parent->addAction(tr("Reset In Point"), Olive::MainWindow, SLOT(clear_in()))->setProperty("id", "resetin");
    parent->addAction(tr("Reset Out Point"), Olive::MainWindow, SLOT(clear_out()))->setProperty("id", "resetout");
    parent->addAction(tr("Clear In/Out Point"), Olive::MainWindow, SLOT(clear_inout()), QKeySequence("G"))->setProperty("id", "clearinout");
}
