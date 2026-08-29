/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include "ociolut.h"

#include <QFileInfo>
#include <QMetaObject>

#include <QApplication>

#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "render/lutlibrary.h"
#include "render/previewautocacher.h"
#include "render/rendermanager.h"

namespace olive
{

const QString OCIOLutNode::kFileInput = QStringLiteral("lut_file_in");
const QString OCIOLutNode::kDirectionInput = QStringLiteral("lut_dir_in");

#define super OCIOBaseNode

namespace
{

bool IsMainProcess()
{
	return qobject_cast<QApplication *>(QCoreApplication::instance()) !=
		   nullptr;
}

int ReadDirectionInput(const Node *node)
{
	QVariant v = node->GetStandardValue(OCIOLutNode::kDirectionInput);

	bool ok = false;
	int direction = v.toInt(&ok);
	if (ok) {
		return direction;
	}

	// Some old serializers stored the combo value as a string.
	const QString s = v.toString().toLower();
	if (s == QStringLiteral("forward") || s == QStringLiteral("0")) {
		return 0;
	}
	if (s == QStringLiteral("inverse") || s == QStringLiteral("1")) {
		return 1;
	}

	qWarning() << "OCIOLutNode: unexpected direction value" << v;
	return 0;
}

} // namespace

OCIOLutNode::OCIOLutNode()
{
	AddInput(kFileInput, NodeValue::kFile, QString(),
			 InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
	SetInputProperty(
		kFileInput, QStringLiteral("filter"),
		tr("LUT Files (*.cube *.3dl);;Cube LUT (*.cube);;3DL LUT (*.3dl);;All Files (*)"));
	SetInputProperty(kFileInput, QStringLiteral("placeholder"),
					 tr("Select a .cube or .3dl LUT file"));
	// Allow the UI to offer the global LUT library for this input
	SetInputProperty(kFileInput, QStringLiteral("lut_library"), true);

	AddInput(kDirectionInput, NodeValue::kCombo, 0,
			 InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

	qRegisterMetaType<olive::ColorProcessorPtr>();
}

QString OCIOLutNode::Name() const
{
	return tr("OCIO LUT");
}

QString OCIOLutNode::id() const
{
	return QStringLiteral("org.olivevideoeditor.Olive.ociolut");
}

QVector<Node::CategoryID> OCIOLutNode::Category() const
{
	return { kCategoryColor };
}

QString OCIOLutNode::Description() const
{
	return tr("Applies a LUT file through OpenColorIO.");
}

void OCIOLutNode::Retranslate()
{
	super::Retranslate();

	SetInputName(kTextureInput, tr("Input"));
	SetInputName(kFileInput, tr("LUT File"));
	SetInputName(kDirectionInput, tr("Direction"));
	SetComboBoxStrings(kDirectionInput, { tr("Forward"), tr("Inverse") });
}

void OCIOLutNode::InputValueChangedEvent(const QString &input, int element)
{
	Q_UNUSED(element)

	if (input == kFileInput || input == kDirectionInput) {
		// In the worker process, creating the OCIO processor can be slow and we
		// are often called from LoadGraph while the main process is blocked
		// waiting for a response. Defer generation to Value() time so the worker
		// can ack the graph load immediately.
		if (IsMainProcess()) {
			GenerateProcessor();
		} else {
			QMutexLocker locker(&gen_mutex_);
			processor_dirty_ = true;
		}
	}
}

void OCIOLutNode::ConfigChanged()
{
	if (IsMainProcess()) {
		GenerateProcessor();
	} else {
		QMutexLocker locker(&gen_mutex_);
		processor_dirty_ = true;
	}
}

void OCIOLutNode::Value(const NodeValueRow &value, const NodeGlobals &globals,
						NodeValueTable *table) const
{
	// Ensure the processor is up-to-date before the base class emits the color
	// transform job. This is especially important in the render worker, where
	// processor creation is deferred until the first render.
	EnsureProcessor();

	super::Value(value, globals, table);
}

void OCIOLutNode::GenerateProcessor()
{
	EnsureProcessor();

	// The processor has changed. In the main GUI process, refresh the viewer by
	// invalidating the cache and cancelling background cache jobs.
	// Invalidating first ensures any in-flight renders that complete afterwards
	// won't write stale frames back. The worker process uses QGuiApplication and
	// has no RenderManager/PreviewAutoCacher, so skip this step to avoid crashing.
	if (IsMainProcess()) {
		InvalidateAll(kTextureInput);
		if (RenderManager *rm = RenderManager::instance()) {
			if (PreviewAutoCacher *cacher = rm->GetCacher()) {
				cacher->CancelVideoTasks(false);
			}
		}
	}
}

void OCIOLutNode::EnsureProcessor() const
{
	QMutexLocker locker(&gen_mutex_);

	if (!processor_dirty_ && last_processor_ &&
		GetStandardValue(kFileInput).toString() == last_path_ &&
		ReadDirectionInput(this) == last_direction_) {
		return;
	}

	CreateProcessorFromInputs();
}

void OCIOLutNode::SetLastError(const QString &error) const
{
	if (last_error_ == error) {
		return;
	}

	last_error_ = error;

	// Make the error visible to the user instead of failing silently, but only
	// from the main process (the render worker has no status bar)
	if (!error.isEmpty() && IsMainProcess() && Core::instance()) {
		Core::instance()->ShowStatusBarMessage(error, 10000);
	}
}

bool OCIOLutNode::CreateProcessorFromInputs() const
{
	if (!manager()) {
		const_cast<OCIOLutNode *>(this)->set_processor(nullptr);
		last_processor_.reset();
		last_path_.clear();
		last_direction_ = -1;
		processor_dirty_ = false;
		return false;
	}

	const QString path = GetStandardValue(kFileInput).toString();
	const int direction = ReadDirectionInput(this);

	if (path.isEmpty()) {
		const_cast<OCIOLutNode *>(this)->set_processor(nullptr);
		last_processor_.reset();
		last_path_.clear();
		last_direction_ = -1;
		processor_dirty_ = false;
		SetLastError(QString());
		return false;
	}

	// Re-use the existing processor if the file and direction haven't changed.
	if (path == last_path_ && direction == last_direction_ && last_processor_) {
		processor_dirty_ = false;
		return false;
	}

	const QFileInfo info(path);
	if (!info.exists() || !info.isFile()) {
		qWarning() << "OCIO LUT file does not exist:" << path;
		const_cast<OCIOLutNode *>(this)->set_processor(nullptr);
		last_processor_.reset();
		last_path_.clear();
		last_direction_ = -1;
		processor_dirty_ = false;
		SetLastError(tr("OCIO LUT: file does not exist: %1").arg(path));
		return false;
	}

	const QString suffix = info.suffix();
	if (!LUTLibrary::IsSupportedExtension(suffix)) {
		qWarning() << "Unsupported OCIO LUT file extension:" << path;
		const_cast<OCIOLutNode *>(this)->set_processor(nullptr);
		last_processor_.reset();
		last_path_.clear();
		last_direction_ = -1;
		processor_dirty_ = false;
		SetLastError(
			tr("OCIO LUT: unsupported LUT file extension (expected .cube or "
			   ".3dl): %1")
				.arg(path));
		return false;
	}

	ColorProcessorPtr processor;
	try {
		const bool forward = static_cast<ColorProcessor::Direction>(
								 direction) == ColorProcessor::kNormal;
		qDebug() << "OCIOLutNode: creating processor for" << path
				 << "direction=" << direction
				 << "ocio_dir=" << (forward ? "FORWARD" : "INVERSE")
				 << "process=" << (IsMainProcess() ? "main" : "worker");

		OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
		transform->setSrc(path.toUtf8().constData());
		transform->setInterpolation(OCIO::INTERP_LINEAR);
		transform->setDirection(forward ? OCIO::TRANSFORM_DIR_FORWARD :
										  OCIO::TRANSFORM_DIR_INVERSE);

		processor = ColorProcessor::Create(
			manager()->GetConfig()->getProcessor(transform));
	} catch (const std::exception &e) {
		qWarning() << "OCIO LUT processor error:" << e.what();
		processor = nullptr;
	}

	if (!processor) {
		SetLastError(tr("OCIO LUT: failed to load LUT file: %1").arg(path));
	} else {
		SetLastError(QString());
	}

	last_path_ = path;
	last_direction_ = direction;
	last_processor_ = processor;
	const_cast<OCIOLutNode *>(this)->set_processor(processor);
	processor_dirty_ = false;

	return true;
}

} // namespace olive
