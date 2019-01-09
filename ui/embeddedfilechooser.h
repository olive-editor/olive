#ifndef EMBEDDEDFILECHOOSER_H
#define EMBEDDEDFILECHOOSER_H

#include <QWidget>

class QLabel;

class EmbeddedFileChooser : public QWidget {
	Q_OBJECT
public:
	EmbeddedFileChooser(QWidget* parent = 0);

	const QString& getFilename();
	const QString& getPreviousValue();
	void setFilename(const QString& s);
signals:
	void changed();
private:
	QLabel* file_label;
	QString filename;
	QString old_filename;
	void update_label();
private slots:
	void browse();
};

#endif // EMBEDDEDFILECHOOSER_H
