#ifndef PROXYDIALOG_H
#define PROXYDIALOG_H

#include <QDialog>
#include <QVector>
#include <QComboBox>

struct Footage;

class ProxyDialog : public QDialog {
	Q_OBJECT
public:
	ProxyDialog(QWidget* parent, const QVector<Footage*>& footage);
private:
	// user's dimensions
	QComboBox* size_combobox;

	// allows users to set the location to store proxies
	QComboBox* location_combobox;

	// stores the custom location to store proxies if the user sets a custom location
	QString custom_location;

	// stores the subdirectory to be made next to the source in the user's language
	QString proxy_folder_name;
private slots:
	void location_changed(int i);
};

#endif // PROXYDIALOG_H
