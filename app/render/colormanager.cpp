#include "colormanager.h"

#include <QFloat16>

#include "common/define.h"
#include "config/config.h"

ColorManager* ColorManager::instance_ = nullptr;

void ColorManager::SetConfig(const QString &filename)
{
  SetConfig(OCIO::Config::CreateFromFile(filename.toUtf8()));
}

void ColorManager::SetConfig(OpenColorIO::v1::ConstConfigRcPtr config)
{
  OCIO::SetCurrentConfig(config);

  emit ConfigChanged();
}

void ColorManager::CreateInstance()
{
  if (instance_ == nullptr) {
    instance_ = new ColorManager();
  }
}

ColorManager *ColorManager::instance()
{
  return instance_;
}

void ColorManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
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

  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  int number_of_displays = config->getNumDisplays();

  for (int i=0;i<number_of_displays;i++) {
    displays.append(config->getDisplay(i));
  }

  return displays;
}

QString ColorManager::GetDefaultDisplay()
{
  return OCIO::GetCurrentConfig()->getDefaultDisplay();
}

QStringList ColorManager::ListAvailableViews(QString display)
{
  QStringList views;

  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  int number_of_views = config->getNumViews(display.toUtf8());

  for (int i=0;i<number_of_views;i++) {
    views.append(config->getView(display.toUtf8(), i));
  }

  return views;
}

QString ColorManager::GetDefaultView(const QString &display)
{
  return OCIO::GetCurrentConfig()->getDefaultView(display.toUtf8());
}

QStringList ColorManager::ListAvailableLooks()
{
  QStringList looks;

  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  int number_of_looks = config->getNumLooks();

  for (int i=0;i<number_of_looks;i++) {
    looks.append(config->getLookNameByIndex(i));
  }

  return looks;
}

QStringList ColorManager::ListAvailableInputColorspaces(OpenColorIO::v1::ConstConfigRcPtr config)
{
  QStringList spaces;

  int number_of_colorspaces = config->getNumColorSpaces();

  for (int i=0;i<number_of_colorspaces;i++) {
    spaces.append(config->getColorSpaceNameByIndex(i));
  }

  return spaces;
}

ColorManager::ColorManager()
{
}

void ColorManager::AssociateAlphaPixFmtFilter(ColorManager::AlphaAction action, FramePtr f)
{
  int pixel_count = f->width() * f->height() * kRGBAChannels;

  switch (static_cast<olive::PixelFormat>(f->format())) {
  case olive::PIX_FMT_INVALID:
  case olive::PIX_FMT_COUNT:
    qWarning() << "Alpha association functions received an invalid pixel format";
    break;
  case olive::PIX_FMT_RGBA8:
  case olive::PIX_FMT_RGBA16U:
    qWarning() << "Alpha association functions only works on float-based pixel formats at this time";
    break;
  case olive::PIX_FMT_RGBA16F:
  {
    AssociateAlphaInternal<qfloat16>(action, reinterpret_cast<qfloat16*>(f->data()), pixel_count);
    break;
  }
  case olive::PIX_FMT_RGBA32F:
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
