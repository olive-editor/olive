#include "probe.h"

#include <QFileInfo>

#include "decoder/probeserver.h"

ProbeTask::ProbeTask(Footage *footage) :
  footage_(footage)
{
  QString base_filename = QFileInfo(footage_->filename()).fileName();

  set_text(tr("Probing \"%1\"").arg(base_filename));
}

bool ProbeTask::Action()
{
  olive::ProbeMedia(footage_);

  return true;
}
