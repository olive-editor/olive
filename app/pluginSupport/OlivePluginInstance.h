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
#ifndef OLIVE_INSTANCE_H
#define OLIVE_INSTANCE_H
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include <QString>
#include "ofxhImageEffect.h"
#include "node/plugins/Plugin.h"
#include "render/videoparams.h"
#include "undo/undocommand.h"

#include <map>
#include <mutex>
#include <QCoreApplication>
#include <QPointer>
#include <QThread>
#include <qcontainerfwd.h>
#include <qlist.h>

namespace olive
{

inline bool IsGuiThread()
{
	if (auto *app = QCoreApplication::instance()) {
		return QThread::currentThread() == app->thread();
	}
	return true;
}
class ProgressDialog;
namespace plugin
{
class PluginNode;
enum class ErrorType { Error, Warning, Message };
struct PersistentErrors {
	ErrorType type;
	QString message;
};
class OlivePluginInstance : public OFX::Host::ImageEffect::Instance {
public:
	OlivePluginInstance(OFX::Host::ImageEffect::ImageEffectPlugin *plugin,
						OFX::Host::ImageEffect::Descriptor &desc,
						const std::string &context, bool interactive)
		: OFX::Host::ImageEffect::Instance(plugin, desc, context, interactive)
	{
	}
	OlivePluginInstance(OlivePluginInstance &instance)
		: Instance(instance._plugin, *instance._descriptor, instance._context,
				   instance._interactive)
	{
		// Do NOT shallow-copy _clips: Instance::~Instance() deletes them,
		// which would cause a double-free. Clips are re-created in populate().
		_created = instance._created;
		_clipPrefsDirty = instance._clipPrefsDirty;
		_continuousSamples = instance._continuousSamples;
		_frameVarying = instance._frameVarying;
		_outputPreMultiplication = instance._outputPreMultiplication;
		_outputFielding = instance._outputFielding;
		_outputFrameRate = instance._outputFrameRate;
	}
	explicit OlivePluginInstance(Instance &instance)
		: Instance(instance) {};
	~OlivePluginInstance() override;
	const std::string &getDefaultOutputFielding() const override;

	void setVideoParam(VideoParams params)
	{
		this->params_ = params;
	}
	void setNode(std::shared_ptr<PluginNode> node);
	std::shared_ptr<PluginNode> node() const
	{
		return node_;
	}
	void setOpenGLEnabled(bool enabled)
	{
		open_gl_enabled_ = enabled;
	}
	bool isCreated() const
	{
		return _created;
	}
	OFX::Host::ImageEffect::ClipInstance *
	newClipInstance(OFX::Host::ImageEffect::Instance *plugin,
					OFX::Host::ImageEffect::ClipDescriptor *descriptor,
					int index) override;

	OfxStatus vmessage(const char *type, const char *id, const char *format,
					   va_list args) override;

	OfxStatus setPersistentMessage(const char *type, const char *id,
								   const char *format, va_list args) override;

	OfxStatus clearPersistentMessage() override;
	int persistentMessageCount() const
	{
		return persistentErrors_.size();
	}
	const QList<PersistentErrors> &persistentMessages() const
	{
		return persistentErrors_;
	}

	void getProjectSize(double &xSize, double &ySize) const override;
	void getProjectOffset(double &xOffset, double &yOffset) const override;
	void getProjectExtent(double &xSize, double &ySize) const override;
	// The pixel aspect ratio of the current project
	double getProjectPixelAspectRatio() const override;

	// The duration of the effect
	// This contains the duration of the plug-in effect, in frames.
	double getEffectDuration() const override;

	// For an instance, this is the frame rate of the project the effect is in.
	double getFrameRate() const override;

	/// This is called whenever a param is changed by the plugin so that
	/// the recursive instanceChangedAction will be fed the correct frame
	double getFrameRecursive() const override;

	/// This is called whenever a param is changed by the plugin so that
	/// the recursive instanceChangedAction will be fed the correct
	/// renderScale
	void getRenderScaleRecursive(double &x, double &y) const override;

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	// overridden for Param::SetInstance

	/// make a parameter instance
	///
	/// Client host code needs to implement this
	OFX::Host::Param::Instance *
	newParam(const std::string &name,
			 OFX::Host::Param::Descriptor &Descriptor) override;

	void SubmitUndoCommand(UndoCommand *command, const QString &label);

	/// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditBegin
	///
	/// Client host code needs to implement this
	virtual OfxStatus editBegin(const std::string &name) override;

	/// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
	///
	/// Client host code needs to implement this
	virtual OfxStatus editEnd() override;

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	// overridden for Progress::ProgressI

	/// Start doing progress.
	virtual void progressStart(const std::string &message,
							   const std::string &messageid) override;

	/// finish yer progress
	virtual void progressEnd() override;

	/// set the progress to some level of completion, returns
	/// false if you should abandon processing, true to continue
	virtual bool progressUpdate(double t) override;

#ifdef OFX_SUPPORTS_OPENGLRENDER
	virtual OfxStatus contextAttachedAction() override;
	virtual OfxStatus contextDetachedAction() override;
#endif

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	// overridden for TimeLine::TimeLineI

	/// get the current time on the timeline. This is not necessarily the same
	/// time as being passed to an action (eg render)
	virtual double timeLineGetTime();

	/// set the timeline to a specific time
	virtual void timeLineGotoTime(double t);

	/// get the first and last times available on the effect's timeline
	virtual void timeLineGetBounds(double &t1, double &t2);

	void setCustomInArgs(const std::string &action,
						 OFX::Host::Property::Set &inArgs) override;

private:
	QList<PersistentErrors> persistentErrors_;
	VideoParams params_;
	std::shared_ptr<PluginNode> node_ = nullptr;
	int edit_depth_ = 0;
	MultiUndoCommand *edit_command_ = nullptr;
	QString edit_label_;
	QString edit_first_label_;
	int edit_param_count_ = 0;
	QPointer<olive::ProgressDialog> progress_dialog_;
	bool progress_cancelled_ = false;
	bool progress_active_ = false;
	bool open_gl_enabled_ = false;

public:
	std::mutex &mutex()
	{
		return mutex_;
	}

private:
	std::mutex mutex_;
};
}
}
#endif
