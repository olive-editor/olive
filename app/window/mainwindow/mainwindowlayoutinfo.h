#ifndef MAINWINDOWLAYOUTINFO_H
#define MAINWINDOWLAYOUTINFO_H

#include "node/project/folder/folder.h"
#include "node/project/sequence/sequence.h"

namespace olive {

class MainWindowLayoutInfo
{
public:
  MainWindowLayoutInfo() = default;

  void toXml(QXmlStreamWriter* writer) const;

  static MainWindowLayoutInfo fromXml(QXmlStreamReader* reader, const QHash<quintptr, Node*> &node_map);

  void add_folder(Folder* f);

  struct OpenSequence {
    Sequence* sequence;
    QByteArray panel_state;
  };

  void add_sequence(const OpenSequence& seq);

  void set_state(const QByteArray& layout);

  const QList<Folder*>& open_folders() const
  {
    return open_folders_;
  }

  const QList<OpenSequence>& open_sequences() const
  {
    return open_sequences_;
  }

  const QByteArray& state() const
  {
    return state_;
  }

private:
  QByteArray state_;

  QList<Folder*> open_folders_;

  QList<OpenSequence> open_sequences_;

};

}

Q_DECLARE_METATYPE(olive::MainWindowLayoutInfo)

#endif // MAINWINDOWLAYOUTINFO_H
