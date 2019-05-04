#ifndef PROJECTFUNCTIONS_H
#define PROJECTFUNCTIONS_H

#include "project/media.h"
#include "timeline/sequence.h"
#include "timeline/mediaimportdata.h"

namespace olive {
namespace project {

MediaPtr CreateFolder(QString name);

SequencePtr CreateSequenceFromMedia(QVector<olive::timeline::MediaImportData> &media_list);

}
}

#endif // PROJECTFUNCTIONS_H
