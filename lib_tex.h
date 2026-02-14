/**
@file lib_tex.h
@brief Texture Management Library for Game Engines
@version 0.1
@date June 22, 2025

@section Overview
lib_tex.h is a standalone texture management library designed for game engines and rendering applications.
It provides a high-level, cross-platform API for loading, managing, and rendering textures, with support for modern OpenGL (4.6) and Vulkan (1.1+) backends. The library is intended to be used alongside
platform abstraction libraries like situation.h, inheriting and extending texture-related functionality from existing systems (e.g., rgl.h).

This library serves as a modular component, allowing developers to manage textures independently of core rendering systems while providing seamless integration with situation.h for virtual display compositing,
shader-based blend modes, and command-buffer-centric rendering workflows.

@section Purpose
The primary purpose of lib_tex.h is to address the need for a robust, flexible texture management system that:
- Simplifies texture loading, creation, and manipulation for game developers.
- Supports advanced features like texture streaming, atlases, and render-to-texture workflows.
- Integrates with situation.h to enhance its rendering capabilities (e.g., virtual display textures).
- Consolidates texture-related functionality from rgl.h and situation.h into a unified, reusable module.
- Maintains performance and modularity, allowing use in both small and large projects.

@section Key Features
- Texture Loading: Load textures from common formats (PNG, JPEG, DDS, KTX) with customizable parameters.
- Render Textures: Create and manage render-to-texture targets (FBOs in OpenGL, VkImages in Vulkan).
- Texture Streaming: Support for asynchronous texture streaming to manage large assets efficiently.
- Texture Atlases: Tools for generating and managing texture atlases for sprite-based rendering.
- Shader Integration: Bind textures to shaders or descriptor sets, respecting situation.h's "Shader Contract."
- Memory Management: Reference counting and VRAM budget tracking to prevent memory leaks and exhaustion.
- Cross-Platform: Compatible with Windows, Linux, and macOS, with Windows-specific optimizations (e.g., DXGI interop).
- Error Handling: Robust error reporting integrated with situation.h's SituationError system.

@section Design Principles
- Modularity: Operates as a standalone library, optionally included in situation.h or other rendering systems.
- Performance: Optimized for modern GPUs, leveraging OpenGL 4.6 and Vulkan features (e.g., compressed textures, async loading).
- Abstraction: Hides backend-specific details (OpenGL textures vs. Vulkan images) behind a unified API.
- Extensibility: Designed to support future features like texture arrays, cubemaps, and procedural textures.
- Consistency: Aligns with situation.h's API conventions, error handling, and command buffer workflows.

@section Usage Models
A) Header-Only: Define LT_IMPLEMENTATION in one .c/.cpp file before including lib_tex.h.
B) Shared Library: Compile a separate file with LT_IMPLEMENTATION defined to create a .dll/.so, then link against it.

@section Dependencies
- Required: cglm (for vector/matrix operations), stb_image (for texture loading).
- OpenGL Backend: GLAD (generated for OpenGL 4.6 Core Profile).
- Vulkan Backend: Vulkan SDK headers, Vulkan Memory Allocator (VMA).
- Optional: situation.h (for integration with virtual displays and command buffers).
- Windows APIs (optional): Dxgi.lib (for texture format interop).

@section Scope and Limitations
- Focuses on texture management, not general rendering or scene management.
- Vulkan backend is a structural placeholder, with OpenGL 4.6 as the primary, stable backend.
- Advanced features like texture compression and atlas generation are planned but may be limited in the initial release.
- Does not include high-level resource management (e.g., scene graphs); users must manage texture lifecycles.

@section Integration with situation.h
lib_tex.h is designed to integrate with situation.h by:
- Providing texture resources for virtual display compositing (e.g., FBO textures).
- Supporting situation.h's command buffer API for texture binding and rendering.
- Reusing situation.h's filesystem utilities and error handling for consistency.
- Allowing situation.h to include lib_tex.h conditionally for internal texture needs.

@section Integration with rgl.h
The following rgl.h texture functions are migrated and extended in lib_tex.h:
- RGL_CreateRenderTexture -> LTCreateRenderTexture
- RGL_DestroyRenderTexture -> LTDestroyTexture
- RGL_SetRenderTarget -> LTSetRenderTarget
- RGL_ResetRenderTarget -> LTResetRenderTarget
- RGL_LoadTextureWithParams -> LTLoadTextureWithParams
- RGL_LoadTexture -> LTLoadTexture
- RGL_UnloadTexture -> LTDestroyTexture

@section Example Usage
```c
#define LT_IMPLEMENTATION
#include "lib_tex.h"
#include <stdio.h>

int main() {
  LTInitInfo init_info = { .renderer_type = LT_RENDERER_OPENGL };
  if (LTInit(&init_info) != LT_SUCCESS) {
      printf("Failed to initialize lib_tex: %s\n", LTGetLastErrorMsg());
      return -1;
  }

  LTTexture texture = LTLoadTexture("assets/texture.png", LT_WRAP_REPEAT, LT_FILTER_LINEAR);
  if (texture.id == 0) {
      printf("Failed to load texture: %s\n", LTGetLastErrorMsg());
  } else {
      // Bind texture to a shader (example with situation.h)
      // SituationCommandBuffer cmd = SituationGetMainCommandBuffer();
      // LTCmdBindTexture(cmd, texture, 0);

      // Render with texture...
      printf("Loaded texture '%s' with ID %u\n", texture.name, texture.id);

      LTDestroyTexture(&texture);
  }

  LTShutdown();
  return 0;
}
*/

#ifndef LT_H
#define LT_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
// Core dependencies
#include <cglm/cglm.h>
// situation.h integration (optional)
#ifdef SITUATION_H
    #include "situation.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Assumes stb_image.h is in the include path

#if defined(SITUATION_USE_OPENGL)
    #include <glad/glad.h>
#elif defined(SITUATION_USE_VULKAN)
    #include <vulkan/vulkan.h>
    #include "vk_mem_alloc.h"
#endif


// --- Configuration Defines ---
#define LT_MAX_TEXTURE_NAME_LEN 256
#define LT_MAX_ERROR_MSG_LEN 256
#define LT_MAX_TEXTURES 1024
#define LT_DEFAULT_VRAM_BUDGET_MB 2048

// --- Renderer Abstraction ---
typedef enum {
    LT_RENDERER_OPENGL,
    LT_RENDERER_VULKAN
} LTRendererType;

// --- Texture Format ---
typedef enum {
    LT_FORMAT_RGBA8, // 8-bit RGBA (default)
    LT_FORMAT_RGB8, // 8-bit RGB
    LT_FORMAT_BC7, // BC7 compressed (DX10+)
    LT_FORMAT_ASTC_4x4, // ASTC 4x4 compressed
    LT_FORMAT_R32F, // 32-bit float (single channel)
    LT_FORMAT_RGBA16F // 16-bit float RGBA
} LTTextureFormat;

// --- Texture Wrap Modes ---
typedef enum {
    LT_WRAP_CLAMP_TO_EDGE,
    LT_WRAP_REPEAT,
    LT_WRAP_MIRRORED_REPEAT
} LTWrapMode;

// --- Texture Filter Modes ---
typedef enum {
    LT_FILTER_NEAREST,
    LT_FILTER_LINEAR,
    LT_FILTER_NEAREST_MIPMAP_NEAREST,
    LT_FILTER_LINEAR_MIPMAP_LINEAR
} LTFilterMode;

// --- Texture Parameters ---
typedef struct {
    LTTextureFormat format; // Texture format (default: RGBA8)
    LTWrapMode wrap_s; // Wrap mode for S coordinate
    LTWrapMode wrap_t; // Wrap mode for T coordinate
    LTFilterMode filter_min; // Minification filter
    LTFilterMode filter_mag; // Magnification filter
    bool generate_mipmaps; // Generate mipmaps automatically
    int anisotropic_level; // Anisotropic filtering level (0 = disabled)
} LTTextureParams;

// --- Texture Handle ---
typedef struct {
    uint32_t id;                        // Unique identifier for the library's internal pool
    int width;                          // Texture width in pixels
    int height;                         // Texture height in pixels
    LTTextureFormat format;             // Texture format
    bool has_mipmaps; // NEW: Track if mipmaps have been generated
    char name[LT_MAX_TEXTURE_NAME_LEN]; // Optional name (e.g., filename)

#ifdef LT_IMPLEMENTATION
    // Internal backend-specific data, not for public use
    union {
        // OpenGL specific data
        struct {
            //GLuint gl_texture_id; // Use GLuint for clarity
            GLuint fbo_id;        // Framebuffer ID (for render textures)
            GLuint rbo_id;        // Depth renderbuffer ID (for render textures)
        } gl;
        // Vulkan specific data (using actual types)
#if defined(SITUATION_USE_VULKAN)
        struct {
            VkImage vk_image;
            VkDeviceMemory vk_memory;
            VkImageView vk_image_view;
            VkSampler vk_sampler;
            VkFramebuffer vk_framebuffer; // For render textures
        } vk;
#else // Fallback for when Vulkan headers aren't included but the struct needs a definition
        struct { void* _[5]; } vk;
#endif
    } backend;
#endif
} LTTexture;

// A helper struct to hold all internal library state.
// This avoids polluting the global namespace.
typedef struct {
    bool is_initialized;
    LTRendererType renderer_type;
    char last_error_msg[LT_MAX_ERROR_MSG_LEN];

    // Texture Pool
    LTTexture textures[LT_MAX_TEXTURES];
    bool texture_slots_used[LT_MAX_TEXTURES];
    int ref_counts[LT_MAX_TEXTURES]; // For reference counting
    int active_texture_count;

    // Memory Management
    uint64_t vram_usage_bytes;
    uint64_t vram_budget_bytes;
    bool ref_counting_enabled;

    // Async Operations (requires mutex)
    // ma_mutex texture_pool_mutex; // Example using miniaudio's threading primitives
} LT_State;

// The single, static instance of our library's state.
static LT_State lt_state;

// --- Error Handling ---
typedef enum {
    LT_SUCCESS = 0,
    LT_ERROR_GENERAL,
    LT_ERROR_INIT_FAILED,
    LT_ERROR_NOT_INITIALIZED,
    LT_ERROR_INVALID_PARAM,
    LT_ERROR_SHUTDOWN_FAILED,
    LT_ERROR_MEMORY_ALLOCATION,
    LT_ERROR_TEXTURE_LOAD_FAILED,
    LT_ERROR_TEXTURE_FORMAT_UNSUPPORTED,
    LT_ERROR_TEXTURE_OUT_OF_MEMORY,
    LT_ERROR_TEXTURE_LIMIT_REACHED,
    LT_ERROR_RENDER_TARGET_INVALID
} LTError;

// --- Initialization Info ---
typedef struct {
    LTRendererType renderer_type; // OpenGL or Vulkan
    uint64_t vram_budget_mb; // VRAM budget in megabytes (0 = use default)
    bool enable_async_loading; // Enable asynchronous texture loading
    const char** required_vulkan_extensions; // Vulkan-specific extensions
    uint32_t required_vulkan_extension_count;
} LTInitInfo;

// --- API Declaration Control ---
#if defined(_WIN32)
    #if defined(LT_BUILD_SHARED)
        #define LTAPI __declspec(dllexport)
    #elif defined(LT_USE_SHARED)
        #define LTAPI __declspec(dllimport)
    #else
        #define LTAPI
    #endif
#else // Non-Windows platforms
    #if defined(LT_BUILD_SHARED)
        #define LTAPI __attribute__((visibility("default")))
    #else
        #define LTAPI
    #endif
#endif

// --- API Declarations ---

// Core Lifecycle & Error Handling
LTAPI LTError LTInit(const LTInitInfo* init_info);
LTAPI void LTShutdown(void);
LTAPI const char* LTGetLastErrorMsg(void);
LTAPI bool LTIsInitialized(void);
LTAPI LTRendererType LTGetRendererType(void);

// Texture Creation and Management
LTAPI LTTexture LTCreateRenderTexture(int width, int height, LTTextureFormat format);
LTAPI LTTexture LTLoadTexture(const char* filename, LTWrapMode wrap_mode, LTFilterMode filter_mode);
LTAPI LTTexture LTLoadTextureWithParams(const char* filename, const LTTextureParams* params);
LTAPI LTTexture LTCreateTextureFromMemory(const void* data, int width, int height, const LTTextureParams* params);
LTAPI void LTDestroyTexture(LTTexture* texture);
LTAPI LTError LTUpdateTexture(LTTexture texture, const void* data, int x, int y, int width, int height);

// Render Target Management
LTAPI void LTSetRenderTarget(LTTexture texture); // Pass a texture with valid FBO/VkFramebuffer
LTAPI void LTResetRenderTarget(void); // Revert to default framebuffer (screen)

// Texture Binding for Rendering
#ifdef SITUATION_H
LTAPI void LTCmdBindTexture(SituationCommandBuffer cmd, LTTexture texture, uint32_t binding_slot);
#endif
LTAPI void LTBindTexture(LTTexture texture, uint32_t binding_slot); // Direct binding (non-command-buffer)

// Texture Streaming
LTAPI LTError LTStreamTextureAsync(const char* filename, LTTexture* out_texture, void (callback)(LTTexture, void), void* user_data);
LTAPI LTError LTCancelStream(LTTexture texture);

// Texture Atlas Management
LTAPI LTTexture LTCreateTextureAtlas(const char** filenames, int count, int max_width, int max_height, vec4* out_uv_coords);
LTAPI LTError LTAddToAtlas(LTTexture atlas, const char* filename, vec4* out_uv_coords);

// Memory Management
LTAPI uint64_t LTGetVRAMUsageBytes(void);
LTAPI LTError LTSetVRAMBudget(uint64_t budget_bytes);
LTAPI void LTEnableReferenceCounting(bool enable);
LTAPI int LTGetTextureReferenceCount(LTTexture texture);

// Utility Functions
LTAPI LTError LTGenerateMipmaps(LTTexture texture);
LTAPI LTError LTSetTextureParams(LTTexture texture, const LTTextureParams* params);
LTAPI void LTGetTextureSize(LTTexture texture, int* width, int* height);
LTAPI LTTextureFormat LTGetTextureFormat(LTTexture texture);

#ifdef LT_IMPLEMENTATION
// ============================================================================
//
//                          IMPLEMENTATION
//
// ============================================================================

static void _LTSetError(LTError error, const char* msg);
static void _LTCleanupTextureBackendData(LTTexture* texture);

static bool _LTConvertFormatToGL(LTTextureFormat lt_format, GLint* out_internal_format, GLenum* out_format, GLenum* out_type);
static int _LTGetBytesPerPixel(LTTextureFormat format);
static GLenum _LTConvertWrapToGL(LTWrapMode mode);
static GLenum _LTConvertFilterToGL(LTFilterMode mode);
static uint64_t _LTGetTextureSizeBytes(LTTextureFormat format, int width, int height, bool has_mipmaps);

// --- Internal Helper Functions ---

// Internal helper to set the last error message and code.
static void _LTSetError(LTError error, const char* msg) {
    if (msg) {
        strncpy(lt_state.last_error_msg, msg, LT_MAX_ERROR_MSG_LEN - 1);
        lt_state.last_error_msg[LT_MAX_ERROR_MSG_LEN - 1] = '\0';
    } else {
        // Provide a default message if NULL is passed.
        switch (error) {
            case LT_ERROR_INIT_FAILED: strcpy(lt_state.last_error_msg, "Initialization failed"); break;
            case LT_ERROR_INVALID_PARAM: strcpy(lt_state.last_error_msg, "Invalid parameter provided"); break;
            // Add more default messages as needed
            default: strcpy(lt_state.last_error_msg, "An unknown error occurred"); break;
        }
    }
}

// Internal helper to clean up the backend-specific data for a single texture.
static void _LTCleanupTextureBackendData(LTTexture* texture) {
    if (!texture || texture->id == 0) return;

#if defined(SITUATION_USE_OPENGL)
    if (lt_state.renderer_type == LT_RENDERER_OPENGL) {
        if (texture->backend.gl.gl_texture_id > 0) { glDeleteTextures(1, &texture->backend.gl.gl_texture_id); }
        if (texture->backend.gl.fbo_id > 0) { glDeleteFramebuffers(1, &texture->backend.gl.fbo_id); }
        if (texture->backend.gl.rbo_id > 0) { glDeleteRenderbuffers(1, &texture->backend.gl.rbo_id); }
    }
#elif defined(SITUATION_USE_VULKAN)
    if (lt_state.renderer_type == LT_RENDERER_VULKAN) {
        // TODO: Implement Vulkan resource cleanup
        // This would involve calls like:
        // vkDestroySampler(device, texture->backend.vk.vk_sampler, NULL);
        // vkDestroyImageView(device, texture->backend.vk.vk_image_view, NULL);
        // vkDestroyImage(device, texture->backend.vk.vk_image, NULL);
        // vmaFreeMemory(allocator, texture->backend.vk.vk_memory);
    }
#endif
}

static uint64_t _LTGetTextureSizeBytes(LTTextureFormat format, int width, int height, bool has_mipmaps) {
    // First, handle uncompressed formats using our existing helper
    int bpp = _LTGetBytesPerPixel(format);
    if (bpp > 0) {
        uint64_t base_size = (uint64_t)width * (uint64_t)height * bpp;
        // Mipmaps add roughly 33% to the total size.
        return has_mipmaps ? (uint64_t)(base_size * 1.33334f) : base_size;
    }

    // Next, handle compressed formats
    // Note: requires dimensions to be multiple of 4. A robust impl would pad.
    uint64_t num_blocks = (uint64_t)((width + 3) / 4) * ((height + 3) / 4);
    uint64_t size_in_bytes = 0;

    switch (format) {
        // BC7 uses 16 bytes per 4x4 block (128 bits)
        case LT_FORMAT_BC7:
            size_in_bytes = num_blocks * 16;
            break;
        // ASTC 4x4 also uses 16 bytes per 4x4 block (128 bits)
        case LT_FORMAT_ASTC_4x4:
            size_in_bytes = num_blocks * 16;
            break;
        default:
            return 0;
    }

    return has_mipmaps ? (uint64_t)(size_in_bytes * 1.33334f) : size_in_bytes;
}

// --- API Implementation: Core Lifecycle & Error Handling ---

/**
 * @brief Initializes the texture library.
 *
 * This function must be called before any other library function. It sets up
 * internal state and verifies the rendering environment. The user is responsible
 * for creating a valid OpenGL or Vulkan context and loading the necessary function
 * pointers (e.g., via GLAD) BEFORE calling this function.
 *
 * @param init_info A pointer to the initialization settings.
 * @return LT_SUCCESS on success, or an error code on failure.
 */
LTAPI LTError LTInit(const LTInitInfo* init_info) {
    if (lt_state.is_initialized) { _LTSetError(LT_ERROR_INIT_FAILED, "lib_tex is already initialized."); return LT_ERROR_INIT_FAILED; }
    if (!init_info) { _LTSetError(LT_ERROR_INVALID_PARAM, "LTInitInfo cannot be NULL."); return LT_ERROR_INVALID_PARAM; }

    // Corrected memset
    memset(&LT_state, 0, sizeof(LT_State));

    // Conditional check for situation.h
    #ifdef SITUATION_H
    if (!SituationIsInitialized()) { _LTSetError(LT_ERROR_INIT_FAILED, "Underlying graphics context (situation.h) is not initialized."); return LT_ERROR_INIT_FAILED; }
    if (init_info->renderer_type != (LTRendererType)SituationGetRendererType()) { _LTSetError(LT_ERROR_INIT_FAILED, "Renderer type mismatch between lib_tex and situation.h."); return LT_ERROR_INIT_FAILED; }
    #else
    // If situation.h is not used, we assume the user has created a graphics context.
    // A more advanced version might take a context handle as a parameter.
    #endif

    // Copy initialization settings to our internal state.
    lt_state.renderer_type = init_info->renderer_type;
    lt_state.ref_counting_enabled = false; // Default to off

    // Set VRAM budget, using default if none is provided.
    if (init_info->vram_budget_mb > 0) {
        lt_state.vram_budget_bytes = init_info->vram_budget_mb * 1024 * 1024;
    } else {
        lt_state.vram_budget_bytes = (uint64_t)LT_DEFAULT_VRAM_BUDGET_MB * 1024 * 1024;
    }

    // TODO: Initialize threading primitives if async loading is enabled.
    // if (init_info->enable_async_loading) {
    //     ma_mutex_init(&LT_state.texture_pool_mutex);
    // }

    // Mark as initialized and return success.
    lt_state.is_initialized = true;
    _LTSetError(LT_SUCCESS, "lib_tex initialized successfully.");
    return LT_SUCCESS;
}

LTAPI void LTShutdown(void) {
    if (!lt_state.is_initialized) {
        return; // Not an error to shut down a non-initialized library.
    }

    // Force-destroy any textures that the user may have leaked.
    int leaked_count = 0;
    for (int i = 0; i < LT_MAX_TEXTURES; i++) {
        if (lt_state.texture_slots_used[i]) {
            leaked_count++;
            _LTCleanupTextureBackendData(&LT_state.textures[i]);
        }
    }

#ifndef NDEBUG
    if (leaked_count > 0) {
        fprintf(stderr, "[lib_tex] Warning: %d texture(s) were not destroyed before shutdown. Forcing cleanup.\n", leaked_count);
    }
#endif

    // TODO: Clean up threading primitives if they were initialized.
    // ma_mutex_uninit(&LT_state.texture_pool_mutex);

    // Finally, zero-out the entire state struct to reset it.
    memset(&LT_state, 0, sizeof(LT_State));
}

/**
 * @brief Retrieves the last error message.
 * @return A pointer to a static, internal error message string. Do not free this pointer.
 *         The message is valid until the next API call that sets an error.
 */
LTAPI const char* LTGetLastErrorMsg(void) {
    return lt_state.last_error_msg;
}

LTAPI bool LTIsInitialized(void) {
    return lt_state.is_initialized;
}

LTAPI LTRendererType LTGetRendererType(void) {
    if (!lt_state.is_initialized) {
        // Return a sensible default, though behavior is undefined if not initialized.
        return LT_RENDERER_OPENGL;
    }
    return lt_state.renderer_type;
}

/**
 * @brief Converts an LTTextureFormat enum to the corresponding OpenGL enums.
 * @param lt_format The lib_tex format enum.
 * @param out_internal_format Pointer to store the OpenGL internal format (e.g., GL_RGBA8).
 * @param out_format Pointer to store the OpenGL data format (e.g., GL_RGBA).
 * @param out_type Pointer to store the OpenGL data type (e.g., GL_UNSIGNED_BYTE).
 * @return true if the format is supported, false otherwise.
 */
static bool _LTConvertFormatToGL(LTTextureFormat lt_format, GLint* out_internal_format, GLenum* out_format, GLenum* out_type) {
    switch (lt_format) {
        case LT_FORMAT_RGBA8:
            *out_internal_format = GL_RGBA8;
            *out_format = GL_RGBA;
            *out_type = GL_UNSIGNED_BYTE;
            return true;
        case LT_FORMAT_RGB8:
            *out_internal_format = GL_RGB8;
            *out_format = GL_RGB;
            *out_type = GL_UNSIGNED_BYTE;
            return true;
        case LT_FORMAT_R32F:
            *out_internal_format = GL_R32F;
            *out_format = GL_RED;
            *out_type = GL_FLOAT;
            return true;
        case LT_FORMAT_RGBA16F:
            *out_internal_format = GL_RGBA16F;
            *out_format = GL_RGBA;
            *out_type = GL_HALF_FLOAT;
            return true;
        // Compressed formats are not valid for uninitialized render targets.
        case LT_FORMAT_BC7:
        case LT_FORMAT_ASTC_4x4:
        default:
            return false;
    }
}

/**
 * @brief Calculates the number of bytes per pixel for a given uncompressed format.
 * @param format The lib_tex format enum.
 * @return The number of bytes per pixel, or 0 for unsupported/compressed formats.
 */
static int _LTGetBytesPerPixel(LTTextureFormat format) {
    switch (format) {
        case LT_FORMAT_RGB8:    return 3;
        case LT_FORMAT_RGBA8:   return 4;
        case LT_FORMAT_R32F:    return 4;
        case LT_FORMAT_RGBA16F: return 8; // 16 bits * 4 channels = 64 bits = 8 bytes
        default:                return 0;
    }
}


// --- API Implementation: Texture Creation and Management ---

LTAPI LTTexture LTCreateRenderTexture(int width, int height, LTTextureFormat format) {
    LTTexture texture = {0}; // Always return a zeroed handle on failure.

    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return texture; }
    if (width <= 0 || height <= 0) { _LTSetError(LT_ERROR_INVALID_PARAM, "Render texture dimensions must be positive."); return texture; }
    if (lt_state.active_texture_count >= LT_MAX_TEXTURES) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "Maximum number of textures reached."); return texture; }

    // --- Find a free slot in our texture pool ---
    int texture_idx = -1;
    for (int i = 1; i < LT_MAX_TEXTURES; i++) { // Start at 1, so ID 0 is always invalid.
        if (!lt_state.texture_slots_used[i]) { texture_idx = i; break; }
    }
    // This case should be caught by the active_texture_count check, but is a good safeguard.
    if (texture_idx == -1) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "No available texture slots."); return texture; }

    // --- Convert our format to OpenGL types ---
    GLint gl_internal_format;
    GLenum gl_format, gl_type;
    if (!_LTConvertFormatToGL(format, &gl_internal_format, &gl_format, &gl_type)) { _LTSetError(LT_ERROR_TEXTURE_FORMAT_UNSUPPORTED, "The specified format cannot be used for a render texture."); return texture; }

    // Get a pointer to the texture we will build in the pool.
    LTTexture* new_tex = &LT_state.textures[texture_idx];
    memset(new_tex, 0, sizeof(LTTexture));

    // --- Create OpenGL Objects (FBO, Color Texture, Depth RBO) ---
    glGenFramebuffers(1, &new_tex->backend.gl.fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, new_tex->backend.gl.fbo_id);

    glGenTextures(1, &new_tex->backend.gl.gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, new_tex->backend.gl.gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, gl_type, NULL);

    // Set sensible default filtering and wrapping for render targets.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, new_tex->backend.gl.gl_texture_id, 0);

    // Create a depth/stencil renderbuffer. GL_DEPTH24_STENCIL8 is a robust default.
    glGenRenderbuffers(1, &new_tex->backend.gl.rbo_id);
    glBindRenderbuffer(GL_RENDERBUFFER, new_tex->backend.gl.rbo_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, new_tex->backend.gl.rbo_id);

    // --- Check for FBO Completeness (CRITICAL STEP) ---
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        _LTSetError(LT_ERROR_RENDER_TARGET_INVALID, "Framebuffer is not complete. Check format support and GPU limits.");
        _LTCleanupTextureBackendData(new_tex); // Cleanup the partially created GL objects.
        memset(new_tex, 0, sizeof(LTTexture)); // Zero out the struct in the pool.
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind to be safe.
        return texture; // Return the empty handle.
    }

    // --- Success! Finalize the texture struct and state ---
    new_tex->id = texture_idx;
    new_tex->width = width;
    new_tex->height = height;
    new_tex->format = format;
    snprintf(new_tex->name, LT_MAX_TEXTURE_NAME_LEN, "RenderTexture_%d", new_tex->id);

    lt_state.texture_slots_used[texture_idx] = true;
    lt_state.active_texture_count++;
    lt_state.ref_counts[texture_idx] = 1;

    // --- VRAM Tracking ---
    // FIX: Remove the old, redundant VRAM calculation. Use only the helper.
    uint64_t color_size = _LTGetTextureSizeBytes(format, width, height, false);
    // Depth buffer is always DEPTH24_STENCIL8 = 4 bytes per pixel
    uint64_t depth_size = (uint64_t)width * (uint64_t)height * 4;
    lt_state.vram_usage_bytes += color_size + depth_size;
    // Note: This doesn't account for the depth buffer, but is a good start.
    // Unbind everything to return to a clean state (main framebuffer).
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return *new_tex; // Return a copy of the newly created texture from the pool.
}

LTAPI void LTDestroyTexture(LTTexture* texture) {
    // Silently fail if not initialized, as there's nothing to destroy.
    if (!lt_state.is_initialized) { return; }

    // Invalid handle passed.
    if (!texture || texture->id == 0 || texture->id >= LT_MAX_TEXTURES) { return; }

    // This slot is already free, nothing to do. Zero out the user's handle just in case it's stale.
    if (!lt_state.texture_slots_used[texture->id]) { memset(texture, 0, sizeof(LTTexture)); return; }

    // --- Handle Reference Counting ---
    if (lt_state.ref_counting_enabled) {
        lt_state.ref_counts[texture->id]--;
        if (lt_state.ref_counts[texture->id] > 0) {
            // Other references still exist, so just zero the user's handle but don't destroy the resource yet.
            memset(texture, 0, sizeof(LTTexture));
            return;
        }
    }

    // --- Retrieve the actual texture from our internal pool ---
    LTTexture* tex_to_destroy = &LT_state.textures[texture->id];

    // --- Update VRAM usage ---
    uint64_t texture_size = _LTGetTextureSizeBytes(tex_to_destroy->format, tex_to_destroy->width, tex_to_destroy->height, tex_to_destroy->has_mipmaps); 
    // Check if it was a render texture by seeing if it has an FBO
    if (tex_to_destroy->backend.gl.fbo_id > 0) {
        // Add the size of the depth buffer
        texture_size += (uint64_t)tex_to_destroy->width * (uint64_t)tex_to_destroy->height * 4;
    }
    if (lt_state.vram_usage_bytes >= texture_size) {
        lt_state.vram_usage_bytes -= texture_size;
    } else {
        lt_state.vram_usage_bytes = 0; // Avoid underflow
    }
    // TODO: Add logic for compressed texture size calculation.

    // --- Clean up the backend-specific GPU resources ---
    // This uses the same internal helper as LTShutdown.
    _LTCleanupTextureBackendData(tex_to_destroy);

    // --- Free the slot in the resource pool ---
    lt_state.texture_slots_used[texture->id] = false;
    lt_state.active_texture_count--;
    lt_state.ref_counts[texture->id] = 0; // Reset ref count

    // --- Zero out the user's handle to prevent reuse ---
    memset(texture, 0, sizeof(LTTexture));
}

LTAPI LTError LTUpdateTexture(LTTexture texture, const void* data, int x, int y, int width, int height) {
    if (!lt_state.is_initialized) return LT_ERROR_NOT_INITIALIZED;
    if (texture.id == 0 || !lt_state.texture_slots_used[texture.id]) return LT_ERROR_INVALID_PARAM;
    if (!data || x < 0 || y < 0 || width <= 0 || height <= 0) return LT_ERROR_INVALID_PARAM;
    if ((x + width > texture.width) || (y + height > texture.height)) return LT_ERROR_INVALID_PARAM; // Bounds check

    GLint unused_internal_format;
    GLenum gl_format, gl_type;
    if (!_LTConvertFormatToGL(texture.format, &unused_internal_format, &gl_format, &gl_type)) { return LT_ERROR_TEXTURE_FORMAT_UNSUPPORTED; }

    glBindTexture(GL_TEXTURE_2D, texture.backend.gl.gl_texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, gl_format, gl_type, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return LT_SUCCESS;
}

/**
 * @brief Sets the current rendering target to a specified texture.
 *
 * All subsequent drawing operations will be directed to this texture instead of the screen.
 * It is the CALLER'S RESPONSIBILITY to flush any pending rendering commands (e.g., from a sprite batcher) BEFORE calling this function.
 *
 * @param texture The texture to set as the target. It must have been created with LTCreateRenderTexture, otherwise this call will have no effect.
 */
LTAPI void LTSetRenderTarget(LTTexture texture) {
    if (!lt_state.is_initialized) { return; }
    // Validate that the provided texture is actually a render target.
    // A regular texture will have a fbo_id of 0.
#if defined(SITUATION_USE_OPENGL)
    if (texture.backend.gl.fbo_id == 0) { _LTSetError(LT_ERROR_RENDER_TARGET_INVALID, "Attempted to set a non-render-target texture as the render target."); return; }
    // Bind the framebuffer. This redirects all subsequent drawing.
    glBindFramebuffer(GL_FRAMEBUFFER, texture.backend.gl.fbo_id);
    // The viewport should also be set to match the texture's dimensions.
    glViewport(0, 0, texture.width, texture.height);

#elif defined(SITUATION_USE_VULKAN)
    // TODO: Vulkan implementation would begin a new render pass targeting the texture's framebuffer.
    // This is a much more involved process than in OpenGL.
#endif
}

/**
 * @brief Resets the rendering target back to the main window/screen.
 *
 * It is the CALLER'S RESPONSIBILITY to flush any pending rendering commands (e.g.,
 * from a sprite batcher) BEFORE calling this function.
 */
LTAPI void LTResetRenderTarget(void) {
    if (!lt_state.is_initialized) { return; }

#if defined(SITUATION_USE_OPENGL)
    // Bind the default framebuffer (ID 0), which corresponds to the main window.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // If integrated with situation.h, we can automatically reset the viewport to the main window's size. Otherwise, the user is responsible for this.
    #ifdef SITUATION_H
    int width, height;
    SituationGetWindowSize(&width, &height);
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
    #endif

#elif defined(SITUATION_USE_VULKAN)
    // TODO: Vulkan implementation would end the current render pass. The main render loop would then begin the next render pass on the swapchain image.
#endif
}

#ifdef SITUATION_H
LTAPI void LTCmdBindTexture(SituationCommandBuffer cmd, LTTexture texture, uint32_t binding_slot) {
    // In OpenGL, command buffers are conceptual. We bind immediately.
    // The 'cmd' parameter is unused but maintains API compatibility.
    (void)cmd; // Mark as unused to prevent compiler warnings
    if (!lt_state.is_initialized || texture.id == 0 || !lt_state.texture_slots_used[texture.id]) return;
    glActiveTexture(GL_TEXTURE0 + binding_slot);
    glBindTexture(GL_TEXTURE_2D, texture.backend.gl.gl_texture_id);
}
#endif

// For standalone use or direct binding
LTAPI void LTBindTexture(LTTexture texture, uint32_t binding_slot) {
    if (!lt_state.is_initialized || texture.id == 0 || !lt_state.texture_slots_used[texture.id]) return;
    glActiveTexture(GL_TEXTURE0 + binding_slot);
    glBindTexture(GL_TEXTURE_2D, texture.backend.gl.gl_texture_id);
}

/**
 * @brief Converts an LTWrapMode enum to the corresponding OpenGL enum.
 */
static GLenum _LTConvertWrapToGL(LTWrapMode mode) {
    switch (mode) {
        case LT_WRAP_REPEAT:           return GL_REPEAT;
        case LT_WRAP_MIRRORED_REPEAT:  return GL_MIRRORED_REPEAT;
        case LT_WRAP_CLAMP_TO_EDGE:
        default:                       return GL_CLAMP_TO_EDGE;
    }
}

/**
 * @brief Converts an LTFilterMode enum to the corresponding OpenGL enum.
 */
static GLenum _LTConvertFilterToGL(LTFilterMode mode) {
    switch (mode) {
        case LT_FILTER_LINEAR:                  return GL_LINEAR;
        case LT_FILTER_NEAREST_MIPMAP_NEAREST:  return GL_NEAREST_MIPMAP_NEAREST;
        case LT_FILTER_LINEAR_MIPMAP_LINEAR:    return GL_LINEAR_MIPMAP_LINEAR;
        case LT_FILTER_NEAREST:
        default:                                return GL_NEAREST;
    }
}

// --- API Implementation: Texture Loading ---

LTAPI LTTexture LTLoadTextureWithParams(const char* filename, const LTTextureParams* params) {
    LTTexture texture = {0};
    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return texture; }
    if (!filename) { _LTSetError(LT_ERROR_INVALID_PARAM, "Texture filename cannot be null."); return texture; }
    if (lt_state.active_texture_count >= LT_MAX_TEXTURES) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "Maximum number of textures reached."); return texture; }

    // --- Find a free slot in our texture pool ---
    int texture_idx = -1;
    for (int i = 1; i < LT_MAX_TEXTURES; i++) {
        if (!lt_state.texture_slots_used[i]) {
            texture_idx = i;
            break;
        }
    }
    if (texture_idx == -1) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "No available texture slots."); return texture; }

    // --- Load image data from file using stb_image ---
    stbi_set_flip_vertically_on_load(true); // Match OpenGL's coordinate system
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) { _LTSetError(LT_ERROR_TEXTURE_LOAD_FAILED, stbi_failure_reason()); return texture; }

    // --- Determine the desired format ---
    // Respect the format from params, fallback to RGBA8.
    LTTextureFormat lt_format = params ? params->format : LT_FORMAT_RGBA8;
    GLint gl_internal_format;
    GLenum gl_format, gl_type;
    if (!_LTConvertFormatToGL(lt_format, &gl_internal_format, &gl_format, &gl_type)) { _LTSetError(LT_ERROR_TEXTURE_FORMAT_UNSUPPORTED, "Unsupported texture format for loading."); return texture; }

    // Determine how many channels stb_image should request based on the format.
    int desired_channels = 0; // 0 = let stb_image decide, then we'll check
    if (lt_format == LT_FORMAT_RGB8) desired_channels = 3;
    if (lt_format == LT_FORMAT_RGBA8) desired_channels = 4;
    // Note: For R32F etc., we'd need to load as float, which is more complex (stbi_loadf).
    // For now, we'll focus on the common byte-based formats.

    // --- Load image data from file ---
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, desired_channels);
    if (!data) { _LTSetError(LT_ERROR_TEXTURE_LOAD_FAILED, stbi_failure_reason()); return texture; }
    
    // If we let stb_image decide, we need to set the final OpenGL format now
    if (desired_channels == 0) {
        if (nrChannels == 1)      { gl_internal_format = GL_R8;   gl_format = GL_RED;  lt_format = LT_FORMAT_RGBA8; }
        else if (nrChannels == 3) { gl_internal_format = GL_RGB8; gl_format = GL_RGB;  lt_format = LT_FORMAT_RGB8;  }
        else if (nrChannels == 4) { gl_internal_format = GL_RGBA8;gl_format = GL_RGBA; lt_format = LT_FORMAT_RGBA8; }
    }


    // --- Create and upload the texture to the GPU ---
    LTTexture* new_tex = <_state.textures[texture_idx]; // FIX: Correct pointer access
    memset(new_tex, 0, sizeof(LTTexture));

    glGenTextures(1, &new_tex->backend.gl.gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, new_tex->backend.gl.gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

    // --- Apply parameters ---
    if (params) {
        if (params->generate_mipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _LTConvertWrapToGL(params->wrap_s));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _LTConvertWrapToGL(params->wrap_t));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _LTConvertFilterToGL(params->filter_min));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _LTConvertFilterToGL(params->filter_mag));
        if (params->anisotropic_level > 0) {
            // Check for extension support before setting
            GLfloat max_anisotropy;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, (GLfloat)params->anisotropic_level > max_anisotropy ? max_anisotropy : (GLfloat)params->anisotropic_level);
        }
    } else {
        // Apply some sane defaults if no params are provided
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data); // Free CPU memory
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind

    // --- Success! Finalize the texture struct and state ---
    new_tex->id = texture_idx;
    new_tex->width = width;
    new_tex->height = height;
    new_tex->format = lt_format;
    strncpy(new_tex->name, filename, LT_MAX_TEXTURE_NAME_LEN - 1);
    new_tex->name[LT_MAX_TEXTURE_NAME_LEN - 1] = '\0';

    lt_state.texture_slots_used[texture_idx] = true;
    lt_state.active_texture_count++;
    lt_state.ref_counts[texture_idx] = 1;
    bool has_mips = params ? params->generate_mipmaps : false;
    lt_state.vram_usage_bytes += _LTGetTextureSizeBytes(lt_format, width, height, has_mips);

    _LTSetError(LT_SUCCESS, "Texture loaded successfully.");
    return *new_tex;
}

LTAPI LTTexture LTLoadTexture(const char* filename, LTWrapMode wrap_mode, LTFilterMode filter_mode) {
    LTTextureParams params = {
        .format = LT_FORMAT_RGBA8,
        .wrap_s = wrap_mode,
        .wrap_t = wrap_mode,
        .filter_min = filter_mode,
        .filter_mag = (filter_mode == LT_FILTER_NEAREST || filter_mode == LT_FILTER_NEAREST_MIPMAP_NEAREST) ? LT_FILTER_NEAREST : LT_FILTER_LINEAR,
        .generate_mipmaps = (filter_mode == LT_FILTER_LINEAR_MIPMAP_LINEAR || filter_mode == LT_FILTER_NEAREST_MIPMAP_NEAREST),
        .anisotropic_level = 0
    };
    return LTLoadTextureWithParams(filename, ¶ms); // FIX: Corrected identifier
}


LTAPI LTTexture LTCreateTextureFromMemory(const void* data, int width, int height, const LTTextureParams* params) {
    LTTexture texture = {0};

    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return texture; }
    if (!data || width <= 0 || height <= 0 || !params) { _LTSetError(LT_ERROR_INVALID_PARAM, "Invalid parameters provided to LTCreateTextureFromMemory."); return texture; }
    if (lt_state.active_texture_count >= LT_MAX_TEXTURES) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "Maximum number of textures reached."); return texture; }

    int texture_idx = -1;
    for (int i = 1; i < LT_MAX_TEXTURES; i++) {
        if (!lt_state.texture_slots_used[i]) { texture_idx = i; break; }
    }
    if (texture_idx == -1) { _LTSetError(LT_ERROR_TEXTURE_LIMIT_REACHED, "No available texture slots."); return texture; }

    GLint gl_internal_format;
    GLenum gl_format, gl_type;
    if (!_LTConvertFormatToGL(params->format, &gl_internal_format, &gl_format, &gl_type)) { _LTSetError(LT_ERROR_TEXTURE_FORMAT_UNSUPPORTED, "The specified format is not supported for creation from memory."); return texture; }

    LTTexture* new_tex = <_state.textures[texture_idx];
    memset(new_tex, 0, sizeof(LTTexture));

    glGenTextures(1, &new_tex->backend.gl.gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, new_tex->backend.gl.gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, gl_type, data);

    // Apply parameters (this can be refactored into a helper function later)
    if (params->generate_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _LTConvertWrapToGL(params->wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _LTConvertWrapToGL(params->wrap_t));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _LTConvertFilterToGL(params->filter_min));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _LTConvertFilterToGL(params->filter_mag));
    if (params->anisotropic_level > 0) {
        GLfloat max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        if (max_anisotropy > 0.0f) {
            float level = (GLfloat)params->anisotropic_level > max_anisotropy ? max_anisotropy : (GLfloat)params->anisotropic_level;
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, level);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Finalize state
    new_tex->id = texture_idx;
    new_tex->width = width;
    new_tex->height = height;
    new_tex->format = params->format;
    snprintf(new_tex->name, LT_MAX_TEXTURE_NAME_LEN, "MemoryTexture_%d", new_tex->id);

    lt_state.texture_slots_used[texture_idx] = true;
    lt_state.active_texture_count++;
    lt_state.ref_counts[texture_idx] = 1;
    lt_state.vram_usage_bytes += _LTGetTextureSizeBytes(params->format, width, height, params->generate_mipmaps);
    return *new_tex;
}

/**
 * @brief Creates a texture atlas from a list of image files.
 * @param filenames An array of file paths to the images.
 * @param count The number of images in the array.
 * @param max_width The maximum width of the resulting atlas texture.
 * @param max_height The maximum height of the resulting atlas texture.
 * @param out_uv_coords A user-provided array of vec4 to be filled with the UV coordinates for each input image. Each vec4 represents the sub-rectangle in normalized texture coordinates (x, y, width, height).
 * @return The generated texture atlas.
 */
LTAPI LTTexture LTCreateTextureAtlas(const char** filenames, int count, int max_width, int max_height, vec4* out_uv_coords) {
    LTTexture atlas = {0};
    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return atlas; }
    // TODO: Load images, pack into atlas, generate UV coords
    return atlas;
}

/**
 * @brief Retrieves the current estimated VRAM usage by textures managed by this library.
 * @return The total number of bytes currently allocated for textures.
 */
LTAPI uint64_t LTGetVRAMUsageBytes(void) {
    if (!lt_state.is_initialized) { return 0; }
    return lt_state.vram_usage_bytes;
}

/**
 * @brief Sets the VRAM budget for the texture manager.
 *
 * This value is currently for informational purposes and is not strictly enforced.
 * Future versions may use this to automatically evict non-critical textures.
 *
 * @param budget_bytes The new VRAM budget in bytes. A value of 0 implies an unlimited budget.
 * @return LT_SUCCESS on success, or LT_ERROR_NOT_INITIALIZED.
 */
LTAPI LTError LTSetVRAMBudget(uint64_t budget_bytes) {
    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "Cannot set VRAM budget, library not initialized."); return LT_ERROR_NOT_INITIALIZED; }
    lt_state.vram_budget_bytes = budget_bytes;
    return LT_SUCCESS;
}

/**
 * @brief Enables or disables reference counting for texture resources.
 *
 * When enabled, calling LTDestroyTexture on a texture that is referenced multiple times will only decrement its reference count. The texture is only truly destroyed
 * when its reference count reaches zero.
 *
 * @param enable Set to true to enable reference counting, false to disable.
 */
LTAPI void LTEnableReferenceCounting(bool enable) {
    if (!lt_state.is_initialized) { return; }
    lt_state.ref_counting_enabled = enable;
}

/**
 * @brief Gets the current reference count for a specific texture.
 *
 * This is useful for debugging resource lifetimes when reference counting is enabled.
 *
 * @param texture The texture handle to query.
 * @return The current reference count of the texture, or 0 if the handle is invalid or reference counting is disabled.
 */
LTAPI int LTGetTextureReferenceCount(LTTexture texture) {
    if (!lt_state.is_initialized || !lt_state.ref_counting_enabled || texture.id == 0 || texture.id >= LT_MAX_TEXTURES) { return 0; }
    // Ensure the texture slot is actually in use before returning its count.
    if (!lt_state.texture_slots_used[texture.id]) { return 0; }
    return lt_state.ref_counts[texture.id];
}

/**
 * @brief Generates mipmaps for an existing texture.
 *
 * This is useful for textures that were created without mipmaps initially.
 * Calling this on a texture that already has mipmaps will regenerate them.
 * Note: This operation can increase VRAM usage by approximately 33%.
 * This VRAM increase is not yet tracked by the library's budget system.
 *
 * @param texture The texture to generate mipmaps for.
 * @return LT_SUCCESS on success, or an error code if the texture is invalid.
 */
LTAPI LTError LTGenerateMipmaps(LTTexture texture) {
    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return LT_ERROR_NOT_INITIALIZED; }
    if (texture.id == 0 || texture.id >= LT_MAX_TEXTURES || !lt_state.texture_slots_used[texture.id]) { _LTSetError(LT_ERROR_INVALID_PARAM, "Invalid texture handle provided to LTGenerateMipmaps."); return LT_ERROR_INVALID_PARAM; }

    uint64_t base_size = _LTGetTextureSizeBytes(texture.format, texture.width, texture.height, false);
    lt_state.vram_usage_bytes += (uint64_t)(base_size * 0.33334f);

    // Retrieve the texture from the pool to modify its state
    LTTexture* tex_to_update = <_state.textures[texture.id];
    
#if defined(SITUATION_USE_OPENGL)
    if (lt_state.renderer_type == LT_RENDERER_OPENGL) {
        if (tex_to_update->has_mipmaps) {
            glBindTexture(GL_TEXTURE_2D, tex_to_update->backend.gl.gl_texture_id);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            return LT_SUCCESS;
        }

        glBindTexture(GL_TEXTURE_2D, tex_to_update->backend.gl.gl_texture_id);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        uint64_t base_size = _LTGetTextureSizeBytes(tex_to_update->format, tex_to_update->width, tex_to_update->height, false);
        uint64_t mipmap_extra_size = (uint64_t)(base_size * 0.33334f);
        lt_state.vram_usage_bytes += mipmap_extra_size;
        tex_to_update->has_mipmaps = true;
    }
#elif defined(SITUATION_USE_VULKAN)
    if (lt_state.renderer_type == LT_RENDERER_VULKAN) {
        // TODO: Vulkan mipmap generation is a complex process involving multiple image
        // blits with layout transitions in a command buffer.
        _LTSetError(LT_ERROR_GENERAL, "Mipmap generation is not yet implemented for the Vulkan backend.");
        return LT_ERROR_GENERAL; // Return an error to indicate it's not ready
    }
#endif

    return LT_SUCCESS;
}

/**
 * @brief Sets the wrapping, filtering, and anisotropic properties of an existing texture.
 *
 * @param texture The texture to modify.
 * @param params A pointer to an LTTextureParams struct containing the new parameters.
 * @return LT_SUCCESS on success, or an error code if parameters are invalid.
 */
LTAPI LTError LTSetTextureParams(LTTexture texture, const LTTextureParams* params) {
    if (!lt_state.is_initialized) { _LTSetError(LT_ERROR_NOT_INITIALIZED, "lib_tex is not initialized."); return LT_ERROR_NOT_INITIALIZED; }
    if (texture.id == 0 || texture.id >= LT_MAX_TEXTURES || !lt_state.texture_slots_used[texture.id]) { _LTSetError(LT_ERROR_INVALID_PARAM, "Invalid texture handle provided to LTSetTextureParams."); return LT_ERROR_INVALID_PARAM; }
    if (!params) { _LTSetError(LT_ERROR_INVALID_PARAM, "LTTextureParams cannot be NULL."); return LT_ERROR_INVALID_PARAM; }

#if defined(SITUATION_USE_OPENGL)
    glBindTexture(GL_TEXTURE_2D, texture.backend.gl.gl_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _LTConvertWrapToGL(params->wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _LTConvertWrapToGL(params->wrap_t));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _LTConvertFilterToGL(params->filter_min));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _LTConvertFilterToGL(params->filter_mag));

    if (params->anisotropic_level > 0) {
        // Check for extension support before setting anisotropic filtering.
        // A more robust implementation would cache this check during init.
        GLfloat max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        if (max_anisotropy > 0.0f) {
            float level = (GLfloat)params->anisotropic_level > max_anisotropy ? max_anisotropy : (GLfloat)params->anisotropic_level;
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, level);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
#elif defined(SITUATION_USE_VULKAN)
    // TODO: Vulkan doesn't modify textures directly. This would involve creating a new VkSampler object and associating it with the texture's descriptor.
    _LTSetError(LT_ERROR_GENERAL, "Setting texture parameters is not yet implemented for the Vulkan backend.");
#endif

    return LT_SUCCESS;
}

/**
 * @brief Gets the dimensions of a texture.
 *
 * @param texture The texture to query.
 * @param width A pointer to an integer to store the width.
 * @param height A pointer to an integer to store the height.
 */
LTAPI void LTGetTextureSize(LTTexture texture, int* width, int* height) {
    // A simple getter doesn't need to check for initialization.
    // It's safe to call on an uninitialized library.
    if (!width || !height) { return; } // Avoid crashing on null pointers.
    if (texture.id == 0 || texture.id >= LT_MAX_TEXTURES || !lt_state.texture_slots_used[texture.id]) {
        *width = 0;
        *height = 0;
        return;
    }
    // Retrieve the dimensions from our internal state.
    *width = lt_state.textures[texture.id].width;
    *height = lt_state.textures[texture.id].height;
}

/**
 * @brief Gets the format of a texture.
 *
 * @param texture The texture to query.
 * @return The LTTextureFormat enum for the texture, or LT_FORMAT_RGBA8 for an invalid handle.
 */
LTAPI LTTextureFormat LTGetTextureFormat(LTTexture texture) {
    if (texture.id == 0 || texture.id >= LT_MAX_TEXTURES || !lt_state.texture_slots_used[texture.id]) { return LT_FORMAT_RGBA8; } // Return a sensible default for invalid handles.
    return lt_state.textures[texture.id].format;
}
// ... rest of the implementation for other functions ...

// ============================================================================
//
// IMPLEMENTATION
//
// ============================================================================
// Include backend-specific headers and implementation code here.
// For example:
// #include <stdio.h>
// #include <string.h>
//
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
//
// #if defined(SITUATION_USE_OPENGL) // Assuming situation.h provides this
// #include <glad/glad.h>
// #elif defined(SITUATION_USE_VULKAN)
// #include <vulkan/vulkan.h>
// #define VMA_IMPLEMENTATION
// #include "vk_mem_alloc.h"
// #endif
#endif // LT_IMPLEMENTATION
#endif // LT_H