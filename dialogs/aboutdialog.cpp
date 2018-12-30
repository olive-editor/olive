#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle("About Olive");
	setMaximumWidth(360);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setSpacing(20);
	setLayout(layout);

	QLabel* label = new QLabel("<html><head/><body><p><img src=\":/icons/olive-splash.png\"/></p><p><a href=\"https://www.olivevideoeditor.org/\"><span style=\" text-decoration: underline; color:#007af4;\">https://www.olivevideoeditor.org/</span></a></p><p>Olive is a non-linear video editor. This software is free and protected by the GNU GPL.</p><p>Olive Team is obliged to inform users that Olive source code is available for download from its website.</p><p>Olive uses (at least) the following libraries in accordance with the GNU GPL/LGPL:</p><p>Qt, FFmpeg, libass, libfreetype, libmp3lame, libopenjpeg, libopus, libtheora, libtwolame, libvpx, libwavpack, libwebp, libx264, libx265, lzma, bzlib, zlib, libvidstab, libvorbis.</p></body></html>");
	label->setAlignment(Qt::AlignCenter);
	label->setWordWrap(true);
	layout->addWidget(label);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	buttons->setCenterButtons(true);
	layout->addWidget(buttons);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
}
