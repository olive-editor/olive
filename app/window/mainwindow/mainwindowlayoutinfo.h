#ifndef MAINWINDOWLAYOUTINFO_H
#define MAINWINDOWLAYOUTINFO_H

#include "node/project/folder/folder.h"
#include "node/project/sequence/sequence.h"
#include "panel/panel.h"

namespace olive {

class MainWindowLayoutInfo
{
public:
  MainWindowLayoutInfo() = default;

  void toXml(QXmlStreamWriter* writer) const;

  static MainWindowLayoutInfo fromXml(QXmlStreamReader* reader, const QHash<quintptr, Node*> &node_map);

  void add_folder(Folder* f);

  void add_sequence(Sequence *seq);

  void add_viewer(ViewerOutput *viewer);

  void set_panel_data(const QString &id, const PanelWidget::Info &data);

  void set_state(const QByteArray& layout);

  const std::vector<Folder*>& open_folders() const
  {
    return open_folders_;
  }

  const std::vector<Sequence*>& open_sequences() const
  {
    return open_sequences_;
  }

  const std::vector<ViewerOutput*>& open_viewers() const
  {
    return open_viewers_;
  }

  const std::map<QString, PanelWidget::Info> &panel_data() const
  {
    return panel_data_;
  }

  const QByteArray& state() const
  {
    return state_;
  }

private:
  QByteArray state_;

  std::vector<Folder*> open_folders_;

  std::vector<Sequence*> open_sequences_;

  std::vector<ViewerOutput*> open_viewers_;

  std::map<QString, PanelWidget::Info> panel_data_;

  static const unsigned int kVersion = 1;

};

}

Q_DECLARE_METATYPE(olive::MainWindowLayoutInfo)

#endif // MAINWINDOWLAYOUTINFO_H
