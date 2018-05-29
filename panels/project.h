#ifndef PROJECT_H
#define PROJECT_H

#include <QDockWidget>
#include <QVector>

struct Media;
struct Sequence;
class Timeline;
class Viewer;
class SourceTable;

namespace Ui {
class Project;
}

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
	void import_dialog();
	void new_sequence(Sequence* s);
	QString get_next_sequence_name();

	SourceTable* source_table;

private:
	Ui::Project *ui;
};

#endif // PROJECT_H
