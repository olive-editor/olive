#include "dynamicrenderer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QOpenGLContext>

namespace olive
{

// Stores the requested backend name; the actual backend may later become
// OpenGL if loading or availability checks require a Vulkan fallback.
DynamicRenderer::DynamicRenderer(const QString &backend, QObject *parent)
	: Renderer(parent)
	, backend_(backend.toLower())
{
}

// Tears down the backend in the reverse order used by Load(): release renderer
// resources, destroy the opaque backend object, then unload the shared library.
DynamicRenderer::~DynamicRenderer()
{
	Destroy();
	PostDestroy();
	if (handle_ && destroy_) {
		destroy_(handle_);
		handle_ = nullptr;
	}
	if (library_.isLoaded()) {
		library_.unload();
	}
}

// Builds the private backend library path for the current platform.
// The search is intentionally restricted to Oak-controlled directories so a
// system libGL/libvulkan loader is never mistaken for an Oak render backend.
QString DynamicRenderer::LibraryFilename() const
{
	QString base;
	if (backend_ == QStringLiteral("opengl")) {
		base = QStringLiteral("oakgl");
	} else if (backend_ == QStringLiteral("vulkan")) {
		base = QStringLiteral("oakvulkan");
	} else {
		// Unknown backend: use the name verbatim so the load fails and the
		// caller's OpenGL fallback engages
		base = backend_;
	}
#if defined(Q_OS_WIN)
	const QString filename = base + QStringLiteral(".dll");
#elif defined(Q_OS_MAC)
	const QString filename =
		QStringLiteral("lib") + base + QStringLiteral(".dylib");
#else
	const QString filename =
		QStringLiteral("lib") + base + QStringLiteral(".so");
#endif

	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QStringList candidates = {
		app_dir.filePath(filename),
		app_dir.filePath(
			QDir(QStringLiteral("render_backends")).filePath(filename)),
		app_dir.filePath(QDir(QStringLiteral("../lib")).filePath(filename)),
		app_dir.filePath(QDir(QStringLiteral("../../lib")).filePath(filename)),
		app_dir.filePath(QDir(QStringLiteral("../app")).filePath(filename)),
		app_dir.filePath(QDir(QStringLiteral("../../app")).filePath(filename))
	};
	for (const QString &candidate : candidates) {
		if (QFileInfo::exists(candidate)) {
			return candidate;
		}
	}
	return candidates.first();
}

// Loads the selected backend, resolves its C ABI table, creates the opaque
// backend object, and optionally falls back from Vulkan to OpenGL when runtime
// availability checks fail.
bool DynamicRenderer::Load()
{
	if (handle_) {
		return true;
	}

	library_.setFileName(LibraryFilename());
	if (!library_.load()) {
		if (backend_ == QStringLiteral("vulkan")) {
			qWarning()
				<< "Failed to load Vulkan render backend" << library_.fileName()
				<< library_.errorString() << "falling back to OpenGL backend";
			backend_ = QStringLiteral("opengl");
			library_.setFileName(LibraryFilename());
		}

		if (!library_.load()) {
			qWarning() << "Failed to load render backend" << backend_
					   << library_.fileName() << library_.errorString();
			return false;
		}
	}

	if (!ResolveFunctions()) {
		qWarning() << "Render backend is missing required symbols" << backend_;
		library_.unload();
		return false;
	}

	// Pass this (rather than this->parent()) so the backend renderer becomes a
	// child QObject of the adapter. That ensures it follows DynamicRenderer when
	// the latter is moved to the render thread; otherwise it stays in the thread
	// where Load() was called and every GL operation is rejected as "wrong
	// thread", producing a black screen.
	handle_ = create_(this);
	if (!handle_) {
		library_.unload();
		return false;
	}
	if (is_available_ && !is_available_(handle_)) {
		qWarning() << "Render backend is not available" << backend_
				   << library_.fileName();
		if (backend_ == QStringLiteral("vulkan")) {
			return FallbackToOpenGL();
		}
		destroy_(handle_);
		handle_ = nullptr;
		library_.unload();
		return false;
	}
	return handle_ != nullptr;
}

// Resolves the mandatory C ABI entry points from the loaded shared library.
// Optional information probes are resolved after the required render interface.
bool DynamicRenderer::ResolveFunctions()
{
	ResetFunctions();
#define RESOLVE(member, type, symbol)                          \
	member = reinterpret_cast<type>(library_.resolve(symbol)); \
	if (!member)                                               \
	return false

	RESOLVE(create_, OakBackendCreateFn, "oak_renderer_create");
	RESOLVE(destroy_, OakBackendDestroyFn, "oak_renderer_destroy");
	RESOLVE(init_, OakBackendInitFn, "oak_renderer_init");
	RESOLVE(init_with_context_, OakBackendInitWithContextFn,
			"oak_renderer_init_with_context");
	RESOLVE(post_init_, OakBackendPostInitFn, "oak_renderer_post_init");
	RESOLVE(post_destroy_, OakBackendPostDestroyFn,
			"oak_renderer_post_destroy");
	RESOLVE(destroy_internal_, OakBackendDestroyInternalFn,
			"oak_renderer_destroy_internal");
	RESOLVE(clear_destination_, OakBackendClearDestinationFn,
			"oak_renderer_clear_destination");
	RESOLVE(create_native_texture_, OakBackendCreateNativeTextureFn,
			"oak_renderer_create_native_texture");
	RESOLVE(destroy_native_texture_, OakBackendDestroyNativeTextureFn,
			"oak_renderer_destroy_native_texture");
	RESOLVE(create_native_shader_, OakBackendCreateNativeShaderFn,
			"oak_renderer_create_native_shader");
	RESOLVE(destroy_native_shader_, OakBackendDestroyNativeShaderFn,
			"oak_renderer_destroy_native_shader");
	RESOLVE(upload_to_texture_, OakBackendUploadToTextureFn,
			"oak_renderer_upload_to_texture");
	RESOLVE(download_from_texture_, OakBackendDownloadFromTextureFn,
			"oak_renderer_download_from_texture");
	RESOLVE(flush_, OakBackendFlushFn, "oak_renderer_flush");
	RESOLVE(get_pixel_from_texture_, OakBackendGetPixelFromTextureFn,
			"oak_renderer_get_pixel_from_texture");
	RESOLVE(blit_, OakBackendBlitFn, "oak_renderer_blit");
	RESOLVE(attach_output_texture_, OakBackendAttachOutputTextureFn,
			"oak_renderer_attach_output_texture");
	RESOLVE(detach_output_texture_, OakBackendDetachOutputTextureFn,
			"oak_renderer_detach_output_texture");
	RESOLVE(opengl_context_, OakBackendOpenGLContextFn,
			"oak_renderer_opengl_context");
#undef RESOLVE
	get_info_ = reinterpret_cast<OakBackendGetInfoFn>(
		library_.resolve("oak_renderer_get_info"));
	is_available_ = reinterpret_cast<OakBackendIsAvailableFn>(
		library_.resolve("oak_renderer_is_available"));
	return true;
}

// Discards a partially-created backend and restarts loading with the OpenGL
// backend. This keeps RenderManager's fallback path inside the adapter.
bool DynamicRenderer::FallbackToOpenGL()
{
	if (handle_ && destroy_) {
		destroy_(handle_);
		handle_ = nullptr;
	}
	if (library_.isLoaded()) {
		library_.unload();
	}
	ResetFunctions();
	backend_ = QStringLiteral("opengl");
	return Load();
}

// Clears all cached C function pointers so a failed backend cannot leave stale
// call targets behind for a later fallback load.
void DynamicRenderer::ResetFunctions()
{
	create_ = nullptr;
	destroy_ = nullptr;
	get_info_ = nullptr;
	is_available_ = nullptr;
	init_ = nullptr;
	init_with_context_ = nullptr;
	post_init_ = nullptr;
	post_destroy_ = nullptr;
	destroy_internal_ = nullptr;
	clear_destination_ = nullptr;
	create_native_texture_ = nullptr;
	destroy_native_texture_ = nullptr;
	create_native_shader_ = nullptr;
	destroy_native_shader_ = nullptr;
	upload_to_texture_ = nullptr;
	download_from_texture_ = nullptr;
	flush_ = nullptr;
	get_pixel_from_texture_ = nullptr;
	blit_ = nullptr;
	attach_output_texture_ = nullptr;
	detach_output_texture_ = nullptr;
	opengl_context_ = nullptr;
}

// Returns backend metadata exposed by the dynamic library when available.
bool DynamicRenderer::GetBackendInfo(OakRenderBackendInfo *out_info) const
{
	return handle_ && get_info_ && out_info && get_info_(handle_, out_info);
}

// Initializes the loaded backend using its own context/device creation path.
bool DynamicRenderer::Init()
{
	return Load() && init_(handle_);
}

// Initializes an OpenGL backend against an existing widget context; non-OpenGL
// backends may ignore the context on the library side.
bool DynamicRenderer::InitWithOpenGLContext(QOpenGLContext *context)
{
	if (!Load()) {
		return false;
	}
	init_with_context_(handle_, context);
	return true;
}

// Forwards post-destroy cleanup to the backend while the library is still
// loaded and its symbols are still valid.
void DynamicRenderer::PostDestroy()
{
	if (handle_ && post_destroy_) {
		post_destroy_(handle_);
	}
}

// Runs backend post-initialization after Init/InitWithOpenGLContext has
// established the device or GL context.
void DynamicRenderer::PostInit()
{
	if (handle_) {
		post_init_(handle_);
	}
}

// Forwards render target clearing through the C ABI.
void DynamicRenderer::ClearDestination(Texture *texture, double r, double g,
									   double b, double a)
{
	clear_destination_(handle_, texture, r, g, b, a);
}

// Creates a backend-native shader and receives the result as an opaque QVariant
// because this first-generation ABI still shares C++/Qt types between modules.
QVariant DynamicRenderer::CreateNativeShader(ShaderCode code)
{
	QVariant out;
	create_native_shader_(handle_, &code, &out);
	return out;
}

// Releases a backend-native shader handle.
void DynamicRenderer::DestroyNativeShader(QVariant shader)
{
	destroy_native_shader_(handle_, &shader);
}

// Uploads CPU pixel data into a backend texture through the dynamic ABI.
void DynamicRenderer::UploadToTexture(const QVariant &handle,
									  const VideoParams &params,
									  const void *data, int linesize)
{
	upload_to_texture_(handle_, &handle, &params, data, linesize);
}

// Downloads backend texture data into a caller-provided CPU buffer.
void DynamicRenderer::DownloadFromTexture(const QVariant &handle,
										  const VideoParams &params, void *data,
										  int linesize)
{
	download_from_texture_(handle_, &handle, &params, data, linesize);
}

// Waits for backend work to become visible to subsequent CPU or GPU consumers.
void DynamicRenderer::Flush()
{
	flush_(handle_);
}

// Reads a single pixel through the backend-provided readback hook.
Color DynamicRenderer::GetPixelFromTexture(Texture *texture, const QPointF &pt)
{
	Color out;
	get_pixel_from_texture_(handle_, texture, &pt, &out);
	return out;
}

// Exposes the wrapped OpenGL context when the backend is OpenGL; Vulkan returns
// null so callers can avoid GL-only paths.
QOpenGLContext *DynamicRenderer::OpenGLContext() const
{
	return opengl_context_ && handle_ ?
			   static_cast<QOpenGLContext *>(opengl_context_(handle_)) :
			   nullptr;
}

// Reports the effective backend after any load-time fallback has completed.
bool DynamicRenderer::IsOpenGL() const
{
	return backend_ == QStringLiteral("opengl");
}

bool DynamicRenderer::IsVulkan() const
{
	return backend_ == QStringLiteral("vulkan");
}

// Dispatches a shader blit to the loaded backend.
void DynamicRenderer::Blit(QVariant shader, AcceleratedJob &job,
						   Texture *destination, VideoParams destination_params,
						   bool clear_destination)
{
	blit_(handle_, &shader, &job, destination, &destination_params,
		  clear_destination);
}

// Allocates a backend-native texture and wraps its opaque handle in QVariant.
QVariant DynamicRenderer::CreateNativeTexture(int width, int height, int depth,
											  PixelFormat format,
											  int channel_count,
											  const void *data, int linesize)
{
	QVariant out;
	create_native_texture_(handle_, width, height, depth, format, channel_count,
						   data, linesize, &out);
	return out;
}

// Releases a backend-native texture handle.
void DynamicRenderer::DestroyNativeTexture(QVariant texture)
{
	destroy_native_texture_(handle_, &texture);
}

// Releases renderer-owned backend resources before the backend object itself is
// destroyed.
void DynamicRenderer::DestroyInternal()
{
	if (handle_) {
		destroy_internal_(handle_);
	}
}

// Exposes OFX OpenGL output binding through the dynamic backend when supported.
void DynamicRenderer::AttachOutputTexture(Texture *texture)
{
	if (attach_output_texture_ && texture) {
		QVariant id = texture->id();
		attach_output_texture_(handle_, &id);
	}
}

// Clears any OFX output texture binding owned by the backend.
void DynamicRenderer::DetachOutputTexture()
{
	if (detach_output_texture_) {
		detach_output_texture_(handle_);
	}
}

}
