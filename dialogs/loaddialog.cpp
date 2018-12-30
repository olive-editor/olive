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
