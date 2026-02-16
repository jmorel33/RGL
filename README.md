# rgl


@file rgl.h
@brief A High-Performance 2D/3D Renderer with an Integrated Dynamic Lighting and World System.

@version 6.1
@date June 25, 2025

@section overview Overview
rgl.h is a single-header rendering library for the KaOS Engine, designed for high-performance, batched 2D/3D graphics. It provides a flexible API for general-purpose
rendering and includes specialized, powerful systems for creating data-driven worlds, complete with dynamic lighting, procedural Paths, and structured levels. It is ideal
for projects ranging from modern UI applications to complex, retro-inspired arcade games.

@section key_features Key Features
- **Unified Lighting Engine:** A powerful, UBO-driven lighting system supporting Point, Directional, and Spot lights. All 3D geometry is dynamically lit with per-pixel diffuse lighting.
- **True 3D Rendering Pipeline:** All geometry is rendered as true 3D primitives with correct perspective, depth, and lighting calculations.
- **High-Performance Batching:** Automatically minimizes GPU state changes and draw calls by sorting and batching thousands of commands, with dynamic buffer growth to prevent overflows.
- **Extensible, Data-Driven World Systems:**
- **Multi-Path System:** Define complex, spline-based path networks with topological junctions (forks, merges, crossroads). The visual appearance of paths is fully customizable via
a callback-based "style" system, allowing for roads, rivers, rollercoasters, and more out of the box.
- **Extensible Scenery System:** Attach scenery to paths with custom, user-defined drawing logic. A global registry allows you to add new types of scenery (e.g., animated signs,
interactive elements) and control how they are rendered.
- **Structured Level System:** Build classic "Doom-style" levels from vertices, walls, and flats, with support for non-convex shapes and full dynamic lighting.
- **Comprehensive API:** Includes low-level primitives (`RGL_DrawSpritePro`), high-level systems (`RGL_DrawPath`, `RGL_DrawLevel`), powerful world query functions (`RGL_QueryJunction`),
and debug tools (`RGL_SetDebugDrawTriggers`).
- **Retro Aesthetics Toolkit:** A rich suite of YPQ color space functions for emulating classic CRT/NTSC visual styles, fully compatible with the modern rendering pipeline.

@section path_system_philosophy Path & Scenery System Philosophy
The world-building systems in rgl.h are designed to be powerful, data-driven, and, most importantly, extensible. The core principle is the **separation of data from presentation**.
- **Paths are Just Data:** A "Path" in rgl.h is a purely mathematical concept—a 3D spline with associated data like width, banking, and scenery. By itself, it has no visual appearance.
- **Styles Define Appearance:** The visual look of a path is determined by an `RGLPathStyle`. This is a struct containing a function pointer to a master drawing function. `rgl.h` provides
a default style for drawing classic roads (`RGL_DrawPathAsRoad`), but users are encouraged to write their own drawing functions to render paths as anything they can imagine: rivers,
castle walls, sci-fi energy conduits, etc. You can assign different styles to different paths using `RGL_SetPathStyle()`.
- **Scenery is also Extensible:** In the same way, the appearance of "scenery" attached to a path is not fixed. Using `RGL_RegisterSceneryStyle()`, you can define how any `RGLSceneryType`
is rendered, or even create your own custom scenery types for things like animated signs, particle emitters, or interactive objects.
- **Junctions are Topological:** The path system supports true path networks. A `RGL_SCENERY_JUNCTION_TRIGGER` is not just a visual signpost; it's a topological link between different
named paths. The `RGL_QueryJunction()` function allows game logic to robustly navigate these networks, enabling features like highway off-ramps, branching dungeon corridors, or
complex track-switching.

@section recommended_workflow The Render-Pass Workflow (The Painter's Algorithm)
To achieve a correctly rendered scene, draw calls must be layered in a specific order.
1.  **Initialize & Build:** Call `RGL_Init()`. Register any custom Path or Scenery styles. Build your world geometry using `RGL_CreatePath()`, `RGL_CreateLevel()`, etc.
2.  **Game Loop - Update State:** Update your camera and dynamic objects. Use `RGL_QueryJunction()` to handle path switching logic.
3.  **Game Loop - Render Scene (`RGL_Begin`/`RGL_End` block):**
- **Pass 1: Opaque Geometry (The "Depth Pass")**
Draw all solid, non-transparent geometry first to populate the depth buffer.
- `RGL_SetCamera3D(...)`
- `RGL_DrawLevel()`
- `RGL_DrawPath(...)` // Or the RGL_DrawPathAsRoad() wrapper
- `RGL_DrawMesh(...)`

- **Pass 2: Shadows (The "Stencil Pass")**
Cast all your shadows onto the now-solid world.
- `RGL_CastStencilShadowFromMesh(...)`
- `RGL_DrawSpriteWithShadow(...)`

- **Pass 3: Transparent Geometry (The "Alpha Pass")**
Draw alpha-blended objects like sprites and particles.
- `RGL_DrawBillboard(...)`

- **Pass 4: UI / Overlay (The "HUD Pass")**
Switch to a 2D camera to draw your UI.
- `RGL_PushMatrix()`, `RGL_SetCamera2D(...)`, `RGL_DrawText(...)`, `RGL_PopMatrix()`

@example (A Sci-Fi Scene with a Custom Path Style and Junctions)
// --- In MyGame_Init() ---
void DrawEnergyConduit(float player_z, int dist, void* data); // My custom drawing function
RGLPathStyle conduit_style = { .draw_path_func = DrawEnergyConduit, .user_data = NULL };

RGL_CreatePath("MainConduit");
RGL_SetPathStyle("MainConduit", &conduit_style); // Assign my custom style
// ... add points to MainConduit ...

RGL_CreatePath("SideTunnel"); // A different path
// ... add points to SideTunnel ...

// At Z=500 on MainConduit, add a junction trigger to fork left into the SideTunnel
RGLPathPoint p = { .world_z = 500, ... };
p.scenery_left.type = RGL_SCENERY_JUNCTION_TRIGGER;
p.scenery_left.data.junction.type = RGL_JUNCTION_FORK_EXIT;
strncpy(p.scenery_left.data.junction.connect_left.path_name, "SideTunnel", 31);
p.scenery_left.data.junction.connect_left.z_pos = 0.0f;
RGL_AddPathPoint("MainConduit", p);


// --- In MyGame_Update() ---
RGLJunctionInfo junction;
// Check if the player is turning left near a junction
if (player_is_turning_left && RGL_QueryJunction(g_player_z, 10.0f, &junction)) {
if (junction.choice_left.path_name[0] != '\0') {
RGL_SetActivePath(junction.choice_left.path_name);
g_player_z = junction.choice_left.z_pos;
}
}


// --- In MyGame_Render() ---
RGL_Begin(-1);
RGL_SetCamera3D(camera_pos, camera_target, camera_up, 75.0f);

// --- PASS 1: OPAQUE GEOMETRY ---
// This one call will use our custom DrawEnergyConduit function because we set the style!
RGL_DrawPath(g_player_z, 300);

// ... other passes ...
RGL_End();

@section workflow_3d_objects Custom 3D Object Workflow
Beyond the high-level world systems, rgl.h provides a powerful pipeline for loading, transforming, and rendering custom 3D models. This workflow is essential for player characters,
vehicles, items, and any other dynamic entity in your scene.

1.  **Load Assets at Startup:** The most important principle for performance is to load your 3D model and texture data only once, for example, during your game's initialization.
- Use `RGL_LoadMeshFromFile()` to load a .obj file into a persistent `RGLMesh` object. This function parses the geometry and uploads it to the GPU.
- Use `RGL_LoadTexture()` to load the corresponding texture.
- Store these `RGLMesh` and `RGLTexture` handles in your game's object structures.

2.  **Define Your Game Object:** In your game's code, create a struct to represent your 3D object. This struct will hold the asset handles and the object's state.
@code
typedef struct {
RGLMesh mesh_handle;
RGLTexture texture_handle;
vec3 position;
vec3 rotation_eul_deg;
vec3 scale;

mat4 final_transform; // This will be calculated each frame
} My3DObject;
@endcode

3.  **Update State & Transform (In Game Loop):** Each frame, update your object's position, rotation, and scale based on player input or AI. Then, combine these
into a single `mat4` transformation matrix.
@code
// In your Update() function:
My3DObject* player_ship;
// ... update player_ship->position from input ...
// Calculate its final transform matrix for this frame
glm_mat4_identity(player_ship->final_transform);
glm_translate(player_ship->final_transform, player_ship->position);
glm_euler_to_mat4((vec3){rad(rot.x), rad(rot.y), rad(rot.z)}, rotation_mat);
glm_mat4_mul(player_ship->final_transform, rotation_mat, player_ship->final_transform);
glm_scale(player_ship->final_transform, player_ship->scale);
@endcode

4.  **Render Using the Painter's Algorithm (In Render Loop):** Follow the correct render-pass order to ensure correct lighting and shadowing.
- **Pass 1: Opaque Geometry:** Draw your solid 3D object. This renders it to the screen and populates the depth buffer.
- `RGL_DrawMesh(player_ship->mesh_handle, player_material, player_ship->texture_handle, player_ship->final_transform);`
- **Pass 2: Shadows:** Use the same mesh and transform to cast a shadow. The library uses the mesh's CPU-side vertex data to generate the shadow volume.
- `RGL_CastStencilShadowFromMesh(player_ship->mesh_handle, player_ship->final_transform, &shadow_config);`

5.  **Cleanup:** When the object is no longer needed (e.g., at level unload), be sure to free the resources to prevent memory leaks.
- `RGL_DestroyMesh(&my_object.mesh_handle);`
- `RGL_UnloadTexture(my_object.texture_handle);`

@example (Rendering a Player Ship with a Stencil Shadow)
// --- In Render Function ---
RGL_Begin(-1);
RGL_SetCamera3D(...);

// --- PASS 1: OPAQUE ---
RGL_DrawLevel();
// Draw the player ship. It is now part of the lit, solid world.
RGL_DrawMesh(g_player.mesh, g_player.material, g_player.texture, g_player.transform);

// --- PASS 2: SHADOWS ---
RGLShadowConfig config = { .light_id = g_sun_light, ... };
// The player ship now casts a shadow onto the level.
RGL_CastStencilShadowFromMesh(g_player.mesh, g_player.transform, &config);

// --- PASS 3 & 4: TRANSPARENT & UI ---
RGL_DrawParticles();
// ... draw UI ...
RGL_End();


@section opengl_4_6_migration_plan OpenGL 4.6 Modernization Roadmap (Future)
rgl.h was originally designed with an OpenGL 3.3 feature set, which is robust and widely compatible. To maximize performance on modern hardware and align with the
capabilities of situation.h, a future version of rgl.h will be migrated to leverage core OpenGL 4.6 features. This migration will focus on reducing CPU overhead,
minimizing driver work, and offloading tasks to the GPU.

This section outlines the planned improvements, which will be implemented after the migration to lib_tex.h is complete.

@subsection opengl_4_6_step1 Step 1: Adopting the "Shader Contract" and UBOs
- **Problem:** The current renderer uses `glGetUniformLocation()` at runtime to find shader uniforms. This involves string comparisons and can be a performance bottleneck.
- **Solution:** The shaders will be updated to use explicit locations and Uniform Buffer Objects (UBOs), fully adopting the "Shader Contract" established by situation.h.
- **Uniforms:** `uniform mat4 view;` will become `layout (location = N) uniform mat4 model;`.
- **UBOs:** The renderer will bind to the `ViewData` UBO provided by situation.h (at `binding = 1`) to get camera matrices, and will continue to use its own UBO for lighting data (at `binding = 0`).
- **Benefit:** Eliminates all runtime string lookups for uniforms and allows large data blocks like camera matrices to be updated on the GPU in a single, efficient call.

@subsection opengl_4_6_step2 Step 2: Full Integration of Direct State Access (DSA)
- **Problem:** The current code relies heavily on the "bind-to-edit" pattern (e.g., `glBindTexture`, then `glTexParameteri`). This forces the driver to constantly re-validate state.
- **Solution:** All state modification calls will be converted to their DSA equivalents, which operate directly on an object's ID without changing the currently bound object.
- `glBindTexture` + `glTexParameter*` -> `glTextureParameter*`
- `glBindBuffer` + `glBufferData` -> `glNamedBufferData`
- `glGenerateMipmap` -> `glGenerateTextureMipmap`
- **Benefit:** Dramatically reduces driver overhead and API calls, leading to cleaner code and higher performance by avoiding modification of the global GL state machine.

@subsection opengl_4_6_step3 Step 3: High-Throughput Batching with Persistent Buffers
- **Problem:** The current batch renderer (`_RGL_FlushBatch`) assembles a vertex buffer on the CPU and then copies it to the GPU with `glBufferSubData` every time it flushes. This per-flush copy can cause CPU/GPU synchronization stalls.
- **Solution:** The batching system will be migrated to use a **persistently mapped buffer**.
- At initialization, the VBO will be created with `GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT`.
- It will be mapped **once** using `glMapNamedBufferRange`, and the returned pointer will be stored.
- During `_RGL_FlushBatch`, vertex data will be written **directly** to this pointer, which points to GPU-accessible memory.
- **Benefit:** Completely eliminates the per-flush `glBufferSubData` copy from CPU to GPU memory, significantly improving throughput and reducing stalls.

@subsection opengl_4_6_step4 Step 4: Offloading Draw Loops with Multi-Draw Indirect (MDI)
- **Problem:** The `_RGL_FlushBatch` function currently loops through the sorted commands on the CPU and issues multiple `glDrawArrays` calls. This CPU loop is an overhead that can be eliminated.
- **Solution:** The renderer will be updated to use `glMultiDrawArraysIndirect`.
- In `_RGL_FlushBatch`, instead of a C loop, the renderer will build a small array of `DrawArraysIndirectCommand` structs.
- This small command buffer will be uploaded to a dedicated GPU buffer (an "indirect buffer").
- A **single** call to `glMultiDrawArraysIndirect` will tell the GPU to read the commands from the buffer and execute the entire sequence of draws itself.
- **Benefit:** This is the ultimate optimization for batched rendering. It offloads the entire draw loop from the CPU to the GPU, achieving the absolute minimum CPU overhead possible for rendering the scene.
