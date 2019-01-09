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
#include "loaddialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

#include "panels/panels.h"
#include "panels/project.h"
#include "io/loadthread.h"
#include "playback/playback.h"
#include "ui/sourcetable.h"
#include "mainwindow.h"

LoadDialog::LoadDialog(QWidget *parent, bool autorecovery) : QDialog(parent) {
	setWindowTitle("Loading...");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	layout->addWidget(new QLabel("Loading '" + project_url.mid(project_url.lastIndexOf('/')+1) + "'..."));

	bar = new QProgressBar();
	bar->setValue(0);
	layout->addWidget(bar);

	cancel_button = new QPushButton("Cancel");
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(cancel()));

	hboxLayout = new QHBoxLayout();
	hboxLayout->addStretch();
	hboxLayout->addWidget(cancel_button);
	hboxLayout->addStretch();

	layout->addLayout(hboxLayout);

	update();

	lt = new LoadThread(this, autorecovery);
	QObject::connect(lt, SIGNAL(success()), this, SLOT(thread_done()));
	QObject::connect(lt, SIGNAL(error()), this, SLOT(die()));
	QObject::connect(lt, SIGNAL(report_progress(int)), bar, SLOT(setValue(int)));
	lt->start();
}

void LoadDialog::cancel() {
	lt->cancel();
	lt->wait();
	die();
}

void LoadDialog::die() {
	mainWindow->new_project();
	reject();
}

void LoadDialog::thread_done() {
	accept();
}
