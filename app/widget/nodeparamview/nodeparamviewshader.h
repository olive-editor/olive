/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef NODEPARAMVIEWSHADER_H
#define NODEPARAMVIEWSHADER_H

#include <QObject>
#include <QString>
#include <QTextEdit>

class QPlainTextEdit;

namespace olive {

class ExternalEditorProxy;
class Node;


// This is a facility for text-type param view that is used
// as shader code entry (fragment or vertex).
// Instead of showing the full code, a summary is shown
class NodeParamViewShader : public QObject
{
  Q_OBJECT
public:
  NodeParamViewShader( const QPlainTextEdit & content, QObject * parent=nullptr);

  QWidget* widget() {
    return summary_;
  }

  // bind this viewer to a Node that handles one GLSL script,
  // so changes in the script will update this widget
  void attachOwnerNode( const Node * owner);

  // start editing shader in internal or external editor
  void launchCodeEditor(QString & text);

signals:
  void OnTextChangedExternally( const QString & text);

private slots:
  // invoked when the owner Node has parsed a shader script
  void onMetadataChanged( const QString & name, const QString & description, const QString & version);

private:
  QString displayedText() const;

private:
  const QPlainTextEdit & full_shader_;
  QTextEdit* summary_;
  ExternalEditorProxy* ext_editor_proxy_;

  QString name_;
  QString description_;
  QString version_;
};

}

#endif // NODEPARAMVIEWSHADER_H
