#ifndef PROJECT_H
#define PROJECT_H

#include <QDockWidget>
#include <QVector>
#include <QTimer>

struct Media;
struct Sequence;
struct Clip;
class Timeline;
class Viewer;
class SourceTable;
class QTreeWidgetItem;
class QXmlStreamWriter;
class QXmlStreamReader;
class QFile;
class ComboAction;

#define LOAD_TYPE_VERSION 69

namespace Ui {
class Project;
}

extern QString autorecovery_filename;
extern QString project_url;
extern QStringList recent_projects;
extern QString recent_proj_file;

int get_type_from_tree(QTreeWidgetItem* item);
void* get_media_from_tree(QTreeWidgetItem* item);
Media* get_footage_from_tree(QTreeWidgetItem* item);
void set_footage_of_tree(QTreeWidgetItem* item, Media* media);
Sequence* get_sequence_from_tree(QTreeWidgetItem* item);
void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence);
void set_item_to_folder(QTreeWidgetItem* item);
void update_footage_tooltip(QTreeWidgetItem* item, Media* media, QString error = 0);

Sequence* create_sequence_from_media(QVector<void*>& media_list, QVector<int>& type_list);

QString get_channel_layout_name(int channels, int layout);
QString get_interlacing_name(int interlacing);

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
    bool is_focused();
	void clear();
    void new_sequence(ComboAction *ca, Sequence* s, bool open, QTreeWidgetItem* parent);
	QString get_next_sequence_name(QString start = 0);
    void delete_media(QTreeWidgetItem* item);
	void process_file_list(bool recursive, QStringList& files, QTreeWidgetItem *parent, QTreeWidgetItem* replace);
	void replace_media(QTreeWidgetItem* item, QString filename);
	QTreeWidgetItem* get_selected_folder();
	bool reveal_media(void* media, QTreeWidgetItem *parent = 0);

    void new_project();
    void load_project();
    void save_project(bool autorecovery);

	QTreeWidgetItem* new_folder(QString name);

    void save_recent_projects();

    QVector<Sequence*> list_all_project_sequences();

	SourceTable* source_table;

	QVector<Media*> last_imported_media;
public slots:
	void import_dialog();
    void delete_selected_media();
    void duplicate_selected();
	void delete_clips_using_selected_media();
	void replace_selected_file();
	void replace_clip_media();
	void open_properties();
private:
	Ui::Project *ui;
    QTreeWidgetItem* new_item();
    bool load_worker(QFile& f, QXmlStreamReader& stream, int type);
    void save_folder(QXmlStreamWriter& stream, QTreeWidgetItem* parent, int type, bool set_ids_only);
    bool show_err;
    QString error_str;
    int folder_id;
    int media_id;
    int sequence_id;
    Sequence* open_seq;
    QVector<QTreeWidgetItem*> loaded_folders;
    QVector<Media*> loaded_media;
	QVector<QTreeWidgetItem*> loaded_media_items;
    QVector<Clip*> loaded_clips;
    QVector<Sequence*> loaded_sequences;
    QTreeWidgetItem* find_loaded_folder_by_id(int id);
    void add_recent_project(QString url);
	void get_all_media_from_table(QList<QTreeWidgetItem*> items, QList<QTreeWidgetItem*>& list, int type);
	void start_preview_generator(QTreeWidgetItem* item, Media* media, bool replacing);
    void list_all_sequences_worker(QVector<Sequence*>* list, QTreeWidgetItem* parent);
	QString get_file_name_from_path(const QString &path);
private slots:
    void rename_media(QTreeWidgetItem* item, int column);
	void clear_recent_projects();
};

class MediaThrobber : public QObject {
    Q_OBJECT
public:
	MediaThrobber(QTreeWidgetItem*);
public slots:
	void stop(int, bool replace);
private slots:
    void animation_update();
private:
    QPixmap pixmap;
    int animation;
	QTreeWidgetItem* item;
    QTimer animator;
};

#endif // PROJECT_H
