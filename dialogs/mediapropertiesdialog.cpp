#include "mediapropertiesdialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>
#include <QGroupBox>
#include <QListWidget>

#include "project/footage.h"
#include "project/media.h"
#include "panels/project.h"
#include "project/undo.h"

MediaPropertiesDialog::MediaPropertiesDialog(QWidget *parent, Media *i) :
    QDialog(parent),
    item(i)
{
    setWindowTitle("\"" + i->data(0, Qt::DisplayRole).toString() + "\" Properties");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QGridLayout* grid = new QGridLayout();
	setLayout(grid);

    int row = 0;

    Footage* f = item->to_footage();

    grid->addWidget(new QLabel("Tracks:"), row, 0, 1, 2);
    row++;

    track_list = new QListWidget();
    for (int i=0;i<f->video_tracks.size();i++) {
        const FootageStream& fs = f->video_tracks.at(i);
        QListWidgetItem* item = new QListWidgetItem("Video " + QString::number(fs.file_index) + ": " + QString::number(fs.video_width) + "x" + QString::number(fs.video_height) + " " + QString::number(fs.video_frame_rate) + "FPS");
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(fs.enabled ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole+1, fs.file_index);
        track_list->addItem(item);
    }
    for (int i=0;i<f->audio_tracks.size();i++) {
        const FootageStream& fs = f->audio_tracks.at(i);
        QListWidgetItem* item = new QListWidgetItem("Audio " + QString::number(fs.file_index) + ": " + QString::number(fs.audio_frequency) + "Hz " + QString::number(fs.audio_channels) + " channels");
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(fs.enabled ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole+1, fs.file_index);
        track_list->addItem(item);
    }
    grid->addWidget(track_list, row, 0, 1, 2);
    row++;

    if (f->video_tracks.size() > 0) {
        interlacing_box = new QComboBox();
        interlacing_box->addItem("Auto (" + get_interlacing_name(f->video_tracks.at(0).video_auto_interlacing) + ")");
        interlacing_box->addItem(get_interlacing_name(VIDEO_PROGRESSIVE));
        interlacing_box->addItem(get_interlacing_name(VIDEO_TOP_FIELD_FIRST));
        interlacing_box->addItem(get_interlacing_name(VIDEO_BOTTOM_FIELD_FIRST));

        interlacing_box->setCurrentIndex((f->video_tracks.at(0).video_auto_interlacing == f->video_tracks.at(0).video_interlacing) ? 0 : f->video_tracks.at(0).video_interlacing + 1);

        grid->addWidget(new QLabel("Interlacing:"), row, 0);
        grid->addWidget(interlacing_box, row, 1);

        row++;
    }

    name_box = new QLineEdit(item->get_name());
    grid->addWidget(new QLabel("Name:"), row, 0);
    grid->addWidget(name_box, row, 1);
    row++;

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttons->setCenterButtons(true);
    grid->addWidget(buttons, row, 0, 1, 2);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void MediaPropertiesDialog::accept() {
    Footage* f = item->to_footage();

	ComboAction* ca = new ComboAction();

    // set track enable
    for (int i=0;i<track_list->count();i++) {
        QListWidgetItem* item = track_list->item(i);
        const QVariant& data = item->data(Qt::UserRole+1);
        if (!data.isNull()) {
            int index = data.toInt();
            bool found = false;
            for (int j=0;j<f->video_tracks.size();j++) {
                if (f->video_tracks.at(j).file_index == index) {
                    f->video_tracks[j].enabled = (item->checkState() == Qt::Checked);
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (int j=0;j<f->audio_tracks.size();j++) {
                    if (f->audio_tracks.at(j).file_index == index) {
                        f->audio_tracks[j].enabled = (item->checkState() == Qt::Checked);
                        break;
                    }
                }
            }
        }
    }

    // set interlacing
    if (f->video_tracks.size() > 0) {
        if (interlacing_box->currentIndex() > 0) {
            ca->append(new SetInt(&f->video_tracks[0].video_interlacing, interlacing_box->currentIndex() - 1));
        } else {
            ca->append(new SetInt(&f->video_tracks[0].video_interlacing, f->video_tracks.at(0).video_auto_interlacing));
        }
    }

    // set name
    MediaRename* mr = new MediaRename(item, name_box->text());

	ca->append(mr);
	ca->appendPost(new CloseAllClipsCommand());
    ca->appendPost(new UpdateFootageTooltip(item));

	undo_stack.push(ca);

    QDialog::accept();
}
