#ifndef MAINWINDOWLAYOUTINFO_H
#define MAINWINDOWLAYOUTINFO_H

#include "project/item/folder/folder.h"
#include "project/item/sequence/sequence.h"

OLIVE_NAMESPACE_ENTER

class MainWindowLayoutInfo
{
public:
  MainWindowLayoutInfo() = default;

  void toXml(QXmlStreamWriter* writer) const;

  static MainWindowLayoutInfo fromXml(QXmlStreamReader* reader, XMLNodeData &xml_data);

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

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::MainWindowLayoutInfo)

#endif // MAINWINDOWLAYOUTINFO_H
