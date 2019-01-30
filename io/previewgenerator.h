#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>
#include <QSemaphore>

enum IconType {
	ICON_TYPE_VIDEO,
	ICON_TYPE_AUDIO,
	ICON_TYPE_IMAGE,
	ICON_TYPE_ERROR
};

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
	QString get_thumbnail_path(const QString &hash, const FootageStream &ms);
	QString get_waveform_path(const QString& hash, const FootageStream &ms);
};

#endif // PREVIEWGENERATOR_H
