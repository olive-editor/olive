#include "colormanager.h"

#include <QFloat16>

#include "common/define.h"
#include "config/config.h"

ColorManager::ColorManager()
{
  // Ensures config is set to something
  config_ = OCIO::GetCurrentConfig();
}

OCIO::ConstConfigRcPtr ColorManager::GetConfig() const
{
  return config_;
}

void ColorManager::SetConfig(const QString &filename)
{
  SetConfig(OCIO::Config::CreateFromFile(filename.toUtf8()));
}

void ColorManager::SetConfig(OCIO::ConstConfigRcPtr config)
{
  config_ = config;

  emit ConfigChanged();
}

void ColorManager::DisassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kDisassociate, f);
}

void ColorManager::AssociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kAssociate, f);
}

void ColorManager::ReassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kReassociate, f);
}

QStringList ColorManager::ListAvailableDisplays()
{
  QStringList displays;

  int number_of_displays = config_->getNumDisplays();

  for (int i=0;i<number_of_displays;i++) {
    displays.append(config_->getDisplay(i));
  }

  return displays;
}

QString ColorManager::GetDefaultDisplay()
{
  return config_->getDefaultDisplay();
}

QStringList ColorManager::ListAvailableViews(QString display)
{
  QStringList views;

  int number_of_views = config_->getNumViews(display.toUtf8());

  for (int i=0;i<number_of_views;i++) {
    views.append(config_->getView(display.toUtf8(), i));
  }

  return views;
}

QString ColorManager::GetDefaultView(const QString &display)
{
  return config_->getDefaultView(display.toUtf8());
}

QStringList ColorManager::ListAvailableLooks()
{
  QStringList looks;

  int number_of_looks = config_->getNumLooks();

  for (int i=0;i<number_of_looks;i++) {
    looks.append(config_->getLookNameByIndex(i));
  }

  return looks;
}

QStringList ColorManager::ListAvailableInputColorspaces()
{
  return ListAvailableInputColorspaces(config_);
}

QStringList ColorManager::ListAvailableInputColorspaces(OCIO::ConstConfigRcPtr config)
{
  QStringList spaces;

  int number_of_colorspaces = config->getNumColorSpaces();

  for (int i=0;i<number_of_colorspaces;i++) {
    spaces.append(config->getColorSpaceNameByIndex(i));
  }

  return spaces;
}

void ColorManager::AssociateAlphaPixFmtFilter(ColorManager::AlphaAction action, FramePtr f)
{
  int pixel_count = f->width() * f->height() * kRGBAChannels;

  switch (static_cast<PixelFormat::Format>(f->format())) {
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    qWarning() << "Alpha association functions received an invalid pixel format";
    break;
  case PixelFormat::PIX_FMT_RGBA8:
  case PixelFormat::PIX_FMT_RGBA16U:
    qWarning() << "Alpha association functions only works on float-based pixel formats at this time";
    break;
  case PixelFormat::PIX_FMT_RGBA16F:
  {
    AssociateAlphaInternal<qfloat16>(action, reinterpret_cast<qfloat16*>(f->data()), pixel_count);
    break;
  }
  case PixelFormat::PIX_FMT_RGBA32F:
  {
    AssociateAlphaInternal<float>(action, reinterpret_cast<float*>(f->data()), pixel_count);
    break;
  }
  }
}

template<typename T>
void ColorManager::AssociateAlphaInternal(ColorManager::AlphaAction action, T *data, int pix_count)
{
  for (int i=0;i<pix_count;i+=kRGBAChannels) {
    T alpha = data[i+kRGBChannels];

    if (action == kAssociate || alpha > 0) {
      for (int j=0;j<kRGBChannels;j++) {
        if (action == kDisassociate) {
          data[i+j] /= alpha;
        } else {
          data[i+j] *= alpha;
        }
      }
    }
  }
}
