#include "render/backend/renderbackend_c.h"

#include <QDebug>
#include <QOpenGLContext>
#include <QPointF>
#include <QVariant>

#include "render/job/acceleratedjob.h"
#include "render/shadercode.h"
#include "render/texture.h"
#include "render/videoparams.h"
#include "render/vulkan/vulkanrenderer.h"

namespace
{

class BackendVulkanRenderer : public olive::VulkanRenderer {
public:
	using olive::VulkanRenderer::VulkanRenderer;
	using olive::VulkanRenderer::Blit;
	using olive::VulkanRenderer::CreateNativeTexture;
	using olive::VulkanRenderer::DestroyInternal;
	using olive::VulkanRenderer::DestroyNativeTexture;
};

// Converts the opaque C ABI handle back to the C++ Vulkan renderer.
BackendVulkanRenderer *Renderer(OakRenderBackendHandle handle)
{
	return static_cast<BackendVulkanRenderer *>(handle);
}

// Interprets ABI QVariant payloads without copying; this ABI version assumes
// the host and backend are built with the same Qt/C++ ABI.
const QVariant &VariantRef(const void *variant)
{
	return *static_cast<const QVariant *>(variant);
}

} // namespace

// Creates the Vulkan backend object and returns it as an opaque C handle.
OAK_RENDER_BACKEND_EXPORT OakRenderBackendHandle
oak_renderer_create(void *parent)
{
	return new BackendVulkanRenderer(static_cast<QObject *>(parent));
}

// Destroys the opaque backend object created by oak_renderer_create().
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy(OakRenderBackendHandle handle)
{
	delete Renderer(handle);
}

// Reports Vulkan backend capabilities and runtime availability status.
OAK_RENDER_BACKEND_EXPORT bool
oak_renderer_get_info(OakRenderBackendHandle handle,
					  OakRenderBackendInfo *out_info)
{
	if (!handle || !out_info) {
		return false;
	}
	out_info->abi_version = 1;
	out_info->kind = OAK_RENDER_BACKEND_VULKAN;
	out_info->capabilities =
		OAK_RENDER_BACKEND_CAP_TEXTURES | OAK_RENDER_BACKEND_CAP_SHADERS |
		OAK_RENDER_BACKEND_CAP_BLIT | OAK_RENDER_BACKEND_CAP_READBACK;
	out_info->name = "vulkan";
	out_info->status = Renderer(handle)->IsAvailable() ? "available" :
														 "unavailable";
	return true;
}

// Probes runtime availability by trying Init() once; this lets missing ICDs or
// unusable drivers fall back before normal rendering starts.
OAK_RENDER_BACKEND_EXPORT bool
oak_renderer_is_available(OakRenderBackendHandle handle)
{
	auto *r = Renderer(handle);
	if (!r || r->IsAvailable()) {
		return r && r->IsAvailable();
	}
	// Try to initialize if not already available
	if (r->Init()) {
		r->PostInit();
	}
	return r->IsAvailable();
}

// Initializes the Vulkan device path.
OAK_RENDER_BACKEND_EXPORT bool oak_renderer_init(OakRenderBackendHandle handle)
{
	return Renderer(handle)->Init();
}

// Vulkan does not use a QOpenGLContext; the argument is accepted for ABI parity.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_init_with_context(OakRenderBackendHandle handle, void *context)
{
	Q_UNUSED(context)
	Renderer(handle)->Init();
}

// Creates reusable Vulkan resources after device initialization.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_post_init(OakRenderBackendHandle handle)
{
	Renderer(handle)->PostInit();
}

// Reserved for API symmetry; Vulkan cleanup is handled by destroy_internal.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_post_destroy(OakRenderBackendHandle handle)
{
	Renderer(handle)->PostDestroy();
}

// Releases all Vulkan resources owned by the renderer.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_internal(OakRenderBackendHandle handle)
{
	Renderer(handle)->DestroyInternal();
}

// Clears a Vulkan texture destination.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_clear_destination(OakRenderBackendHandle handle, void *texture,
							   double r, double g, double b, double a)
{
	Renderer(handle)->ClearDestination(static_cast<olive::Texture *>(texture),
									   r, g, b, a);
}

// Creates a Vulkan texture and writes its QVariant handle to out_variant.
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

// Destroys a Vulkan texture represented by a QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_native_texture(OakRenderBackendHandle handle,
									const void *variant)
{
	Renderer(handle)->DestroyNativeTexture(VariantRef(variant));
}

// Compiles a Vulkan shader and returns its QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_create_native_shader(OakRenderBackendHandle handle,
								  const void *shader_code, void *out_variant)
{
	*static_cast<QVariant *>(out_variant) =
		Renderer(handle)->CreateNativeShader(
			*static_cast<const olive::ShaderCode *>(shader_code));
}

// Destroys a Vulkan shader represented by a QVariant handle.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_destroy_native_shader(OakRenderBackendHandle handle,
								   const void *variant)
{
	Renderer(handle)->DestroyNativeShader(VariantRef(variant));
}

// Uploads CPU pixel data into a Vulkan texture.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_upload_to_texture(OakRenderBackendHandle handle,
							   const void *variant, const void *video_params,
							   const void *data, int linesize)
{
	Renderer(handle)->UploadToTexture(
		VariantRef(variant),
		*static_cast<const olive::VideoParams *>(video_params), data, linesize);
}

// Downloads a Vulkan texture to CPU memory.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_download_from_texture(
	OakRenderBackendHandle handle, const void *variant,
	const void *video_params, void *data, int linesize)
{
	Renderer(handle)->DownloadFromTexture(
		VariantRef(variant),
		*static_cast<const olive::VideoParams *>(video_params), data, linesize);
}

// Waits for all queued Vulkan work to finish.
OAK_RENDER_BACKEND_EXPORT void oak_renderer_flush(OakRenderBackendHandle handle)
{
	Renderer(handle)->Flush();
}

// Reads one pixel from a Vulkan texture.
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

// Executes a shader blit through the wrapped Vulkan renderer.
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

// Vulkan has no OpenGL context; return null so callers avoid GL-only paths.
OAK_RENDER_BACKEND_EXPORT void *
oak_renderer_opengl_context(OakRenderBackendHandle handle)
{
	Q_UNUSED(handle)
	return nullptr;
}

// OFX OpenGL output attachment is unsupported in Vulkan and intentionally no-op.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_attach_output_texture(OakRenderBackendHandle handle,
								   const void *texture_id)
{
	Q_UNUSED(handle)
	Q_UNUSED(texture_id)
	// Vulkan does not support OFX OpenGL render output attachment.
}

// OFX OpenGL output detachment is unsupported in Vulkan and intentionally no-op.
OAK_RENDER_BACKEND_EXPORT void
oak_renderer_detach_output_texture(OakRenderBackendHandle handle)
{
	Q_UNUSED(handle)
	// Vulkan does not support OFX OpenGL render output attachment.
}
