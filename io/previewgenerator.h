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
	PreviewGenerator(QTreeWidgetItem*, Media*, bool);
    void run();
	void cancel();
signals:
	void set_icon(int, bool);
private:
    void parse_media();
    void generate_waveform();
	void finalize_media();
    QTreeWidgetItem* item;
    Media* media;
    AVFormatContext* fmt_ctx;
	bool retrieve_duration;
	bool contains_still_image;
	bool replace;
	bool cancelled;
};

#endif // PREVIEWGENERATOR_H
