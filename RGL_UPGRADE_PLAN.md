# RGL Library Upgrade Design Document

## 1. Executive Summary
This document outlines the architectural refactoring plan to upgrade the `rgl.h` single-header rendering library. The primary objective is to fully integrate `rgl.h` with the `Situation` API (`situation.h`), transforming it from a direct OpenGL wrapper into a high-level renderer that leverages Situation's modern, backend-agnostic features. This includes adopting bindless resources, command buffer workflows, and unified asset management.

## 2. Architectural Review: `rgl.h` vs `situation.h`

### 2.1 Dependencies & Backend Capabilities
*   **Current State:** `rgl.h` depends on `lib_tex.h` for texture loading and `<glad/glad.h>` for OpenGL 3.3 calls. It manages its own GL context and extensions.
*   **Target State:**
    *   **Dependency Removal:** `rgl.h` will depend *exclusively* on `situation.h` (and the math library `cglm`). All direct calls to `glad`, `glfw`, `miniaudio`, or `lib_tex` will be removed.
    *   **Backend Abstraction:** `rgl.h` will no longer make direct graphics API calls. Instead, it will use `Situation` primitives that abstract over Vulkan 1.4+ (Descriptor Indexing, Buffer Device Address) and OpenGL 4.6+ (Bindless Textures, Persistent Mapping).

### 2.2 Texture Management (`RGLTexture`)
*   **Current State:** `RGLTexture` is an alias for `LTTexture`. Code accesses raw GL IDs (`.id`) and parameters (`.width`, `.wrap_mode`) directly.
*   **Target State:**
    *   **Struct Refactor:** `RGLTexture` will be redefined as an opaque wrapper around `SituationTexture`.
        ```c
        typedef struct {
            SituationTexture texture;       // The opaque resource handle
            uint64_t bindless_handle;       // Cached bindless handle for shaders (SituationGetTextureHandle)
            int virtual_display_id;         // -1 if standard texture, >= 0 if a Render Target
        } RGLTexture;
        ```
    *   **Loading:** Replace `LTLoadTexture` calls with `SituationLoadTexture`. The bindless handle will be fetched immediately after load.
    *   **Access:** Texture properties (width, height) will be cached or queried via `Situation` helpers if needed, though most rendering code should only need the `bindless_handle`.

### 2.3 Mesh & Geometry
*   **Current State:** `RGLMesh` manages `GLuint` buffers and uses `glDrawElements`.
*   **Target State:**
    *   **Storage:** `RGLMesh` will wrap `SituationMesh`.
    *   **Creation:** `RGL_LoadMeshFromFile` will call `SituationLoadModel` or `SituationCreateMesh`.
    *   **Drawing:** `RGL_DrawMesh` will record `SituationCmdDrawMesh` commands into the active `SituationCommandBuffer`.

### 2.4 Render Targets & Virtual Displays
*   **Current State:** `RGL_CreateRenderTexture` creates raw OpenGL Framebuffer Objects (FBOs).
*   **Target State:**
    *   **Creation:** `RGL_CreateRenderTexture` will call `SituationCreateVirtualDisplay`. The returned `RGLTexture` will store the `virtual_display_id` and the color attachment texture.
    *   **Lifecycle:** `RGL_Begin(id)` will map to `SituationCmdBeginRenderPass` targeting that specific Virtual Display ID.

### 2.5 The Rendering Pipeline (Main Batcher)
*   **Current State:** `_RGL_FlushBatch` builds a vertex array on the CPU and uploads it via `glBufferSubData`. It sets state manually with `glEnable`/`glBlendFunc`.
*   **Target State:** Migrate to a **Bindless Command Buffer** model.
    *   **Data Storage:** The batcher will use a large `SituationBuffer` created with `SITUATION_BUFFER_USAGE_STORAGE_BUFFER | SITUATION_BUFFER_USAGE_DEVICE_ADDRESS`.
    *   **Vertex Data:** Instead of binding vertex attributes, `_RGL_FlushBatch` will upload data to this buffer via `SituationUpdateBuffer`.
    *   **Shader Access:** The shader will access vertex data directly via the buffer's 64-bit device address (passed as a Push Constant or UBO), enabling high-performance "Vertex Pulling".
    *   **Commands:** The batcher will record `SituationCmdDraw` calls.
    *   **State:** Pipeline state (blending, depth) will be managed by binding specific `SituationShader` (Pipeline) objects created via `SituationCreateComputePipeline` or `SituationLoadShaderFromMemory`.

### 2.6 Shaders & Lighting
*   **Current State:** Uses legacy `glGetUniformLocation` and `glUniform*` calls.
*   **Target State:**
    *   **Uniforms:** Migrate to **Uniform Buffer Objects (UBOs)**. Create `SituationBuffer`s with `SITUATION_BUFFER_USAGE_UNIFORM_BUFFER`.
    *   **Updates:** Pack light and camera data into C structs matching the shader layout and upload via `SituationUpdateBuffer`.
    *   **Binding:** Bind these UBOs to the pipeline using `SituationCmdBindShaderBuffer` (or `SituationCmdBindDescriptorSet` depending on the pipeline layout).

---

## 3. Phased Upgrade Plan

### Phase 1: Dependency Stripping & Sanitization
*Goal: Ensure `rgl.h` compiles with only `situation.h` included.*
- [ ] **Remove Includes:** Delete `#include "lib_tex.h"` and `#include <glad/glad.h>`.
- [ ] **Type Replacement:** Replace `Rectangle` with `SitRectangle`.
- [ ] **Cleanup:** Fix syntax errors (e.g., missing parentheses in compound literals) and remove duplicate function definitions.

### Phase 2: Texture System Refactor
*Goal: Adapt `RGLTexture` to the Situation API.*
- [ ] **Struct Definition:** Define the new `RGLTexture` struct wrapping `SituationTexture`.
- [ ] **Load/Unload:** Rewrite `RGL_LoadTexture` to call `SituationLoadTexture` and `SituationGetTextureHandle`. Rewrite `RGL_UnloadTexture` to call `SituationDestroyTexture`.
- [ ] **Field Access:** Update all code accessing `.id`, `.width`, etc., to use the new struct fields.

### Phase 3: Virtual Display Integration
*Goal: Modernize Render-to-Texture.*
- [ ] **Creation:** Update `RGL_CreateRenderTexture` to use `SituationCreateVirtualDisplay`.
- [ ] **State:** Update `RGL_SetRenderTarget` to track the active Virtual Display ID.
- [ ] **Begin/End:** Update `RGL_Begin` to use `SituationCmdBeginRenderPass` with the correct target.

### Phase 4: Core Renderer Refactor (Bindless Batcher)
*Goal: Implement the high-performance rendering pipeline.*
- [ ] **Buffer Creation:** In `RGL_Init`, create the main batch buffer as a `SituationBuffer` with `STORAGE | DEVICE_ADDRESS` flags.
- [ ] **Shaders:** Update internal shaders (source strings) to use bindless texture handles (`uvec2` or `sampler2D`) and buffer references. Load them via `SituationLoadShaderFromMemory`.
- [ ] **Flush Logic:** Rewrite `_RGL_FlushBatch`:
    - Acquire command buffer via `SituationAcquireFrameCommandBuffer`.
    - Upload batch data via `SituationUpdateBuffer`.
    - Bind the pipeline via `SituationCmdBindPipeline`.
    - Bind resources via `SituationCmdBindShaderBuffer` / `SituationCmdBindDescriptorSet`.
    - Issue draw calls via `SituationCmdDraw`.

### Phase 5: Lighting & Uniforms
*Goal: Transition to UBO-based lighting.*
- [ ] **UBO Creation:** In `RGL_Init`, create `SituationBuffer`s for Light data and Camera matrices.
- [ ] **Data Upload:** In `_RGL_FlushBatch`, update these buffers with the current frame's light/camera state.
- [ ] **Binding:** Ensure `_RGL_FlushBatch` binds these UBOs to the correct shader slots.

### Phase 6: Final Cleanup
- [ ] **Debug Tools:** Port `RGL_DrawWireframeBounds` to use Situation drawing commands.
- [ ] **Sanitization:** Verify no `gl*` calls remain.
