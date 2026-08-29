/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "OlivePluginInstance.h"

#include "OliveClip.h"
#include "ofxGPURender.h"
#include "ofxCore.h"
#include "ofxMessage.h"
#include "common/Current.h"
#include "core.h"
#include "dialog/progress/progress.h"
#include "node/output/viewer/viewer.h"
#include "panel/panelmanager.h"
#include "panel/timebased/timebased.h"
#include "panel/timeline/timeline.h"

#include <cstdio>
#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QThread>
#include <QtGlobal>
#include <string.h>
#include <QString>
#include "paraminstance.h"
namespace olive
{
namespace plugin
{
namespace
{
const std::string kImageFieldNoneStr(kOfxImageFieldNone);
const std::string kImageFieldUpperStr(kOfxImageFieldUpper);
const std::string kImageFieldLowerStr(kOfxImageFieldLower);

QString FormatOfxMessage(const char *format, va_list args)
{
	char buffer[1024];
	va_list args_copy;
	va_copy(args_copy, args);
	const int needed = vsnprintf(buffer, sizeof(buffer), format, args_copy);
	va_end(args_copy);
	if (needed < 0) {
		return QString();
	}
	if (needed < static_cast<int>(sizeof(buffer))) {
		return QString::fromUtf8(buffer);
	}
	QByteArray dynamic_buffer(needed + 1, 0);
	const int written =
		vsnprintf(dynamic_buffer.data(), dynamic_buffer.size(), format, args);
	if (written < 0) {
		return QString();
	}
	return QString::fromUtf8(dynamic_buffer.constData());
}

const std::string &FieldOrderForParams(const VideoParams &params)
{
	switch (params.interlacing()) {
	case VideoParams::kInterlaceNone:
		return kImageFieldNoneStr;
	case VideoParams::kInterlacedTopFirst:
		return kImageFieldUpperStr;
	case VideoParams::kInterlacedBottomFirst:
		return kImageFieldLowerStr;
	}
	return kImageFieldNoneStr;
}

class DeferredRedoCommand : public UndoCommand {
public:
	explicit DeferredRedoCommand(UndoCommand *inner)
		: inner_(inner)
	{
	}

	~DeferredRedoCommand() override
	{
		delete inner_;
	}

	Project *GetRelevantProject() const override
	{
		return inner_ ? inner_->GetRelevantProject() : nullptr;
	}

protected:
	void redo() override
	{
		if (skip_first_redo_) {
			skip_first_redo_ = false;
			return;
		}
		if (inner_) {
			inner_->redo_now();
		}
	}

	void undo() override
	{
		if (inner_) {
			inner_->undo_now();
		}
	}

private:
	UndoCommand *inner_ = nullptr;
	bool skip_first_redo_ = true;
};

ViewerOutput *GetActiveViewerOutput()
{
	PanelManager *manager = PanelManager::instance();
	if (!manager) {
		return nullptr;
	}

	if (auto *time_panel = manager->MostRecentlyFocused<TimeBasedPanel>()) {
		if (time_panel->GetConnectedViewer()) {
			return time_panel->GetConnectedViewer();
		}
	}

	QList<TimelinePanel *> timelines =
		manager->GetPanelsOfType<TimelinePanel>();
	for (TimelinePanel *panel : timelines) {
		if (panel && panel->GetConnectedViewer()) {
			return panel->GetConnectedViewer();
		}
	}

	return nullptr;
}
} // namespace

const std::string &OlivePluginInstance::getDefaultOutputFielding() const
{
	return FieldOrderForParams(params_);
}

void OlivePluginInstance::setNode(std::shared_ptr<PluginNode> node)
{
	node_ = node;
	for (const auto &entry : getParams()) {
		if (!entry.second) {
			continue;
		}
		if (auto *bound = dynamic_cast<NodeBoundParam *>(entry.second)) {
			bound->SetNode(node_);
		}
	}
}

OfxStatus OlivePluginInstance::vmessage(const char *type, const char *id,
										const char *format, va_list args)
{
	const QString message = FormatOfxMessage(format, args);
	if (message.isEmpty()) {
		return kOfxStatFailed;
	}

	const bool is_question =
		strncmp(type, kOfxMessageQuestion, strlen(kOfxMessageQuestion)) == 0;
	OfxStatus result = kOfxStatOK;
	auto show_message = [&]() {
		if (is_question) {
			const auto ret = QMessageBox::question(
				nullptr, "", message, QMessageBox::Ok, QMessageBox::Cancel);
			result = (ret == QMessageBox::Ok) ? kOfxStatReplyYes :
												kOfxStatReplyNo;
		} else {
			QMessageBox::information(nullptr, "", message);
			result = kOfxStatOK;
		}
	};

	if (IsGuiThread()) {
		show_message();
	} else if (auto *app = QCoreApplication::instance()) {
		if (is_question) {
			QMetaObject::invokeMethod(app, show_message,
									  Qt::BlockingQueuedConnection);
		} else {
			QMetaObject::invokeMethod(app, show_message, Qt::QueuedConnection);
		}
	} else if (is_question) {
		result = kOfxStatReplyNo;
	}

	return result;
}
OfxStatus OlivePluginInstance::setPersistentMessage(const char *type,
													const char *id,
													const char *format,
													va_list args)
{
	const QString message = FormatOfxMessage(format, args);
	if (message.isEmpty()) {
		return kOfxStatFailed;
	}

	ErrorType error_type;
	// If This is a error message
	if (strncmp(type, kOfxMessageError, strlen(kOfxMessageError)) == 0) {
		error_type = ErrorType::Error;
	}
	// A warning
	else if (strncmp(type, kOfxMessageWarning, strlen(kOfxMessageWarning)) ==
			 0) {
		error_type = ErrorType::Warning;
	}
	// A simple information
	else if (strncmp(type, kOfxMessageMessage, strlen(kOfxMessageMessage)) ==
			 0) {
		error_type = ErrorType::Message;
	} else {
		return kOfxStatFailed;
	}

	auto update_ui = [this, error_type, message]() {
		persistentErrors_.append({ error_type, message });
		switch (error_type) {
		case ErrorType::Error:
			QMessageBox::critical(nullptr, "", message);
			break;
		case ErrorType::Warning:
			QMessageBox::warning(nullptr, "", message);
			break;
		case ErrorType::Message:
			QMessageBox::information(nullptr, "", message);
			break;
		}
		if (node_) {
			emit node_->MessageCountChanged();
		}
	};

	if (IsGuiThread()) {
		update_ui();
	} else if (auto *app = QCoreApplication::instance()) {
		QMetaObject::invokeMethod(app, update_ui, Qt::QueuedConnection);
	}
	return kOfxStatOK;
}
OfxStatus OlivePluginInstance::clearPersistentMessage()
{
	auto clear_ui = [this]() {
		persistentErrors_.clear();
		// TODO: tell the shell to remove message.
		if (node_) {
			emit node_->MessageCountChanged();
		}
	};
	if (IsGuiThread()) {
		clear_ui();
	} else if (auto *app = QCoreApplication::instance()) {
		QMetaObject::invokeMethod(app, clear_ui, Qt::QueuedConnection);
	}
	return kOfxStatOK;
}
void OlivePluginInstance::getProjectSize(double &xSize, double &ySize) const
{
	double par = params_.pixel_aspect_ratio().toDouble();
	xSize = params_.width() * par;
	ySize = params_.height();
}
void OlivePluginInstance::getProjectOffset(double &xOffset,
										   double &yOffset) const
{
	double par = params_.pixel_aspect_ratio().toDouble();
	xOffset = params_.x() * par;
	yOffset = params_.y();
}
void OlivePluginInstance::getProjectExtent(double &xSize, double &ySize) const
{
	double par = params_.pixel_aspect_ratio().toDouble();
	xSize = params_.width() * par;
	ySize = params_.height();
}
double OlivePluginInstance::getProjectPixelAspectRatio() const
{
	double par = params_.pixel_aspect_ratio().toDouble();
	if (par == 0.0) {
		return 1.0; // default PAR when not explicitly set
	}
	return par;
}
double OlivePluginInstance::getFrameRate() const
{
	return params_.frame_rate().toDouble();
}

double OlivePluginInstance::getEffectDuration() const
{
	// Return a default duration value
	return 100.0;
}

double OlivePluginInstance::getFrameRecursive() const
{
	// Return current frame (this would typically be set by the host during rendering)
	return 0.0;
}

void OlivePluginInstance::getRenderScaleRecursive(double &x, double &y) const
{
	// Return default render scale (1.0, 1.0)
	x = 1.0;
	y = 1.0;
}
OFX::Host::Param::Instance *
OlivePluginInstance::newParam(const std::string &name,
							  OFX::Host::Param::Descriptor &desc)
{
	const std::string &type = desc.getType();

	if (type == kOfxParamTypeInteger) {
		return new IntegerInstance(node_, desc, this);
	} else if (type == kOfxParamTypeDouble) {
		return new DoubleInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeBoolean) {
		return new BooleanInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeChoice) {
		return new ChoiceInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeString) {
		return new StringInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeRGBA) {
		return new RGBAInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeRGB) {
		return new RGBInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeDouble2D) {
		return new Double2DInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeInteger2D) {
		return new Integer2DInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeDouble3D) {
		return new Double3DInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeInteger3D) {
		return new Integer3DInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeCustom || type == kOfxParamTypeBytes) {
		return new CustomInstance(node_, name, desc, this);
	} else if (type == kOfxParamTypeGroup) {
		return new GroupInstance(desc, this);
	} else if (type == kOfxParamTypePage) {
		return new PageInstance(desc, this);
	} else if (type == kOfxParamTypePushButton) {
		return new PushbuttonInstance(node_, name, desc, this);
	}

	return nullptr; // 未实现的类型
}
OfxStatus OlivePluginInstance::editBegin(const std::string &name)
{
	edit_depth_++;
	if (edit_depth_ == 1) {
		edit_command_ = nullptr;
		edit_label_.clear();
		edit_first_label_.clear();
		edit_param_count_ = 0;
		if (!name.empty()) {
			edit_first_label_ =
				QCoreApplication::translate("OlivePluginInstance", "Change %1")
					.arg(QString::fromStdString(name));
		}
	}
	return kOfxStatOK;
}
OfxStatus OlivePluginInstance::editEnd()
{
	if (edit_depth_ > 0) {
		edit_depth_--;
	}
	if (edit_depth_ == 0 && edit_command_) {
		QString label = edit_label_;
		if (label.isEmpty()) {
			if (edit_param_count_ <= 1 && !edit_first_label_.isEmpty()) {
				label = edit_first_label_;
			} else if (edit_param_count_ > 1 && !edit_first_label_.isEmpty()) {
				label = QCoreApplication::translate("OlivePluginInstance",
													"%1 (+%2)")
							.arg(edit_first_label_)
							.arg(edit_param_count_ - 1);
			} else {
				label = QCoreApplication::translate("OlivePluginInstance",
													"Edit Parameters");
			}
		}
		Core::instance()->undo_stack()->push(edit_command_, label);
		edit_command_ = nullptr;
		edit_label_.clear();
		edit_first_label_.clear();
		edit_param_count_ = 0;
	}
	return kOfxStatOK;
}

void OlivePluginInstance::SubmitUndoCommand(UndoCommand *command,
											const QString &label)
{
	if (!command) {
		return;
	}

	if (edit_depth_ > 0) {
		if (!edit_command_) {
			edit_command_ = new MultiUndoCommand();
		}
		edit_param_count_++;
		if (!label.isEmpty() && edit_first_label_.isEmpty()) {
			edit_first_label_ = label;
		}

		command->redo_now();
		edit_command_->add_child(new DeferredRedoCommand(command));
		return;
	}

	if (!IsGuiThread()) {
		command->redo_now();
		delete command;
		return;
	}

	Core::instance()->undo_stack()->push(command, label);
}

void OlivePluginInstance::progressStart(const std::string &message,
										const std::string &messageid)
{
	(void)messageid;
	progress_cancelled_ = false;
	progress_active_ = true;

	auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
	if (!app) {
		return;
	}

	if (progress_dialog_) {
		progress_dialog_->close();
		progress_dialog_->deleteLater();
	}

	QString dialog_message = message.empty() ? QStringLiteral("Processing...") :
											   QString::fromStdString(message);

	progress_dialog_ = new ::olive::ProgressDialog(
		dialog_message, QStringLiteral("OpenFX"), nullptr);
	progress_dialog_->setAttribute(Qt::WA_DeleteOnClose);
	QObject::connect(progress_dialog_, &::olive::ProgressDialog::Cancelled,
					 progress_dialog_,
					 [this]() { progress_cancelled_ = true; });
	progress_dialog_->show();
}

void OlivePluginInstance::progressEnd()
{
	progress_active_ = false;
	progress_cancelled_ = false;

	if (progress_dialog_) {
		progress_dialog_->close();
		progress_dialog_->deleteLater();
	}
}

bool OlivePluginInstance::progressUpdate(double t)
{
	if (!progress_active_) {
		return true;
	}

	if (progress_dialog_) {
		double clamped = qBound(0.0, t, 1.0);
		progress_dialog_->SetProgress(clamped);
	}

	return !progress_cancelled_;
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
OfxStatus OlivePluginInstance::contextAttachedAction()
{
	if (!open_gl_enabled_) {
		return kOfxStatReplyDefault;
	}
	return kOfxStatOK;
}

OfxStatus OlivePluginInstance::contextDetachedAction()
{
	if (!open_gl_enabled_) {
		return kOfxStatReplyDefault;
	}
	return kOfxStatOK;
}
#endif

double OlivePluginInstance::timeLineGetTime()
{
	if (ViewerOutput *viewer = GetActiveViewerOutput()) {
		return viewer->GetPlayhead().toDouble();
	}

	return 0.0;
}

void OlivePluginInstance::timeLineGotoTime(double t)
{
	if (ViewerOutput *viewer = GetActiveViewerOutput()) {
		viewer->SetPlayhead(olive::core::rational::fromDouble(t));
	}
}

void OlivePluginInstance::timeLineGetBounds(double &t1, double &t2)
{
	if (ViewerOutput *viewer = GetActiveViewerOutput()) {
		t1 = 0.0;
		t2 = viewer->GetLength().toDouble();
		return;
	}

	t1 = 0.0;
	t2 = 0.0;
}

void OlivePluginInstance::setCustomInArgs(const std::string &action,
										  OFX::Host::Property::Set &inArgs)
{
	if (action == kOfxImageEffectActionRender ||
		action == kOfxImageEffectActionBeginSequenceRender ||
		action == kOfxImageEffectActionEndSequenceRender) {
		inArgs.setIntProperty(kOfxImageEffectPropOpenGLEnabled,
							  open_gl_enabled_ ? 1 : 0);
	}
}

OFX::Host::ImageEffect::ClipInstance *OlivePluginInstance::newClipInstance(
	OFX::Host::ImageEffect::Instance *plugin,
	OFX::Host::ImageEffect::ClipDescriptor *descriptor, int index)
{
	// Create a new clip instance
	OliveClipInstance *clipInstance =
		new OliveClipInstance(plugin, *descriptor, params_);

	// Initialize base class clip properties from VideoParams so that
	// setupClipPreferencesArgs and plugin constructors (which may fetch
	// clips and query their properties before getClipPreferences is called)
	// have valid defaults instead of kOfxImageComponentNone / kOfxBitDepthNone.
	std::string depth = kOfxBitDepthFloat; // host default
	std::string comp = kOfxImageComponentRGBA; // host default

	switch (params_.format()) {
	case core::PixelFormat::U8:
		depth = kOfxBitDepthByte;
		break;
	case core::PixelFormat::U16:
		depth = kOfxBitDepthShort;
		break;
	case core::PixelFormat::F16:
		depth = kOfxBitDepthHalf;
		break;
	case core::PixelFormat::F32:
		depth = kOfxBitDepthFloat;
		break;
	default:
		break; // keep F32 default
	}

	switch (params_.channel_count()) {
	case 1:
		comp = kOfxImageComponentAlpha;
		break;
	case 3:
		comp = kOfxImageComponentRGB;
		break;
	case 4:
		comp = kOfxImageComponentRGBA;
		break;
	default:
		break; // keep RGBA default
	}

	clipInstance->setPixelDepth(depth);
	clipInstance->setComponents(comp);

	return clipInstance;
}

OlivePluginInstance::~OlivePluginInstance()
{
	if (!QCoreApplication::instance() ||
		qEnvironmentVariableIsSet("OAK_OFX_ITEST")) {
		_created = false;
	}
}

}
}
