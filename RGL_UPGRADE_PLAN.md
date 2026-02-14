# RGL Library Upgrade Design Document

## 1. Executive Summary
This document outlines the architectural refactoring plan to upgrade the `rgl.h` single-header rendering library. The primary objective is to decouple `rgl.h` from direct OpenGL calls (`glad`) and external texture libraries (`lib_tex`), fully integrating it with the `Situation` API (`situation.h`). This transition will make `rgl.h` renderer-agnostic, leveraging `Situation`'s abstraction layer for windowing, input, and graphics (Vulkan/OpenGL).

## 2. Architectural Review: `rgl.h` vs `situation_api.md`

### 2.1 Dependencies
*   **Current State:** `rgl.h` relies on `lib_tex.h` for image loading/texture management and `<glad/glad.h>` for direct OpenGL 3.3 function calls.
*   **Target State:** All external dependencies must be removed. `rgl.h` shall depend *exclusively* on `situation.h` (for platform/graphics) and `cglm` (for mathematics).

### 2.2 Texture Management
*   **Current State:** Uses `LTTexture` struct and `LTLoadTexture` functions.
*   **Target State:**
    *   Replace `LTTexture` with `SituationTexture` handles.
    *   Replace loading logic with `SituationLoadImage` (CPU) $\rightarrow$ `SituationCreateTexture` (GPU) $\rightarrow$ `SituationUnloadImage`.
    *   This leverages `Situation`'s unified asset loading pipeline.

### 2.3 Mesh & Geometry
*   **Current State:** `RGLMesh` struct uses `SituationMesh` but implementation may rely on GL calls.
*   **Target State:**
    *   Ensure `RGLMesh` wraps `SituationMesh` purely.
    *   `RGL_LoadMeshFromFile` must use `SituationLoadModel` or `SituationCreateMesh`.

### 2.4 Render Targets & Virtual Displays
*   **Current State:** `RGL_CreateRenderTexture` creates FBOs using direct GL calls. `RGL_DrawPathAsMap` renders to them.
*   **Target State:**
    *   `RGL_CreateRenderTexture` must wrap `SituationCreateVirtualDisplay`.
    *   `RGL_DrawPathAsMap` must act as a render pass targeting the specific `SituationVirtualDisplay`.

### 2.5 The Rendering Pipeline (Main Batcher)
*   **Current State:** Uses an "Immediate Mode" style batching system (`_RGL_FlushBatch`) that directly maps to `glBufferSubData` and `glDrawArrays`. It manages GL state manually (`glEnable`, `glBlendFunc`, `glUseProgram`).
*   **Target State:** Must migrate to `Situation`'s **Command Buffer** architecture.
    *   **Frame Cycle:** `RGL_Begin` must acquire a `SituationCommandBuffer` and start a Render Pass.
    *   **Batching:** `_RGL_FlushBatch` must record commands (`SituationCmdDraw`, `SituationCmdBindPipeline`, `SituationCmdBindVertexBuffer`) into the active command buffer.
    *   **State:** Pipeline state (blending, depth) will be encapsulated in `SituationShader` (Pipelines) or Render Pass configurations.

### 2.6 Independent Rendering Paths
*   **Current State:** Functions like `_RGL_DrawMapPolygon` and `RGL_DrawShadowVolumeDebug` use immediate GL calls outside the main batcher.
*   **Target State:** These must be refactored to either:
    *   Use the main batcher (if possible).
    *   Or create their own temporary `SituationBuffer` and issue `SituationCmdDraw` calls.

### 2.7 Lighting & Shaders
*   **Current State:** Uses legacy `glGetUniformLocation` and `glUniform*` calls.
*   **Target State:** Must adopt the "Shader Contract" defined in `situation_api.md`.
    *   **Uniforms:** Migrate to **Uniform Buffer Objects (UBOs)**. Lighting data will be uploaded to a `SituationBuffer` and bound via `SituationCmdBindShaderBuffer`.
    *   **Shaders:** Loaded via `SituationLoadShader`.

---

## 3. Phased Upgrade Plan

### Phase 0: Verification Environment Setup
*Goal: Establish a baseline for testing compilation.*
- [ ] **Create `situation.h` Mock:** Generate a header file matching `situation_api.md`.
- [ ] **Create `test_build.c`:** A minimal C file to compile `rgl.h`.

### Phase 1: Dependency Removal & Type Migration
*Goal: Remove `lib_tex` and `glad`.*
- [ ] **Remove Includes:** Delete `#include "lib_tex.h"` and `#include <glad/glad.h>`.
- [ ] **Refactor Textures:** Redefine `RGLTexture` as `SituationTexture`. Rewrite loading functions.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 2: Initialization & Resource Management
*Goal: Move global resource creation to `Situation` factories.*
- [ ] **Refactor `RGL_Init`:** Remove `LTInit`/`glad` init. Replace `glGenBuffers` with `SituationCreateBuffer`. Load shaders via `SituationLoadShader`.
- [ ] **Refactor `RGL_Shutdown`:** Destroy `Situation` resources.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 3: Render Target & Virtual Display Integration
*Goal: Map RGL Render Textures to Situation Virtual Displays.*
- [ ] **Refactor `RGL_CreateRenderTexture`:** Use `SituationCreateVirtualDisplay`.
- [ ] **Refactor `RGL_DrawPathAsMap`:** Use `SituationCmdBeginRenderPass` with the new display ID.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 4: Core Rendering Pipeline (Main Batcher)
*Goal: Switch main drawing to Command Buffers.*
- [ ] **Refactor `RGL_Begin`/`RGL_End`:** Manage Render Pass lifecycle.
- [ ] **Refactor `_RGL_FlushBatch`:** Use `SituationUpdateBuffer` and `SituationCmd*` commands.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 5: Independent Rendering Paths
*Goal: Port standalone draw functions.*
- [ ] **Refactor `_RGL_DrawMapPolygon`:** Use `Situation` draw commands.
- [ ] **Refactor `RGL_DrawShadowVolumeDebug`:** Use `Situation` draw commands.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 6: Lighting & Uniforms
*Goal: Implement UBOs.*
- [ ] **Implement UBOs:** Create Light and View buffers.
- [ ] **Bind UBOs:** Update `_RGL_FlushBatch` to bind them.
- [ ] **Verification:** Compile `test_build.c`.

### Phase 7: Debug Tools & Cleanup
*Goal: Finalize code.*
- [ ] **Debug Renderer:** Port `RGL_DrawWireframeBounds`.
- [ ] **Sanitization:** Remove all `gl*` calls.
- [ ] **Final Review:** Full compilation check.
