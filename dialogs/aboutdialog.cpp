/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
