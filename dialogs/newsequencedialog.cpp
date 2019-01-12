#include "newsequencedialog.h"

#include "panels/panels.h"
#include "panels/project.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "project/clip.h"
#include "panels/timeline.h"
#include "playback/playback.h"
#include "project/media.h"

#include <QVariant>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>

extern "C" {
	#include <libavcodec/avcodec.h>
}

NewSequenceDialog::NewSequenceDialog(QWidget *parent, Media *existing) :
	QDialog(parent),
	existing_item(existing)
{
	setup_ui();

	if (existing != nullptr) {
		existing_sequence = existing->to_sequence();
		setWindowTitle("Editing \"" + existing_sequence->name + "\"");

		width_numeric->setValue(existing_sequence->width);
		height_numeric->setValue(existing_sequence->height);
		int comp_rate = qRound(existing_sequence->frame_rate*100);
		for (int i=0;i<frame_rate_combobox->count();i++) {
			if (qRound(frame_rate_combobox->itemData(i).toDouble()*100) == comp_rate) {
				frame_rate_combobox->setCurrentIndex(i);
				break;
			}
		}
        sequence_name_edit->setText(existing_sequence->name);
		for (int i=0;i<audio_frequency_combobox->count();i++) {
			if (audio_frequency_combobox->itemData(i) == existing_sequence->audio_frequency) {
				audio_frequency_combobox->setCurrentIndex(i);
				break;
			}
		}
	} else {
		existing_sequence = nullptr;
		setWindowTitle("New Sequence");
	}
}

NewSequenceDialog::~NewSequenceDialog()
{}

void NewSequenceDialog::set_sequence_name(const QString& s) {
    sequence_name_edit->setText(s);
}

void NewSequenceDialog::create() {
	if (existing_sequence == nullptr) {
		Sequence* s = new Sequence();

        s->name = sequence_name_edit->text();
		s->width = width_numeric->value();
		s->height = height_numeric->value();
		s->frame_rate = frame_rate_combobox->currentData().toDouble();
		s->audio_frequency = audio_frequency_combobox->currentData().toInt();
		s->audio_layout = AV_CH_LAYOUT_STEREO;

		ComboAction* ca = new ComboAction();
		panel_project->new_sequence(ca, s, true, nullptr);
		undo_stack.push(ca);
	} else {
		ComboAction* ca = new ComboAction();

		double multiplier = frame_rate_combobox->currentData().toDouble() / existing_sequence->frame_rate;

		EditSequenceCommand* esc = new EditSequenceCommand(existing_item, existing_sequence);
        esc->name = sequence_name_edit->text();
		esc->width = width_numeric->value();
		esc->height = height_numeric->value();
		esc->frame_rate = frame_rate_combobox->currentData().toDouble();
		esc->audio_frequency = audio_frequency_combobox->currentData().toInt();
		esc->audio_layout = AV_CH_LAYOUT_STEREO;
		ca->append(esc);

		for (int i=0;i<existing_sequence->clips.size();i++) {
			Clip* c = existing_sequence->clips.at(i);
			if (c != nullptr) {
				c->refactor_frame_rate(ca, multiplier, true);
			}
		}

		undo_stack.push(ca);
	}

	accept();
}

void NewSequenceDialog::preset_changed(int index)
{
	switch (index) {
	case 0: // FILM 4K
		width_numeric->setValue(4096);
		height_numeric->setValue(2160);
		break;
	case 1: // TV 4K
		width_numeric->setValue(3840);
		height_numeric->setValue(2160);
		break;
	case 2: // 1080p
		width_numeric->setValue(1920);
		height_numeric->setValue(1080);
		break;
	case 3: // 720p
		width_numeric->setValue(1280);
		height_numeric->setValue(720);
		break;
	case 4: // 480p
		width_numeric->setValue(640);
		height_numeric->setValue(480);
		break;
	case 5: // 360p
		width_numeric->setValue(640);
		height_numeric->setValue(360);
		break;
	case 6: // 240p
		width_numeric->setValue(320);
		height_numeric->setValue(240);
		break;
	case 7: // 144p
		width_numeric->setValue(192);
		height_numeric->setValue(144);
		break;
	case 8: // NTSC (480i)
		width_numeric->setValue(720);
		height_numeric->setValue(480);
		break;
	case 9: // PAL (576i)
		width_numeric->setValue(720);
		height_numeric->setValue(576);
		break;
	}
}

void NewSequenceDialog::setup_ui() {
	QVBoxLayout* verticalLayout = new QVBoxLayout(this);

	QWidget* widget = new QWidget(this);

    QHBoxLayout* preset_layout = new QHBoxLayout(widget);
    preset_layout->setContentsMargins(0, 0, 0, 0);

    preset_layout->addWidget(new QLabel("Preset:"));

	preset_combobox = new QComboBox(widget);

	preset_combobox->addItem("Film 4K");
	preset_combobox->addItem("TV 4K (Ultra HD/2160p)");
	preset_combobox->addItem("1080p");
	preset_combobox->addItem("720p");
	preset_combobox->addItem("480p");
	preset_combobox->addItem("360p");
	preset_combobox->addItem("240p");
	preset_combobox->addItem("144p");
	preset_combobox->addItem("NTSC (480i)");
	preset_combobox->addItem("PAL (576i)");
	preset_combobox->addItem("Custom");
	preset_combobox->setCurrentIndex(2);

    preset_layout->addWidget(preset_combobox);

	verticalLayout->addWidget(widget);

    QGroupBox* videoGroupBox = new QGroupBox(this);
    videoGroupBox->setTitle("Video");

    QGridLayout* videoLayout = new QGridLayout(videoGroupBox);

    videoLayout->addWidget(new QLabel("Width:"), 0, 0, 1, 1);
    width_numeric = new QSpinBox(videoGroupBox);
    width_numeric->setMaximum(9999);
    width_numeric->setValue(1920);
    videoLayout->addWidget(width_numeric, 0, 2, 1, 2);

    videoLayout->addWidget(new QLabel("Height:"), 1, 0, 1, 2);
    height_numeric = new QSpinBox(videoGroupBox);
	height_numeric->setMaximum(9999);
	height_numeric->setValue(1080);
    videoLayout->addWidget(height_numeric, 1, 2, 1, 2);

    videoLayout->addWidget(new QLabel("Frame Rate:"), 2, 0, 1, 1);
    frame_rate_combobox = new QComboBox(videoGroupBox);
    frame_rate_combobox->addItem("10 FPS", 10.0);
    frame_rate_combobox->addItem("12.5 FPS", 12.5);
    frame_rate_combobox->addItem("15 FPS", 15.0);
    frame_rate_combobox->addItem("23.976 FPS", 23.976);
    frame_rate_combobox->addItem("24 FPS", 24.0);
    frame_rate_combobox->addItem("25 FPS", 25.0);
    frame_rate_combobox->addItem("29.97 FPS", 29.97);
    frame_rate_combobox->addItem("30 FPS", 30.0);
    frame_rate_combobox->addItem("50 FPS", 50.0);
    frame_rate_combobox->addItem("59.94 FPS", 59.94);
    frame_rate_combobox->addItem("60 FPS", 60.0);
    frame_rate_combobox->setCurrentIndex(6);
    videoLayout->addWidget(frame_rate_combobox, 2, 2, 1, 2);

    videoLayout->addWidget(new QLabel("Pixel Aspect Ratio:"), 4, 0, 1, 1);
    par_combobox = new QComboBox(videoGroupBox);
    par_combobox->addItem("Square Pixels (1.0)");
    videoLayout->addWidget(par_combobox, 4, 2, 1, 2);

    videoLayout->addWidget(new QLabel("Interlacing:"), 6, 0, 1, 1);
    interlacing_combobox = new QComboBox(videoGroupBox);
	interlacing_combobox->addItem("None (Progressive)");
//	interlacing_combobox->addItem("Upper Field First");
//	interlacing_combobox->addItem("Lower Field First");
    videoLayout->addWidget(interlacing_combobox, 6, 2, 1, 2);

    verticalLayout->addWidget(videoGroupBox);

    QGroupBox* audioGroupBox = new QGroupBox(this);
    audioGroupBox->setTitle("Audio");

    QGridLayout* audioLayout = new QGridLayout(audioGroupBox);

    audioLayout->addWidget(new QLabel("Sample Rate: "), 0, 0, 1, 1);

    audio_frequency_combobox = new QComboBox(audioGroupBox);
	audio_frequency_combobox->addItem("22050 Hz", 22050);
	audio_frequency_combobox->addItem("24000 Hz", 24000);
	audio_frequency_combobox->addItem("32000 Hz", 32000);
	audio_frequency_combobox->addItem("44100 Hz", 44100);
	audio_frequency_combobox->addItem("48000 Hz", 48000);
	audio_frequency_combobox->addItem("88200 Hz", 88200);
	audio_frequency_combobox->addItem("96000 Hz", 96000);
	audio_frequency_combobox->setCurrentIndex(4);

    audioLayout->addWidget(audio_frequency_combobox, 0, 1, 1, 1);

    verticalLayout->addWidget(audioGroupBox);

    QWidget* nameWidget = new QWidget(this);
    QHBoxLayout* nameLayout = new QHBoxLayout(nameWidget);
    nameLayout->setContentsMargins(0, 0, 0, 0);

    nameLayout->addWidget(new QLabel("Name:"));

    sequence_name_edit = new QLineEdit(nameWidget);

    nameLayout->addWidget(sequence_name_edit);

    verticalLayout->addWidget(nameWidget);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
	buttonBox->setOrientation(Qt::Horizontal);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
	buttonBox->setCenterButtons(true);

	verticalLayout->addWidget(buttonBox);

	connect(preset_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(preset_changed(int)));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(create()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
