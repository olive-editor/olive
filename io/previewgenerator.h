#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>

class PreviewGenerator : public QThread
{
public:
    PreviewGenerator(QWidget* parent = 0);
    void run() override;
};

#endif // PREVIEWGENERATOR_H
