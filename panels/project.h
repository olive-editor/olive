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

#define MEDIA_TYPE_FOOTAGE 0
#define MEDIA_TYPE_SEQUENCE 1
#define MEDIA_TYPE_FOLDER 2
#define MEDIA_TYPE_SOLID 3

namespace Ui {
class Project;
}

extern bool project_changed;
extern QString project_url;

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
    bool is_focused();
    void clear();
    Media* import_file(QString url);
    void import_dialog();
    void new_sequence(Sequence* s, bool open);
	QString get_next_sequence_name();
    void delete_media(QTreeWidgetItem* item);
    void delete_selected_media();
    void process_file_list(QStringList& files);
    void duplicate_selected();

    void new_project();
    void load_project();
    void save_project();

    void new_folder();

    int get_type_from_tree(QTreeWidgetItem* item);
    Media* get_media_from_tree(QTreeWidgetItem* item);
    void set_media_of_tree(QTreeWidgetItem* item, Media* media);
    Sequence* get_sequence_from_tree(QTreeWidgetItem* item);
    void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence);
    void set_item_to_folder(QTreeWidgetItem* item);

	SourceTable* source_table;
private:
	Ui::Project *ui;
    QTreeWidgetItem* new_item();
private slots:
    void rename_media(QTreeWidgetItem* item, int column);
};

#endif // PROJECT_H
