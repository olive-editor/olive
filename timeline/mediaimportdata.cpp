#include "mediaimportdata.h"

olive::timeline::MediaImportData::MediaImportData(Media *media, olive::timeline::MediaImportType import_type) :
  media_(media),
  import_type_(import_type)
{
}

Media *olive::timeline::MediaImportData::media() const
{
  return media_;
}

olive::timeline::MediaImportType olive::timeline::MediaImportData::type() const
{
  return import_type_;
}
