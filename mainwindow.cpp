#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "io/config.h"

#include "panels/panels.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/timeline.h"

#include "dialogs/aboutdialog.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/exportdialog.h"

#include "ui_timeline.h"

#include <QDebug>
#include <QStyleFactory>
#include <QMessageBox>
#include <QFileDialog>

#define OLIVE_FILE_FILTER "Olive Project (*.ove)"

void MainWindow::setup_layout() {
    panel_project->show();
    panel_effect_controls->show();
    panel_viewer->show();
    panel_timeline->show();

	addDockWidget(Qt::TopDockWidgetArea, panel_project);
	addDockWidget(Qt::TopDockWidgetArea, panel_effect_controls);
	addDockWidget(Qt::TopDockWidgetArea, panel_viewer);
	addDockWidget(Qt::BottomDockWidgetArea, panel_timeline);
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	// set up style?

    qApp->setStyle(QStyleFactory::create("Fusion"));

	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53,53,53));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(25,25,25));
	darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
	darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
	darkPalette.setColor(QPalette::ToolTipText, Qt::white);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(53,53,53));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);

	// end style

	setWindowState(Qt::WindowMaximized);

	ui->setupUi(this);

	setWindowTitle("Olive (May 2018 | Pre-Alpha)");
	statusBar()->showMessage("Welcome to Olive::Qt");

    setCentralWidget(NULL);

    // TODO maybe replace these with non-pointers later on?
    panel_project = new Project(this);
    panel_effect_controls = new EffectControls(this);
    panel_viewer = new Viewer(this);
    panel_timeline = new Timeline(this);

	setup_layout();
}

MainWindow::~MainWindow()
{
	delete ui;

    delete panel_project;
    delete panel_effect_controls;
    delete panel_viewer;
    delete panel_timeline;
}

void MainWindow::on_action_Import_triggered()
{
	panel_project->import_dialog();
}

void MainWindow::on_actionExit_triggered()
{
	QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog a(this);
	a.exec();
}

void MainWindow::on_actionDelete_triggered()
{
	if (panel_timeline->focused()) {
		panel_timeline->delete_selection(false);
	}
}

void MainWindow::on_actionSelect_All_triggered()
{
	if (panel_timeline->focused()) {
		panel_timeline->select_all();
	}
}

void MainWindow::on_actionSequence_triggered()
{
    NewSequenceDialog nsd(this);
	nsd.set_sequence_name(panel_project->get_next_sequence_name());
	nsd.exec();
}

void MainWindow::on_actionZoom_In_triggered()
{
	if (panel_timeline->focused()) {
		panel_timeline->zoom_in();
	}
}

void MainWindow::on_actionZoom_out_triggered()
{
	if (panel_timeline->focused()) {
		panel_timeline->zoom_out();
	}
}

void MainWindow::on_actionTimeline_Track_Lines_toggled(bool e)
{
	show_track_lines = e;
	panel_timeline->redraw_all_clips();
}

void MainWindow::on_actionExport_triggered()
{
    ExportDialog e(this);
	e.exec();
}

void MainWindow::on_actionProject_2_toggled(bool arg1)
{
	panel_project->setVisible(arg1);
}

void MainWindow::on_actionEffect_Controls_toggled(bool arg1)
{
	panel_effect_controls->setVisible(arg1);
}

void MainWindow::on_actionViewer_toggled(bool arg1)
{
	panel_viewer->setVisible(arg1);
}

void MainWindow::on_actionTimeline_toggled(bool arg1)
{
	panel_timeline->setVisible(arg1);
}

void MainWindow::on_actionRipple_Delete_triggered()
{
	panel_timeline->delete_selection(true);
}

void MainWindow::on_action_Undo_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->undo();
    }
}

void MainWindow::on_action_Redo_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->redo();
    }
}

void MainWindow::on_actionSplit_at_Playhead_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->split_at_playhead();
    }
}

void MainWindow::on_actionCu_t_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->copy(true);
    }
}

void MainWindow::on_actionCop_y_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->copy(false);
    }
}

void MainWindow::on_action_Paste_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->paste();
    }
}

bool MainWindow::save_project_as() {
    QString fn = QFileDialog::getSaveFileName(this, "Save Project As...", "", OLIVE_FILE_FILTER);
    if (!fn.isEmpty()) {
        project_url = fn;
        panel_project->save_project();
        return true;
    }
    return false;
}

bool MainWindow::save_project() {
    if (project_url.isEmpty()) {
        return save_project_as();
    } else {
        panel_project->save_project();
        return true;
    }
}

bool MainWindow::can_close_project() {
    if (project_changed) {
        int r = QMessageBox::question(this, "Unsaved Project", "This project has changed since it was last saved. Would you like to save it before closing?", QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
        if (r == QMessageBox::Yes) {
            return save_project();
        } else if (r == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

void MainWindow::on_action_Save_Project_triggered()
{
    save_project();
}

void MainWindow::on_action_Open_Project_triggered()
{
    if (can_close_project()) {
        QString fn = QFileDialog::getOpenFileName(this, "Open Project...", "", OLIVE_FILE_FILTER);
        if (!fn.isEmpty()) {
            project_url = fn;
            panel_project->load_project();
        }
    }
}

void MainWindow::on_actionProject_triggered()
{
    if (can_close_project()) {
        panel_project->new_project();
    }
}

void MainWindow::on_actionSave_Project_As_triggered()
{
    save_project_as();
}

void MainWindow::on_actionDeselect_All_triggered()
{
    if (panel_timeline->focused()) {
        panel_timeline->deselect();
    }
}

void MainWindow::on_actionGo_to_start_triggered()
{
    if (panel_timeline->focused() || panel_viewer->hasFocus()) {
        panel_timeline->go_to_start();
    }
}

void MainWindow::on_actionReset_to_default_layout_triggered()
{
    setup_layout();
}

void MainWindow::on_actionPrevious_Frame_triggered()
{
    if (panel_timeline->focused() || panel_viewer->hasFocus()) {
        panel_timeline->previous_frame();
    }
}

void MainWindow::on_actionNext_Frame_triggered()
{
    if (panel_timeline->focused() || panel_viewer->hasFocus()) {
        panel_timeline->next_frame();
    }
}

void MainWindow::on_actionGo_to_End_triggered()
{
    if (panel_timeline->focused() || panel_viewer->hasFocus()) {
        panel_timeline->go_to_end();
    }
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    if (panel_timeline->focused() || panel_viewer->hasFocus()) {
        panel_timeline->toggle_play();
    }
}

void MainWindow::on_actionCrash_triggered()
{
    if (QMessageBox::warning(this, "Are you sure you want to crash?", "WARNING: This is a debugging function designed to crash the program. Olive WILL crash and any unsaved progress WILL be lost. Are you sure you wish to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        // intentionally tries to crash the program - mostly used for debugging
        Timeline* temp = NULL;
        temp->snapped = true;
    }
}

void MainWindow::on_actionEdit_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolEditButton->click();
}

void MainWindow::on_actionToggle_Snapping_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->snappingButton->click();
}

void MainWindow::on_actionPointer_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolArrowButton->click();
}

void MainWindow::on_actionRazor_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolRazorButton->click();
}

void MainWindow::on_actionRipple_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolRippleButton->click();
}

void MainWindow::on_actionRolling_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolRollingButton->click();
}

void MainWindow::on_actionSlip_Tool_triggered()
{
    if (panel_timeline->focused()) panel_timeline->ui->toolSlipButton->click();
}
