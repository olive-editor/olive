#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>
#include <QSemaphore>

#define ICON_TYPE_VIDEO 0
#define ICON_TYPE_AUDIO 1
#define ICON_TYPE_IMAGE 2
#define ICON_TYPE_ERROR 3

struct Footage;
struct FootageStream;
struct AVFormatContext;
class Media;

class PreviewGenerator : public QThread
{
    Q_OBJECT
public:
	PreviewGenerator(Media*, Footage*, bool);
    void run();
	void cancel();
signals:
	void set_icon(int, bool);
private:
    void parse_media();
	bool retrieve_preview(const QString &hash);
    void generate_waveform();
	void finalize_media();
    AVFormatContext* fmt_ctx;
    Media* media;
    Footage* footage;
	bool retrieve_duration;
	bool contains_still_image;
	bool replace;
	bool cancelled;
	QString data_path;
	QString get_thumbnail_path(const QString &hash, FootageStream* ms);
	QString get_waveform_path(const QString& hash, FootageStream* ms);
};

#endif // PREVIEWGENERATOR_H
