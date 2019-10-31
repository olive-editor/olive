#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>

#include "colorprocessor.h"
#include "decoder/frame.h"

class ColorManager : public QObject
{
  Q_OBJECT
public:
  void SetConfig(const QString& filename);

  void SetConfig(OCIO::ConstConfigRcPtr config);

  static void CreateInstance();

  static ColorManager* instance();

  static void DestroyInstance();

  static void DisassociateAlpha(FramePtr f);

  static void AssociateAlpha(FramePtr f);

  static void ReassociateAlpha(FramePtr f);

  static QStringList ListAvailableDisplays();

  static QString GetDefaultDisplay();

  static QStringList ListAvailableViews(QString display);

  static QString GetDefaultView(const QString& display);

  static QStringList ListAvailableLooks();

  static QStringList ListAvailableInputColorspaces(OCIO::ConstConfigRcPtr config);

signals:
  void ConfigChanged();

private:
  ColorManager();

  static ColorManager* instance_;

  enum AlphaAction {
    kAssociate,
    kDisassociate,
    kReassociate
  };

  static void AssociateAlphaPixFmtFilter(AlphaAction action, FramePtr f);

  template<typename T>
  static void AssociateAlphaInternal(AlphaAction action, T* data, int pix_count);
};

#endif // COLORSERVICE_H
