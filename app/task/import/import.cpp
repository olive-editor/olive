#include "import.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>

// FIXME: Only used for test code
#include "panel/panelfocusmanager.h"
#include "panel/project/project.h"
// End test code

#include "project/item/footage/footage.h"
#include "task/probe/probe.h"
#include "task/taskmanager.h"

ImportTask::ImportTask(Folder *parent, const QStringList &urls) :
  urls_(urls),
  parent_(parent)
{
  set_text(tr("Importing %1 files").arg(urls.size()));
}

bool ImportTask::Action()
{
  for (int i=0;i<urls_.size();i++) {

    const QString& url = urls_.at(i);

    Footage* f = new Footage();

    QFileInfo file_info(url);

    // FIXME: Is it possible for a file to go missing between the Import dialog and here?
    //        And what is the behavior/result of that?

    f->set_filename(url);
    f->set_name(file_info.fileName());
    f->set_timestamp(file_info.lastModified());

    parent_->add_child(f);

    // Create ProbeTask to analyze this media
    ProbeTask* pt = new ProbeTask(f);

    // The task won't work unless it's in the main thread and we're definitely not
    // FIXME: Should Tasks check what thread they're in and move themselves to the main thread?
    pt->moveToThread(qApp->thread());

    // Queue task in task manager
    olive::task_manager.AddTask(pt);

    emit ProgressChanged(i * 100 / urls_.size());

  }

  // FIXME: No good very bad test code to update the ProjectExplorer model/view
  //        A better solution would be for the Project itself to signal that a new row was being added
  ProjectPanel* p = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  p->set_project(p->project());
  // End test code

  return true;
}
