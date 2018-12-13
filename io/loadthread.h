#ifndef LOADTHREAD_H
#define LOADTHREAD_H

#include <QThread>
#include <QDir>
#include <QXmlStreamReader>

class Media;
class Footage;
class Clip;
class Sequence;
class LoadDialog;

class LoadThread : public QThread
{
    Q_OBJECT
public:
    LoadThread(LoadDialog* l, bool a);
    void run();
signals:
    void success();
private slots:
    void success_func();
private:
    LoadDialog* ld;
    bool autorecovery;

    bool load_worker(QFile& f, QXmlStreamReader& stream, int type);
    Sequence* open_seq;
    QVector<Media*> loaded_media_items;
    QDir proj_dir;
    QDir internal_proj_dir;
    QString internal_proj_url;
    bool show_err;
    QString error_str;

    QVector<Media*> loaded_folders;
    QVector<Clip*> loaded_clips;
    QVector<Media*> loaded_sequences;
    Media* find_loaded_folder_by_id(int id);
};

#endif // LOADTHREAD_H
