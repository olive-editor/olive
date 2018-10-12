#include "mediapropertiesdialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>

#include "io/media.h"
#include "panels/project.h"

MediaPropertiesDialog::MediaPropertiesDialog(QWidget *parent) {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QGridLayout* grid = new QGridLayout();
	setLayout(grid);

	interlacing_box = new QComboBox();
	interlacing_box->addItem(get_interlacing_name(VIDEO_PROGRESSIVE), VIDEO_PROGRESSIVE);
	interlacing_box->addItem(get_interlacing_name(VIDEO_TOP_FIELD_FIRST), VIDEO_TOP_FIELD_FIRST);
	interlacing_box->addItem(get_interlacing_name(VIDEO_BOTTOM_FIELD_FIRST), VIDEO_BOTTOM_FIELD_FIRST);

	grid->addWidget(new QLabel("Interlacing:"), 0, 0);
	grid->addWidget(interlacing_box, 0, 1);

	grid->addWidget(new QLabel("Name:"), 1, 0);
	grid->addWidget(new QLineEdit("h"), 1, 1);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttons->setCenterButtons(true);
	grid->addWidget(buttons, 2, 0, 1, 2);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}
