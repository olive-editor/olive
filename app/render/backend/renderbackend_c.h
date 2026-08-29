#ifndef RENDERBACKEND_C_H
#define RENDERBACKEND_C_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#define OAK_RENDER_BACKEND_EXPORT extern "C" __declspec(dllexport)
#else
#define OAK_RENDER_BACKEND_EXPORT \
	extern "C" __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque pointer to the backend-owned C++ renderer object. */
typedef void *OakRenderBackendHandle;

/* Identifies the concrete backend behind a dynamically loaded library. */
enum OakRenderBackendKind {
	OAK_RENDER_BACKEND_UNKNOWN = 0,
	OAK_RENDER_BACKEND_OPENGL = 1,
	OAK_RENDER_BACKEND_VULKAN = 2
};

/* Capability bits advertised by a backend through oak_renderer_get_info(). */
enum OakRenderBackendCapability {
	OAK_RENDER_BACKEND_CAP_TEXTURES = 1ULL << 0,
	OAK_RENDER_BACKEND_CAP_SHADERS = 1ULL << 1,
	OAK_RENDER_BACKEND_CAP_BLIT = 1ULL << 2,
	OAK_RENDER_BACKEND_CAP_READBACK = 1ULL << 3,
	OAK_RENDER_BACKEND_CAP_VIEWER_CONTEXT = 1ULL << 4,
	OAK_RENDER_BACKEND_CAP_INSTANCE = 1ULL << 5,
	OAK_RENDER_BACKEND_CAP_DEVICE = 1ULL << 6
};

/* Static and runtime metadata returned by the backend. */
struct OakRenderBackendInfo {
	uint32_t abi_version;
	uint32_t kind;
	uint64_t capabilities;
	const char *name;
	const char *status;
};

/* Creates a backend renderer object. */
typedef OakRenderBackendHandle (*OakBackendCreateFn)(void *parent);
/* Destroys a backend renderer object created by OakBackendCreateFn. */
typedef void (*OakBackendDestroyFn)(OakRenderBackendHandle handle);
/* Queries backend metadata and capability bits. */
typedef bool (*OakBackendGetInfoFn)(OakRenderBackendHandle handle,
									struct OakRenderBackendInfo *out_info);
/* Checks whether the backend can run on the current machine. */
typedef bool (*OakBackendIsAvailableFn)(OakRenderBackendHandle handle);
/* Initializes backend-owned device/context resources. */
typedef bool (*OakBackendInitFn)(OakRenderBackendHandle handle);
/* Initializes the backend against a caller-supplied GL context when applicable. */
typedef void (*OakBackendInitWithContextFn)(OakRenderBackendHandle handle,
											void *context);
/* Runs backend post-initialization after the device/context exists. */
typedef void (*OakBackendPostInitFn)(OakRenderBackendHandle handle);
/* Runs backend post-destroy cleanup before the library unloads. */
typedef void (*OakBackendPostDestroyFn)(OakRenderBackendHandle handle);
/* Destroys renderer-owned native resources. */
typedef void (*OakBackendDestroyInternalFn)(OakRenderBackendHandle handle);
/* Clears a texture destination or implicit output target. */
typedef void (*OakBackendClearDestinationFn)(OakRenderBackendHandle handle,
											 void *texture, double r, double g,
											 double b, double a);
/* Creates a native texture and writes a QVariant-compatible handle. */
typedef void (*OakBackendCreateNativeTextureFn)(
	OakRenderBackendHandle handle, int width, int height, int depth, int format,
	int channel_count, const void *data, int linesize, void *out_variant);
/* Destroys a native texture represented by a QVariant-compatible handle. */
typedef void (*OakBackendDestroyNativeTextureFn)(OakRenderBackendHandle handle,
												 const void *variant);
/* Creates a native shader and writes a QVariant-compatible handle. */
typedef void (*OakBackendCreateNativeShaderFn)(OakRenderBackendHandle handle,
											   const void *shader_code,
											   void *out_variant);
/* Destroys a native shader represented by a QVariant-compatible handle. */
typedef void (*OakBackendDestroyNativeShaderFn)(OakRenderBackendHandle handle,
												const void *variant);
/* Uploads CPU pixel data to a native texture. */
typedef void (*OakBackendUploadToTextureFn)(OakRenderBackendHandle handle,
											const void *variant,
											const void *video_params,
											const void *data, int linesize);
/* Downloads native texture pixels into caller-owned CPU memory. */
typedef void (*OakBackendDownloadFromTextureFn)(OakRenderBackendHandle handle,
												const void *variant,
												const void *video_params,
												void *data, int linesize);
/* Waits for backend work that must be visible to later operations. */
typedef void (*OakBackendFlushFn)(OakRenderBackendHandle handle);
/* Reads one pixel from a texture. */
typedef void (*OakBackendGetPixelFromTextureFn)(OakRenderBackendHandle handle,
												void *texture,
												const void *point,
												void *out_color);
/* Executes a shader blit job. */
typedef void (*OakBackendBlitFn)(OakRenderBackendHandle handle,
								 const void *shader, void *job,
								 void *destination,
								 const void *destination_params,
								 bool clear_destination);
/* Attaches an output texture for OFX OpenGL rendering when supported. */
typedef void (*OakBackendAttachOutputTextureFn)(OakRenderBackendHandle handle,
												const void *texture_id);
/* Detaches an OFX output texture when supported. */
typedef void (*OakBackendDetachOutputTextureFn)(OakRenderBackendHandle handle);
/* Returns the backend OpenGL context, or null for non-OpenGL backends. */
typedef void *(*OakBackendOpenGLContextFn)(OakRenderBackendHandle handle);

#ifdef __cplusplus
}
#endif

#endif // RENDERBACKEND_C_H
