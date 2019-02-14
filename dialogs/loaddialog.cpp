#include "loaddialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

#include "oliveglobal.h"

#include "panels/panels.h"

#include "io/loadthread.h"
#include "playback/playback.h"
#include "ui/sourcetable.h"
#include "mainwindow.h"

LoadDialog::LoadDialog(QWidget *parent, bool autorecovery) : QDialog(parent) {
	setWindowTitle(tr("Loading..."));
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->addWidget(new QLabel(tr("Loading '%1'...").arg(Olive::ActiveProjectFilename.mid(Olive::ActiveProjectFilename.lastIndexOf('/')+1)), this));

	bar = new QProgressBar(this);
	bar->setValue(0);
	layout->addWidget(bar);

	cancel_button = new QPushButton(tr("Cancel"), this);
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
    Olive::Global.data()->new_project();
	reject();
}

void LoadDialog::thread_done() {
	accept();
}
