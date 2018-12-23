#include "mediapropertiesdialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>

#include "project/footage.h"
#include "project/media.h"
#include "panels/project.h"
#include "project/undo.h"

MediaPropertiesDialog::MediaPropertiesDialog(QWidget *parent, Media *i) :
    QDialog(parent),
    item(i)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QGridLayout* grid = new QGridLayout();
	setLayout(grid);

    Footage* f = item->to_footage();
    if (f->video_tracks.size() > 0) {
        interlacing_box = new QComboBox();
        interlacing_box->addItem("Auto (" + get_interlacing_name(f->video_tracks.at(0)->video_auto_interlacing) + ")");
        interlacing_box->addItem(get_interlacing_name(VIDEO_PROGRESSIVE));
        interlacing_box->addItem(get_interlacing_name(VIDEO_TOP_FIELD_FIRST));
        interlacing_box->addItem(get_interlacing_name(VIDEO_BOTTOM_FIELD_FIRST));

        interlacing_box->setCurrentIndex((f->video_tracks.at(0)->video_auto_interlacing == f->video_tracks.at(0)->video_interlacing) ? 0 : f->video_tracks.at(0)->video_interlacing + 1);

        grid->addWidget(new QLabel("Interlacing:"), 0, 0);
        grid->addWidget(interlacing_box, 0, 1);
    }

    name_box = new QLineEdit(item->get_name());
	grid->addWidget(new QLabel("Name:"), 1, 0);
    grid->addWidget(name_box, 1, 1);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttons->setCenterButtons(true);
	grid->addWidget(buttons, 2, 0, 1, 2);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void MediaPropertiesDialog::accept() {
	ComboAction* ca = new ComboAction();

	//set interlacing
    Footage* f = item->to_footage();
	if (interlacing_box->currentIndex() > 0) {
        ca->append(new SetInt(&f->video_tracks.at(0)->video_interlacing, interlacing_box->currentIndex() - 1));
	} else {
        ca->append(new SetInt(&f->video_tracks.at(0)->video_interlacing, f->video_tracks.at(0)->video_auto_interlacing));
	}

	//set name
    MediaRename* mr = new MediaRename(item, name_box->text());

	ca->append(mr);
	ca->appendPost(new CloseAllClipsCommand());
    ca->appendPost(new UpdateFootageTooltip(item));

	undo_stack.push(ca);

    QDialog::accept();
}
