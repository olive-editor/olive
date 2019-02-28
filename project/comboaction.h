#ifndef COMBOACTION_H
#define COMBOACTION_H

#include <QUndoCommand>
#include <QVector>

/**
 * @brief The ComboAction class
 *
 * The Undo/Redo system works by stacking an action that knows how to "do" and also "undo" itself. As Olive is
 * a very complex program, there are many actions that can in one "user action". For example, moving a clip over
 * another will delete the clip under it, which is at least two actions that need to be undone if the user clicks
 * undo, however the user only (knowingly) did one thing and would find it confusing if this one user action required
 * to undos to complete undo.
 *
 * To address this, ComboAction is an undo action that simply compiles several possible actions into one, doing them
 * all on every redo, and undoing them all on every undo.
 */
class ComboAction : public QUndoCommand {
public:
    /**
     * @brief ComboAction Constructor. Currently empty.
     */
    ComboAction();

    /**
     * @brief ~ComboAction Destructor. Cleans up all QUndoCommand classes that have been added to it.
     */
    virtual ~ComboAction() override;

    /**
     * @brief Undo Function
     *
     * Called by the QUndoStack to undo.
     *
     * Calls QUndoCommand::undo() on all QUndoCommand objects added by append()
     * in REVERSE order to how they were added. Then calls QUndoCommand::redo() on every action added by appendPost()
     * in the order they were added (not in reverse).
     */
    virtual void undo() override;

    /**
     * @brief Redo Function
     *
     * Called by the QUndoStack to redo.
     *
     * Calls QUndoCommand::redo() on all QUndoCommand objects added by append()
     * in the order they were added. Then calls QUndoCommand::redo() on every action added by appendPost() in
     * the order they were added.
     */
    virtual void redo() override;

    /**
     * @brief Add an undo action
     *
     * Add an action to be done/undone. ComboAction takes ownership of this QUndoCommand and will delete it when
     * it is deleted.
     *
     * @param u
     *
     * The QUndoCommand to add.
     */
    void append(QUndoCommand* u);

    /**
     * @brief Add a post-undo PostAction
     *
     * Sometimes the results of all the actions require another function to be called (e.g. repainting the Viewer).
     * QUndoCommand objects added by appendPost() will run after EVERY QUndoCommand added by append() has been run.
     *
     * @param u
     *
     * The PostAction to add
     */
    void appendPost(QUndoCommand* u);

    /**
     * @brief Returns whether actions have been appended or not
     *
     * @return **TRUE** if actions have been appended, **FALSE** if not.
     */
    bool hasActions();

private:
    /**
     * @brief Internal array of QUndoCommand objects
     */
    QVector<QUndoCommand*> commands;

    /**
     * @brief Internal array of PostAction objects
     */
    QVector<QUndoCommand*> post_commands;
};

#endif // COMBOACTION_H
