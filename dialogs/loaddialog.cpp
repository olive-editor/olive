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

LoadDialog::LoadDialog(QWidget *parent, bool autorecovery) : QDialog(parent) {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

    layout->addWidget(new QLabel("Loading '" + project_url.mid(project_url.lastIndexOf('/')+1) + "'..."));

	bar = new QProgressBar();
	bar->setValue(50);
	layout->addWidget(bar);

    cancel_button = new QPushButton("Cancel");
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));

    hboxLayout = new QHBoxLayout();
	hboxLayout->addStretch();
	hboxLayout->addWidget(cancel_button);
	hboxLayout->addStretch();

	layout->addLayout(hboxLayout);

    update();

    LoadThread* lt = new LoadThread(this, autorecovery);
    QObject::connect(lt, SIGNAL(success()), this, SLOT(thread_done()));
    lt->start();
}

void LoadDialog::thread_done() {
//    project_model.update_data();
    close();
}
