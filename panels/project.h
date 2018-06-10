#ifndef PROJECT_H
#define PROJECT_H

#include <QDockWidget>
#include <QVector>

struct Media;
struct Sequence;
class Timeline;
class Viewer;
class SourceTable;
class QTreeWidgetItem;

namespace Ui {
class Project;
}

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
    void clear();
	void import_dialog();
    void set_sequence(Sequence* s);
	void new_sequence(Sequence* s);
	QString get_next_sequence_name();
    void remove_item(int i);

    void load_project();
    void save_project();

    Media* get_media_from_tree(QTreeWidgetItem* item);
    void set_media_of_tree(QTreeWidgetItem* item, Media* media);

	SourceTable* source_table;

private:
	Ui::Project *ui;
};

#endif // PROJECT_H
