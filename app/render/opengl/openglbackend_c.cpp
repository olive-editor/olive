#include "render/backend/renderbackend_c.h"

#include <QOpenGLContext>
#include <QPointF>
#include <QVariant>

#include "render/job/acceleratedjob.h"
#include "render/opengl/openglrenderer.h"
#include "render/shadercode.h"
#include "render/texture.h"
#include "render/videoparams.h"

namespace
{

class BackendOpenGLRenderer : public olive::OpenGLRenderer {
public:
	using olive::OpenGLRenderer::OpenGLRenderer;
	using olive::OpenGLRenderer::Blit;
	using olive::OpenGLRenderer::CreateNativeTexture;
	using olive::OpenGLRenderer::DestroyInternal;
	using olive::OpenGLRenderer::DestroyNativeTexture;
	using olive::OpenGLRenderer::AttachTextureAsDestination;
	using olive::OpenGLRenderer::DetachTextureAsDestination;
};

// Converts the opaque C ABI handle back to the C++ renderer used internally.
BackendOpenGLRenderer *Renderer(OakRenderBackendHandle handle)
{
	return static_cast<BackendOpenGLRenderer *>(handle);
}

// Interprets ABI QVariant payloads without copying; both modules are built
// against the same Qt/C++ ABI in this first-generation dynamic backend.
const QVariant &VariantRef(const void *variant)
{
	return *static_cast<const QVariant *>(variant);
}

} // namespace

// Creates the backend object and returns it as an opaque C handle.
OAK_RENDER_BACKEND_EXPORT OakRenderBackendHandle
oak_renderer_create(void *parent)
{
	return new BackendOpenGLRenderer(static_cast<QObject *>(parent));
}

// Destroys the opaque backend object created by oak_renderer_create().
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy(OakRenderBackendHandle handle)
{
	delete Renderer(handle);
}

// Reports static OpenGL backend capabilities to the adapter.
OAK_RENDER_BACKEND_EXPORT bool
oak_renderer_get_info(OakRenderBackendHandle handle,
					  OakRenderBackendInfo *out_info)
{
	if (!handle || !out_info) {
		return false;
	}
	out_info->abi_version = 1;
	out_info->kind = OAK_RENDER_BACKEND_OPENGL;
	out_info->capabilities =
		OAK_RENDER_BACKEND_CAP_TEXTURES | OAK_RENDER_BACKEND_CAP_SHADERS |
		OAK_RENDER_BACKEND_CAP_BLIT | OAK_RENDER_BACKEND_CAP_READBACK |
		OAK_RENDER_BACKEND_CAP_VIEWER_CONTEXT;
	out_info->name = "opengl";
	out_info->status = "available";
	return true;
}

// OpenGL availability is context-dependent, so object creation is the minimum
// availability signal for this backend.
OAK_RENDER_BACKEND_EXPORT bool
oak_renderer_is_available(OakRenderBackendHandle handle)
{
	return handle != nullptr;
}

// Initializes an offscreen OpenGL context for non-viewer users.
OAK_RENDER_BACKEND_EXPORT bool oak_renderer_init(OakRenderBackendHandle handle)
{
	return Renderer(handle)->Init();
}

// Initializes the backend against a caller-owned viewer OpenGL context.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_init_with_context(OakRenderBackendHandle handle, void *context)
{
	Renderer(handle)->Init(static_cast<QOpenGLContext *>(context));
}

// Runs renderer post-initialization once the GL context is available.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_post_init(OakRenderBackendHandle handle)
{
	Renderer(handle)->PostInit();
}

// Releases post-init OpenGL surface/context state.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_post_destroy(OakRenderBackendHandle handle)
{
	Renderer(handle)->PostDestroy();
}

// Releases renderer-owned GL resources before object destruction.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_internal(OakRenderBackendHandle handle)
{
	Renderer(handle)->DestroyInternal();
}

// Clears either the widget framebuffer or a texture destination.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_clear_destination(OakRenderBackendHandle handle, void *texture,
							   double r, double g, double b, double a)
{
	Renderer(handle)->ClearDestination(static_cast<olive::Texture *>(texture),
									   r, g, b, a);
}

// Creates an OpenGL texture and writes its QVariant handle to out_variant.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_create_native_texture(
	OakRenderBackendHandle handle, int width, int height, int depth, int format,
	int channel_count, const void *data, int linesize, void *out_variant)
{
	*static_cast<QVariant *>(out_variant) =
		Renderer(handle)->CreateNativeTexture(
			width, height, depth,
			static_cast<olive::PixelFormat::Format>(format), channel_count,
			data, linesize);
}

// Destroys an OpenGL texture represented by a QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_native_texture(OakRenderBackendHandle handle,
									const void *variant)
{
	Renderer(handle)->DestroyNativeTexture(VariantRef(variant));
}

// Compiles an OpenGL shader program and returns its QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_create_native_shader(OakRenderBackendHandle handle,
								  const void *shader_code, void *out_variant)
{
	*static_cast<QVariant *>(out_variant) =
		Renderer(handle)->CreateNativeShader(
			*static_cast<const olive::ShaderCode *>(shader_code));
}

// Destroys an OpenGL shader program represented by a QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_native_shader(OakRenderBackendHandle handle,
								   const void *variant)
{
	Renderer(handle)->DestroyNativeShader(VariantRef(variant));
}

// Uploads CPU pixel data into an OpenGL texture.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_upload_to_texture(OakRenderBackendHandle handle,
							   const void *variant, const void *video_params,
							   const void *data, int linesize)
{
	Renderer(handle)->UploadToTexture(
		VariantRef(variant),
		*static_cast<const olive::VideoParams *>(video_params), data, linesize);
}

// Reads an OpenGL texture back to CPU memory.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_download_from_texture(
	OakRenderBackendHandle handle, const void *variant,
	const void *video_params, void *data, int linesize)
{
	Renderer(handle)->DownloadFromTexture(
		VariantRef(variant),
		*static_cast<const olive::VideoParams *>(video_params), data, linesize);
}

// Flushes/waits for pending OpenGL work as required by the renderer.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_flush(OakRenderBackendHandle handle)
{
	Renderer(handle)->Flush();
}

// Reads one pixel from an OpenGL texture.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_get_pixel_from_texture(OakRenderBackendHandle handle,
									void *texture, const void *point,
									void *out_color)
{
	*static_cast<olive::Color *>(out_color) =
		Renderer(handle)->GetPixelFromTexture(
			static_cast<olive::Texture *>(texture),
			*static_cast<const QPointF *>(point));
}

// Executes a shader blit through the wrapped C++ OpenGL renderer.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_blit(OakRenderBackendHandle handle,
												 const void *shader, void *job,
												 void *destination,
												 const void *destination_params,
												 bool clear_destination)
{
	Renderer(handle)->Blit(
		VariantRef(shader), *static_cast<olive::AcceleratedJob *>(job),
		static_cast<olive::Texture *>(destination),
		*static_cast<const olive::VideoParams *>(destination_params),
		clear_destination);
}

// Exposes the wrapped OpenGL context for GL-specific integrations.
OAK_RENDER_BACKEND_EXPORT void *
oak_renderer_opengl_context(OakRenderBackendHandle handle)
{
	return Renderer(handle)->context();
}

// Binds an output texture for OFX OpenGL rendering.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_attach_output_texture(OakRenderBackendHandle handle,
								   const void *texture_id)
{
	Renderer(handle)->AttachTextureAsDestination(VariantRef(texture_id));
}

// Detaches any OFX OpenGL output texture binding.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_detach_output_texture(OakRenderBackendHandle handle)
{
	Renderer(handle)->DetachTextureAsDestination();
}
