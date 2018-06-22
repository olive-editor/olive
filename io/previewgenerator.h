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
    Media* media;
    AVFormatContext* fmt_ctx;
};

#endif // PREVIEWGENERATOR_H
