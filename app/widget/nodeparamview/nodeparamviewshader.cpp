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

#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QPlainTextEdit>

#include "nodeparamviewshader.h"
#include "node/filter/shader/shader.h"
#include "node/block/transition/shader/shadertransition.h"
#include "dialog/codeeditor/externaleditorproxy.h"
#include "dialog/codeeditor/codeeditordialog.h"
#include "config/config.h"


namespace olive {

NodeParamViewShader::NodeParamViewShader(const QPlainTextEdit &content,
                                         QObject *parent) :
  QObject(parent),
  full_shader_(content)
{
  summary_ = new QTextEdit();
  summary_->setReadOnly( true);
  summary_->setVisible( false);

  ext_editor_proxy_ = new ExternalEditorProxy( this);

  connect( ext_editor_proxy_, & ExternalEditorProxy::textChanged,
          this, & NodeParamViewShader::OnTextChangedExternally);
}


void NodeParamViewShader::attachOwnerNode(const Node *owner)
{
  summary_->setVisible( true);

  // create a file whose name is unique for for the node this instance belongs to.
  // 'node_id' is based on address of the node this param belongs to
  // 'QStandardPaths::TempLocation' is guaranteed not to be empty
  QString file_path = QStandardPaths::standardLocations( QStandardPaths::TempLocation).at(0);
  file_path += QString("/%2.glsl").arg((uint64_t)owner);

  ext_editor_proxy_->SetFilePath(file_path);

  /* list of nodes that use this view: */

  const ShaderFilterNode * owner_filter = dynamic_cast<const ShaderFilterNode *>(owner);
  if (owner_filter != nullptr) {
    owner_filter->getMetadata( name_, description_, version_);
    // initialize summary text ...
    onMetadataChanged( name_, description_, version_);

    // ... and track changes
    connect( owner_filter, & ShaderFilterNode::metadataChanged,
             this, & NodeParamViewShader::onMetadataChanged);
  }

  const ShaderTransition * owner_transition = dynamic_cast<const ShaderTransition *>(owner);
  if (owner_transition != nullptr) {
    owner_transition->getMetadata( name_, description_, version_);
    // initialize summary text ...
    onMetadataChanged( name_, description_, version_);

    // ... and track changes
    connect( owner_transition, & ShaderTransition::metadataChanged,
            this, & NodeParamViewShader::onMetadataChanged);
  }

}

void NodeParamViewShader::onMetadataChanged(const QString &name,
                                            const QString &description,
                                            const QString &version)
{
  name_ = name;
  description_ = description;
  version_ = version;

  summary_->setHtml( displayedText());

}

QString NodeParamViewShader::displayedText() const
{
  QString version = version_.isEmpty() ? QString() : QString("<i>(%1)</i>").arg(version_);

  // We don't display the full shader, but only some metadata
  return QString("<font color=\"#808020\" size=\"+1\">%1</font>"
                 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;%2</span>"
                 "<p><font color=\"#E0E0D0\">%3</font></p>").
      arg( name_, version, description_);
}

void NodeParamViewShader::launchCodeEditor(QString & text)
{
  if (Config::Current()["EditorUseInternal"].toBool()) {

    // internal editor
    CodeEditorDialog d(full_shader_.toPlainText(), nullptr);

    // in case external editor was previously opened but
    // then user switched to internal editor.
    ext_editor_proxy_->Detach();

    if (d.exec() == QDialog::Accepted) {
      text = d.text();
    }
  }
  else {

    // external editor
    ext_editor_proxy_->Launch( full_shader_.toPlainText());
  }
}

}


