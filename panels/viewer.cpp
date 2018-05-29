#include "viewer.h"
#include "ui_viewer.h"

#include "playback/audio.h"
#include "timeline.h"
#include "project/sequence.h"
#include "panels/panels.h"

extern "C" {
	#include <libavcodec/avcodec.h>
}

Viewer::Viewer(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Viewer)
{
	ui->setupUi(this);
	ui->glViewerPane->child = ui->openGLWidget;
	viewer_widget = ui->openGLWidget;
	set_sequence(NULL);
}

Viewer::~Viewer()
{
	init_audio(NULL);

	delete ui;
}

void Viewer::set_sequence(Sequence* s) {
	bool null_sequence = (s == NULL);
	sequence = s;

	ui->openGLWidget->setEnabled(!null_sequence);
	ui->openGLWidget->setVisible(!null_sequence);
	ui->pushButton->setEnabled(!null_sequence);
	ui->pushButton_2->setEnabled(!null_sequence);
	ui->pushButton_3->setEnabled(!null_sequence);
	ui->pushButton_4->setEnabled(!null_sequence);
	ui->pushButton_5->setEnabled(!null_sequence);

	if (!null_sequence) {
		ui->glViewerPane->aspect_ratio = (float) sequence->width / (float) sequence->height;
		ui->glViewerPane->adjust();
	}

	init_audio(s);
	update();
}

void Viewer::on_pushButton_clicked()
{
	panel_timeline->go_to_start();
}

void Viewer::on_pushButton_5_clicked()
{
	panel_timeline->go_to_end();
}

void Viewer::on_pushButton_2_clicked()
{
	panel_timeline->previous_frame();
}

void Viewer::on_pushButton_4_clicked()
{
	panel_timeline->next_frame();
}

void Viewer::on_pushButton_3_clicked()
{
	if (panel_timeline->toggle_play()) {
		// playing
		ui->pushButton_3->setIcon(QIcon(":/icons/pause.png"));
	} else {
		ui->pushButton_3->setIcon(QIcon(":/icons/play.png"));
	}
}
