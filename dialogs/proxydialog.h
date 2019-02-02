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
public slots:
	// called if user clicks "OK" on the dialog
	virtual void accept() override;
private:
	// user's desired dimensions
	QComboBox* size_combobox;

	// user's desired proxy format
	QComboBox* format_combobox;

	// allows users to set the location to store proxies
	QComboBox* location_combobox;

	// stores the custom location to store proxies if the user sets a custom location
	QString custom_location;

	// stores the subdirectory to be made next to the source (dependent on the user's language)
	QString proxy_folder_name;

	// list of footage to make proxies for
	QVector<Footage*> selected_footage;
private slots:
	// triggered when the user changes the index in the location combobox
	void location_changed(int i);
};

#endif // PROXYDIALOG_H
