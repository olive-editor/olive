#include "newsequencedialog.h"
#include "ui_newsequencedialog.h"

#include "panels/panels.h"
#include "panels/project.h"
#include "project/sequence.h"

#include <QVariant>
#include <QDebug>

extern "C" {
	#include <libavcodec/avcodec.h>
}

NewSequenceDialog::NewSequenceDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::NewSequenceDialog)
{
	ui->setupUi(this);

	ui->frame_rate_combobox->addItem("10 FPS", 10.0f);
	ui->frame_rate_combobox->addItem("12.5 FPS", 12.5f);
	ui->frame_rate_combobox->addItem("15 FPS", 15.0f);
    ui->frame_rate_combobox->addItem("23.976 FPS", 23.976f);
    ui->frame_rate_combobox->addItem("24 FPS", 24.0f);
    ui->frame_rate_combobox->addItem("25 FPS", 25.0f);
	ui->frame_rate_combobox->addItem("29.97 FPS", 29.97f);
	ui->frame_rate_combobox->addItem("30 FPS", (float) 30.0f);
	ui->frame_rate_combobox->addItem("50 FPS", (float) 50.0f);
	ui->frame_rate_combobox->addItem("59.94 FPS", (float) 59.94f);
	ui->frame_rate_combobox->addItem("60 FPS", (float) 60.0f);
    ui->frame_rate_combobox->setCurrentIndex(6);

	ui->audio_frequency_combobox->addItem("22050 Hz", 22050);
	ui->audio_frequency_combobox->addItem("24000 Hz", 24000);
	ui->audio_frequency_combobox->addItem("32000 Hz", 32000);
	ui->audio_frequency_combobox->addItem("44100 Hz", 44100);
	ui->audio_frequency_combobox->addItem("48000 Hz", 48000);
	ui->audio_frequency_combobox->addItem("88200 Hz", 88200);
	ui->audio_frequency_combobox->addItem("96000 Hz", 96000);
	ui->audio_frequency_combobox->setCurrentIndex(4);
}

NewSequenceDialog::~NewSequenceDialog()
{
	delete ui;
}

void NewSequenceDialog::set_sequence_name(const QString& s) {
	ui->lineEdit->setText(s);
}

void NewSequenceDialog::on_buttonBox_accepted()
{
	Sequence* s = new Sequence();

	s->name = ui->lineEdit->text();
	s->width = ui->width_numeric->value();
	s->height = ui->height_numeric->value();
	s->frame_rate = ui->frame_rate_combobox->currentData().toFloat();
	s->audio_frequency = ui->audio_frequency_combobox->currentData().toInt();
	s->audio_layout = AV_CH_LAYOUT_STEREO;

    panel_project->new_sequence(s, true, NULL);
}

void NewSequenceDialog::on_comboBox_currentIndexChanged(int index)
{
	switch (index) {
	case 0: // FILM 4K
		ui->width_numeric->setValue(4096);
		ui->height_numeric->setValue(2160);

		break;
	case 1: // TV 4K
		ui->width_numeric->setValue(3840);
		ui->height_numeric->setValue(2160);
		break;
	case 2: // 1080p
		ui->width_numeric->setValue(1920);
		ui->height_numeric->setValue(1080);
		break;
	case 3: // 720p
		ui->width_numeric->setValue(1280);
		ui->height_numeric->setValue(720);
		break;
	case 4: // 480p
		ui->width_numeric->setValue(640);
		ui->height_numeric->setValue(480);
		break;
	case 5: // 360p
		ui->width_numeric->setValue(640);
		ui->height_numeric->setValue(360);
		break;
	case 6: // 240p
		ui->width_numeric->setValue(320);
		ui->height_numeric->setValue(240);
		break;
	case 7: // 144p
		ui->width_numeric->setValue(192);
		ui->height_numeric->setValue(144);
		break;
	case 8: // NTSC (480i)
		ui->width_numeric->setValue(720);
		ui->height_numeric->setValue(480);
		break;
	case 9: // PAL (576i)
		ui->width_numeric->setValue(720);
		ui->height_numeric->setValue(576);
		break;
	}
}
