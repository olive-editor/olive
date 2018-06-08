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

#include <QDebug>
#include <QStyleFactory>

void MainWindow::setup_layout() {
	panel_project = new Project(this);
	panel_effect_controls = new EffectControls(this);
	panel_viewer = new Viewer(this);
	panel_timeline = new Timeline(this);

	addDockWidget(Qt::TopDockWidgetArea, panel_project);
	addDockWidget(Qt::TopDockWidgetArea, panel_effect_controls);
	addDockWidget(Qt::TopDockWidgetArea, panel_viewer);
	addDockWidget(Qt::BottomDockWidgetArea, panel_timeline);

	setCentralWidget(NULL);
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

	setup_layout();
}

MainWindow::~MainWindow()
{
	delete ui;
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
	if (panel_timeline->sequence != NULL) e.set_defaults(panel_timeline->sequence);
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
