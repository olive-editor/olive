#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>

#define ICON_TYPE_VIDEO 0
#define ICON_TYPE_AUDIO 1
#define ICON_TYPE_ERROR 2

struct Media;
struct MediaStream;
struct AVFormatContext;
class QTreeWidgetItem;

class PreviewGenerator : public QThread
{
    Q_OBJECT
public:
    PreviewGenerator(QTreeWidgetItem*, Media*);
    void run();
signals:
    void set_icon(int);
private:
    void parse_media();
    void generate_waveform();
    QTreeWidgetItem* item;
    Media* media;
    AVFormatContext* fmt_ctx;
};

#endif // PREVIEWGENERATOR_H
