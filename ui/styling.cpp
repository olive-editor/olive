#include "styling.h"

#include "global/config.h"

bool olive::styling::UseDarkIcons()
{
  return olive::CurrentConfig.style == kOliveDefaultLight || olive::CurrentConfig.style == kNativeDarkIcons;
}

QColor olive::styling::GetIconColor()
{
  if (UseDarkIcons()) {
    return Qt::black;
  } else {
    return Qt::white;
  }
}



bool olive::styling::UseNativeUI()
{
  return olive::CurrentConfig.style == kNativeLightIcons || olive::CurrentConfig.style == kNativeDarkIcons;
}
