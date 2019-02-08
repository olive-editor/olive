#include "advancedvideodialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>

#include <QDebug>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/pixdesc.h>
}

AdvancedVideoDialog::AdvancedVideoDialog(QWidget *parent,
                                         int encoding_codec,
                                         VideoCodecParams &iparams) :
    QDialog(parent),
    params(iparams)
{
    setWindowTitle(tr("Advanced Video Settings"));

    // use variable for row to assist adding new fields to this dialog
    int row = 0;

    // get encoder information for this codec
    AVCodec* codec_info = avcodec_find_encoder(static_cast<AVCodecID>(encoding_codec));

    // set up grid layout for dialog
    QGridLayout* layout = new QGridLayout(this);

    // codec pixel formats
    layout->addWidget(new QLabel(tr("Pixel Format:")), row, 0);

    pix_fmt_combo = new QComboBox();

    // load possible pixel formats for this codec into the combobox
    int pix_fmt_index = 0;

    // AVCodec->pix_fmts is terminated by "-1"
    while (codec_info->pix_fmts[pix_fmt_index] != -1) {
        pix_fmt_combo->addItem(av_get_pix_fmt_name(codec_info->pix_fmts[pix_fmt_index]),
                               codec_info->pix_fmts[pix_fmt_index]);

        if (codec_info->pix_fmts[pix_fmt_index] == params.pix_fmt) {
            pix_fmt_combo->setCurrentIndex(pix_fmt_combo->count()-1);
        }

        pix_fmt_index++;
    }

    layout->addWidget(pix_fmt_combo, row, 1);

    row++;

    // buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->setCenterButtons(true);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons, row, 0, 1, 2);
}

void AdvancedVideoDialog::accept() {
    // store settings back into struct

    params.pix_fmt = pix_fmt_combo->currentData().toInt();

    QDialog::accept();
}
