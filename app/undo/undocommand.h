#ifndef UNDOCOMMAND_H
#define UNDOCOMMAND_H

#include <QUndoCommand>

class UndoCommand : public QUndoCommand
{
public:
  UndoCommand(QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

protected:
  virtual void redo_internal();
  virtual void undo_internal();

private:
  bool modified_;

};

#endif // UNDOCOMMAND_H
