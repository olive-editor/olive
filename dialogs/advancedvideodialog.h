#ifndef ADVANCEDVIDEODIALOG_H
#define ADVANCEDVIDEODIALOG_H

#include <QDialog>

#include "io/exportthread.h"

class QComboBox;

class AdvancedVideoDialog : public QDialog {
    Q_OBJECT
public:
    AdvancedVideoDialog(QWidget* parent,
                        int encoding_codec,
                        VideoCodecParams& iparams);

public slots:
    virtual void accept() override;
private:
    VideoCodecParams& params;

    QComboBox* pix_fmt_combo;
};

#endif // ADVANCEDVIDEODIALOG_H
