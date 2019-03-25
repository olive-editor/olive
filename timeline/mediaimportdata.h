#ifndef MEDIAIMPORTDATA_H
#define MEDIAIMPORTDATA_H

#include "project/media.h"

namespace olive {
namespace timeline {

enum MediaImportType {
  kImportVideoOnly,
  kImportAudioOnly,
  kImportBoth
};

class MediaImportData {
public:
  MediaImportData(Media* media = nullptr, MediaImportType import_type = kImportBoth);
  Media* media() const;
  MediaImportType type() const;
private:
  Media* media_;
  MediaImportType import_type_;
};

}
}


#endif // MEDIAIMPORTDATA_H
