#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

#include "exportaudiotab.h"
#include "exportcodec.h"
#include "exportformat.h"
#include "exportvideotab.h"
#include "widget/viewer/viewer.h"

class ExportDialog : public QDialog
{
  Q_OBJECT
public:
  ExportDialog(QWidget* parent = nullptr);

private:
  void SetUpFormats();
  void LoadPresets();
  void SetDefaultFilename();

  QList<ExportFormat> formats_;
  int previously_selected_format_;

  QList<ExportCodec> codecs_;

  ViewerWidget* preview_viewer_;
  QLineEdit* filename_edit_;
  QComboBox* format_combobox_;

  ExportVideoTab* video_tab_;
  ExportAudioTab* audio_tab_;

  enum Format {
    kFormatDNxHD,
    kFormatMatroska,
    kFormatMPEG4,
    kFormatOpenEXR,
    kFormatQuickTime,
    kFormatPNG,
    kFormatTIFF,

    kFormatCount
  };

  enum Codec {
    kCodecDNxHD,
    kCodecH264,
    kCodecH265,
    kCodecOpenEXR,
    kCodecPNG,
    kCodecProRes,
    kCodecTIFF,

    kCodecMP2,
    kCodecMP3,
    kCodecAAC,
    kCodecPCMS16LE,

    kCodecCount
  };

private slots:
  void BrowseFilename();

  void FormatChanged(int index);

};

#endif // EXPORTDIALOG_H
