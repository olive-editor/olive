#ifndef PROJECT_H
#define PROJECT_H

#include <QDockWidget>
#include <QVector>
#include <QTimer>
#include <QDir>

#include "project/projectmodel.h"

struct Footage;
struct Sequence;
struct Clip;
class Timeline;
class Viewer;
class SourceTable;
class Media;
class QXmlStreamWriter;
class QXmlStreamReader;
class QFile;
class ComboAction;

#define LOAD_TYPE_VERSION 69
#define LOAD_TYPE_URL 70

namespace Ui {
class Project;
}

extern QString autorecovery_filename;
extern QString project_url;
extern QStringList recent_projects;
extern QString recent_proj_file;

extern ProjectModel project_model;

/*int get_type_from_tree(Media *item);
void* get_media_from_tree(QTreeWidgetItem* item);
Footage* get_footage_from_tree(QTreeWidgetItem* item);
void set_footage_of_tree(QTreeWidgetItem* item, Footage* media);
Sequence* get_sequence_from_tree(QTreeWidgetItem* item);
void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence);
void set_item_to_folder(QTreeWidgetItem* item);*/
//void update_footage_tooltip(Media* item, Footage* media, QString error = 0);

Sequence* create_sequence_from_media(QVector<Media *> &media_list);

QString get_channel_layout_name(int channels, uint64_t layout);
QString get_interlacing_name(int interlacing);

class Project : public QDockWidget
{
	Q_OBJECT

public:
	explicit Project(QWidget *parent = 0);
	~Project();
    bool is_focused();
	void clear();
    Media* new_sequence(ComboAction *ca, Sequence* s, bool open, Media* parent);
    QString get_next_sequence_name(QString start = 0);
    void process_file_list(bool recursive, QStringList& files, Media *parent, Media* replace);
    void replace_media(Media* item, QString filename);
    Media* get_selected_folder();
    bool reveal_media(void* media, QModelIndex parent = QModelIndex());
    void add_recent_project(QString url);

    void new_project();
    void load_project(bool autorecovery);
    void save_project(bool autorecovery);

    Media* new_folder(QString name);

    void save_recent_projects();

    QVector<Media*> list_all_project_sequences();

	SourceTable* source_table;

    QVector<Media*> last_imported_media;

    //Media *new_item();

    void start_preview_generator(Media* item, bool replacing);

    Ui::Project *ui;
public slots:
	void import_dialog();
    void delete_selected_media();
    void duplicate_selected();
	void delete_clips_using_selected_media();
	void replace_selected_file();
	void replace_clip_media();
	void open_properties();
private:
    void save_folder(QXmlStreamWriter& stream, int type, bool set_ids_only, const QModelIndex &parent = QModelIndex());
    int folder_id;
    int media_id;
    int sequence_id;
    void get_all_media_from_table(QList<Media *> items, QList<Media *> &list, int type);
    void list_all_sequences_worker(QVector<Media *> *list, Media* parent);
	QString get_file_name_from_path(const QString &path);
    QDir proj_dir;
private slots:
	void clear_recent_projects();
};

class MediaThrobber : public QObject {
    Q_OBJECT
public:
    MediaThrobber(Media*);
public slots:
    void start();
	void stop(int, bool replace);
private slots:
    void animation_update();
private:
    QPixmap pixmap;
    int animation;
    Media* item;
    QTimer* animator;
};

#endif // PROJECT_H
