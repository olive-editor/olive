#ifndef PROJECT_H
#define PROJECT_H

#include <QDockWidget>
#include <QVector>
#include <QTimer>

struct Media;
struct Sequence;
class Timeline;
class Viewer;
class SourceTable;
class QTreeWidgetItem;
class QXmlStreamWriter;
class QXmlStreamReader;
class QFile;

#define SAVE_VERSION "180719"

#define MEDIA_TYPE_FOOTAGE 0
#define MEDIA_TYPE_SEQUENCE 1
#define MEDIA_TYPE_FOLDER 2
#define MEDIA_TYPE_SOLID 3

#define LOAD_TYPE_VERSION 69
#define SAVE_SET_FOLDER_IDS 70

class TimelineAction;

namespace Ui {
class Project;
}

extern bool project_changed;
extern QString project_url;
extern QStringList recent_projects;
extern QString recent_proj_file;

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
    bool is_focused();
    void clear();
    QTreeWidgetItem* import_file(QString url);
    void import_dialog();
    void new_sequence(TimelineAction* ta, Sequence* s, bool open, QTreeWidgetItem* parent);
	QString get_next_sequence_name();
    void delete_media(QTreeWidgetItem* item);
    void delete_selected_media();
    void process_file_list(QStringList& files);
    void duplicate_selected();

    void new_project();
    void load_project();
    void save_project();

    QTreeWidgetItem* new_folder();

    int get_type_from_tree(QTreeWidgetItem* item);
    Media* get_media_from_tree(QTreeWidgetItem* item);
    void set_media_of_tree(QTreeWidgetItem* item, Media* media);
    Sequence* get_sequence_from_tree(QTreeWidgetItem* item);
    void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence);
    void set_item_to_folder(QTreeWidgetItem* item);
    void save_recent_projects();

	SourceTable* source_table;
private:
	Ui::Project *ui;
    QTreeWidgetItem* new_item();
    bool load_worker(QFile& f, QXmlStreamReader& stream, int type);
    void save_folder(QXmlStreamWriter& stream, QTreeWidgetItem* parent, int type);
    QString error_str;
    int folder_id;
    int media_id;
    QVector<QTreeWidgetItem*> loaded_folders;
    QVector<Media*> loaded_media;
    QTreeWidgetItem* find_loaded_folder_by_id(int id);
    void add_recent_project(QString url);
    void get_media_from_table(QList<QTreeWidgetItem*> items, QList<QTreeWidgetItem*>& list, int type);
private slots:
    void rename_media(QTreeWidgetItem* item, int column);
    void clear_recent_projects();
};

class MediaThrobber : public QObject {
    Q_OBJECT
public:
    MediaThrobber(QTreeWidgetItem*);
public slots:
    void stop(int);
private slots:
    void animation_update();
private:
    QPixmap pixmap;
    int animation;
    QTreeWidgetItem* item;
    QTimer animator;
};

#endif // PROJECT_H
