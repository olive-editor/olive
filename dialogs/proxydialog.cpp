#include "proxydialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>

#include <QDebug>

#include "io/proxygenerator.h"
#include "project/footage.h"
#include "mainwindow.h"

ProxyDialog::ProxyDialog(QWidget *parent, const QVector<Footage *> &footage) :
	QDialog(parent),
	selected_footage(footage)
{
	// set dialog title
	setWindowTitle(tr("Create Proxy"));

	// set proxy folder name to "Proxy", depending on the user's language
	proxy_folder_name = tr("Proxy");

	// set up dialog's layout
	QGridLayout* layout = new QGridLayout(this);

	// set the video dimensions of the proxy
	layout->addWidget(new QLabel(tr("Dimensions:"), this), 0, 0);

	size_combobox = new QComboBox(this);
	size_combobox->addItem(tr("Same Size as Source"), 1.0);
	size_combobox->addItem(tr("Half Resolution (1/2)"), 0.5);
	size_combobox->addItem(tr("Quarter Resolution (1/4)"), 0.25);
	size_combobox->addItem(tr("Eighth Resolution (1/8)"), 0.125);
	size_combobox->addItem(tr("Sixteenth Resolution (1/16)"), 0.0625);
	layout->addWidget(size_combobox, 0, 1);

	// set the desired format of the proxy to create
	layout->addWidget(new QLabel(tr("Format:"), this), 1, 0);

	format_combobox = new QComboBox(this);
	format_combobox->addItem(tr("ProRes HQ"));
//	format_combobox->addItem(tr("ProRes SQ"));
//	format_combobox->addItem(tr("ProRes LT"));
//	format_combobox->addItem(tr("DNxHD"));
//	format_combobox->addItem(tr("H.264"));
	layout->addWidget(format_combobox, 1, 1);

	// set the location to place the proxies
	layout->addWidget(new QLabel(tr("Location:"), this), 2, 0);

	location_combobox = new QComboBox(this);
	location_combobox->addItem(tr("Same as Source (in \"%1\" folder)").arg(proxy_folder_name));
	location_combobox->addItem("");
	connect(location_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(location_changed(int)));
	layout->addWidget(location_combobox, 2, 1);

	// location_changed will set the default "location" items
	location_changed(0);

	// set up dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->setCenterButtons(true);
	layout->addWidget(buttons, 3, 0, 1, 2);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void ProxyDialog::accept() {
	QVector<ProxyInfo> info_list;
	bool yesForAll = false;

	for (int i=0;i<selected_footage.size();i++) {
		// loop through selected footage and send info to the proxy queue

		ProxyInfo info;

		// fill info struct based on user input
		info.footage = selected_footage.at(i);
		info.codec_type = 0;
		info.size_multiplier = size_combobox->currentData().toDouble();

        QString base_footage_fn = QFileInfo(selected_footage.at(i)->url).baseName();

        // TEMPORARILY hardcoded proxy format
        base_footage_fn.append(".mov");

		// determine path from input
		if (custom_location.isEmpty()) {
			// use same as source (proxy subfolder)

			// generate full path from footage path and proxy_folder_name's translated "Proxy"
			info.path = QDir(QFileInfo(selected_footage.at(i)->url).dir().filePath(proxy_folder_name)).filePath(base_footage_fn);
		} else {
			// use existing location
			info.path = QDir(custom_location).filePath(base_footage_fn);
		}

		// if the proposed proxy file already exists & user didn't select YesToAll box
		if (QFileInfo::exists(info.path) && !yesForAll){
			int rtn = QMessageBox::warning(	this,
								tr("Proxy file exists"),
								tr("The file \"%1\" already exists. Do you wish to replace it?").arg(info.path),
								QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No);
								// return to dialog without closing or starting any proxy generation
			switch (rtn){
				case QMessageBox::Yes:
					break;
				case QMessageBox::YesToAll:
					yesForAll = true;
					break;
				case QMessageBox::No:
					return;
			}
		}
		// send to proxy generator thread
		info_list.append(info);
	}
	// all proxy info checks out, queue it with the proxy generator
	for (int i=0;i<info_list.size();i++) {
		info_list.at(i).footage->proxy = true;
		info_list.at(i).footage->proxy_path.clear();

		proxy_generator.queue(info_list.at(i));
	}

	mainWindow->setWindowModified(true);

	QDialog::accept();
}

void ProxyDialog::location_changed(int i) {
	custom_location.clear();
	if (i == 1) {
		QString s = QFileDialog::getExistingDirectory(this);
		if (s.isEmpty()) {
			location_combobox->setCurrentIndex(0);
		} else {
			location_combobox->setItemText(1, s);
			custom_location = s;
		}
	} else {
		location_combobox->setItemText(1, tr("Custom Location"));
	}
}
