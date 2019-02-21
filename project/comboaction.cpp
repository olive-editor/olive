#include "comboaction.h"

ComboAction::ComboAction() {}

ComboAction::~ComboAction() {
    for (int i=0;i<commands.size();i++) {
        delete commands.at(i);
    }
}

void ComboAction::undo() {
    for (int i=commands.size()-1;i>=0;i--) {
        commands.at(i)->undo();
    }
    for (int i=0;i<post_commands.size();i++) {
        post_commands.at(i)->undo();
    }
}

void ComboAction::redo() {
    for (int i=0;i<commands.size();i++) {
        commands.at(i)->redo();
    }
    for (int i=0;i<post_commands.size();i++) {
        post_commands.at(i)->redo();
    }
}

void ComboAction::append(QUndoCommand* u) {
    commands.append(u);
}

void ComboAction::appendPost(QUndoCommand* u) {
    post_commands.append(u);
}
