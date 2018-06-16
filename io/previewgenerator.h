#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>

struct Media;
struct MediaStream;
struct AVFormatContext;

class PreviewGenerator : public QThread
{
public:
    PreviewGenerator(QObject* parent = 0);
    void run();
    MediaStream* get_stream_from_file_index(int index);
    Media* media;
    AVFormatContext* fmt_ctx;
};

#endif // PREVIEWGENERATOR_H
