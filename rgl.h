/**
 * @file rgl.h
 * @brief A High-Performance 2D/3D Renderer with an Integrated Dynamic Lighting and World System.
 *
 * @version 6.1
 * @date June 25, 2025
 *
 * @section overview Overview
 *   rgl.h is a single-header rendering library for the KaOS Engine, designed for high-performance, batched 2D/3D graphics. It provides a flexible API for general-purpose
 *   rendering and includes specialized, powerful systems for creating data-driven worlds, complete with dynamic lighting, procedural Paths, and structured levels. It is ideal
 *   for projects ranging from modern UI applications to complex, retro-inspired arcade games.
 *
 * @section key_features Key Features
 *   - **Unified Lighting Engine:** A powerful, UBO-driven lighting system supporting Point, Directional, and Spot lights. All 3D geometry is dynamically lit with per-pixel diffuse lighting.
 *   - **True 3D Rendering Pipeline:** All geometry is rendered as true 3D primitives with correct perspective, depth, and lighting calculations.
 *   - **High-Performance Batching:** Automatically minimizes GPU state changes and draw calls by sorting and batching thousands of commands, with dynamic buffer growth to prevent overflows.
 *   - **Extensible, Data-Driven World Systems:**
 *     - **Multi-Path System:** Define complex, spline-based path networks with topological junctions (forks, merges, crossroads). The visual appearance of paths is fully customizable via
 *         a callback-based "style" system, allowing for roads, rivers, rollercoasters, and more out of the box.
 *     - **Extensible Scenery System:** Attach scenery to paths with custom, user-defined drawing logic. A global registry allows you to add new types of scenery (e.g., animated signs,
 *         interactive elements) and control how they are rendered.
 *     - **Structured Level System:** Build classic "Doom-style" levels from vertices, walls, and flats, with support for non-convex shapes and full dynamic lighting.
 *   - **Comprehensive API:** Includes low-level primitives (`RGL_DrawSpritePro`), high-level systems (`RGL_DrawPath`, `RGL_DrawLevel`), powerful world query functions (`RGL_QueryJunction`),
 *         and debug tools (`RGL_SetDebugDrawTriggers`).
 *   - **Retro Aesthetics Toolkit:** A rich suite of YPQ color space functions for emulating classic CRT/NTSC visual styles, fully compatible with the modern rendering pipeline.
 *
 * @section path_system_philosophy Path & Scenery System Philosophy
 *   The world-building systems in rgl.h are designed to be powerful, data-driven, and, most importantly, extensible. The core principle is the **separation of data from presentation**.
 *   - **Paths are Just Data:** A "Path" in rgl.h is a purely mathematical concept—a 3D spline with associated data like width, banking, and scenery. By itself, it has no visual appearance.
 *   - **Styles Define Appearance:** The visual look of a path is determined by an `RGLPathStyle`. This is a struct containing a function pointer to a master drawing function. `rgl.h` provides
 *       a default style for drawing classic roads (`RGL_DrawPathAsRoad`), but users are encouraged to write their own drawing functions to render paths as anything they can imagine: rivers,
 *       castle walls, sci-fi energy conduits, etc. You can assign different styles to different paths using `RGL_SetPathStyle()`.
 *   - **Scenery is also Extensible:** In the same way, the appearance of "scenery" attached to a path is not fixed. Using `RGL_RegisterSceneryStyle()`, you can define how any `RGLSceneryType`
 *       is rendered, or even create your own custom scenery types for things like animated signs, particle emitters, or interactive objects.
 *   - **Junctions are Topological:** The path system supports true path networks. A `RGL_SCENERY_JUNCTION_TRIGGER` is not just a visual signpost; it's a topological link between different
 *        named paths. The `RGL_QueryJunction()` function allows game logic to robustly navigate these networks, enabling features like highway off-ramps, branching dungeon corridors, or
 *        complex track-switching.
 *
 * @section recommended_workflow The Render-Pass Workflow (The Painter's Algorithm)
 *   To achieve a correctly rendered scene, draw calls must be layered in a specific order.
 *   1.  **Initialize & Build:** Call `RGL_Init()`. Register any custom Path or Scenery styles. Build your world geometry using `RGL_CreatePath()`, `RGL_CreateLevel()`, etc.
 *   2.  **Game Loop - Update State:** Update your camera and dynamic objects. Use `RGL_QueryJunction()` to handle path switching logic.
 *   3.  **Game Loop - Render Scene (`RGL_Begin`/`RGL_End` block):**
 *       - **Pass 1: Opaque Geometry (The "Depth Pass")**
 *         Draw all solid, non-transparent geometry first to populate the depth buffer.
 *         - `RGL_SetCamera3D(...)`
 *         - `RGL_DrawLevel()`
 *         - `RGL_DrawPath(...)` // Or the RGL_DrawPathAsRoad() wrapper
 *         - `RGL_DrawMesh(...)`
 *
 *       - **Pass 2: Shadows (The "Stencil Pass")**
 *         Cast all your shadows onto the now-solid world.
 *         - `RGL_CastStencilShadowFromMesh(...)`
 *         - `RGL_DrawSpriteWithShadow(...)`
 *
 *       - **Pass 3: Transparent Geometry (The "Alpha Pass")**
 *         Draw alpha-blended objects like sprites and particles.
 *         - `RGL_DrawBillboard(...)`
 *
 *       - **Pass 4: UI / Overlay (The "HUD Pass")**
 *         Switch to a 2D camera to draw your UI.
 *         - `RGL_PushMatrix()`, `RGL_SetCamera2D(...)`, `RGL_DrawText(...)`, `RGL_PopMatrix()`
 *
 * @example (A Sci-Fi Scene with a Custom Path Style and Junctions)
 *   // --- In MyGame_Init() ---
 *   void DrawEnergyConduit(float player_z, int dist, void* data); // My custom drawing function
 *   RGLPathStyle conduit_style = { .draw_path_func = DrawEnergyConduit, .user_data = NULL };
 *   
 *   RGL_CreatePath("MainConduit");
 *   RGL_SetPathStyle("MainConduit", &conduit_style); // Assign my custom style
 *   // ... add points to MainConduit ...
 *
 *   RGL_CreatePath("SideTunnel"); // A different path
 *   // ... add points to SideTunnel ...
 *
 *   // At Z=500 on MainConduit, add a junction trigger to fork left into the SideTunnel
 *   RGLPathPoint p = { .world_z = 500, ... };
 *   p.scenery_left.type = RGL_SCENERY_JUNCTION_TRIGGER;
 *   p.scenery_left.data.junction.type = RGL_JUNCTION_FORK_EXIT;
 *   strncpy(p.scenery_left.data.junction.connect_left.path_name, "SideTunnel", 31);
 *   p.scenery_left.data.junction.connect_left.z_pos = 0.0f;
 *   RGL_AddPathPoint("MainConduit", p);
 *
 *
 *   // --- In MyGame_Update() ---
 *   RGLJunctionInfo junction;
 *   // Check if the player is turning left near a junction
 *   if (player_is_turning_left && RGL_QueryJunction(g_player_z, 10.0f, &junction)) {
 *       if (junction.choice_left.path_name[0] != '\0') {
 *           RGL_SetActivePath(junction.choice_left.path_name);
 *           g_player_z = junction.choice_left.z_pos;
 *       }
 *   }
 *
 *
 *   // --- In MyGame_Render() ---
 *   RGL_Begin(-1);
 *       RGL_SetCamera3D(camera_pos, camera_target, camera_up, 75.0f);
 *
 *       // --- PASS 1: OPAQUE GEOMETRY ---
 *       // This one call will use our custom DrawEnergyConduit function because we set the style!
 *       RGL_DrawPath(g_player_z, 300);
 *
 *       // ... other passes ...
 *   RGL_End();
 *
 * @section workflow_3d_objects Custom 3D Object Workflow
 *   Beyond the high-level world systems, rgl.h provides a powerful pipeline for loading, transforming, and rendering custom 3D models. This workflow is essential for player characters,
 *   vehicles, items, and any other dynamic entity in your scene.
 *
 *   1.  **Load Assets at Startup:** The most important principle for performance is to load your 3D model and texture data only once, for example, during your game's initialization.
 *       - Use `RGL_LoadMeshFromFile()` to load a .obj file into a persistent `RGLMesh` object. This function parses the geometry and uploads it to the GPU.
 *       - Use `RGL_LoadTexture()` to load the corresponding texture.
 *       - Store these `RGLMesh` and `RGLTexture` handles in your game's object structures.
 *
 *   2.  **Define Your Game Object:** In your game's code, create a struct to represent your 3D object. This struct will hold the asset handles and the object's state.
 *       @code
 *       typedef struct {
 *           RGLMesh mesh_handle;
 *           RGLTexture texture_handle;
 *           vec3 position;
 *           vec3 rotation_eul_deg;
 *           vec3 scale;
 *           
 *           mat4 final_transform; // This will be calculated each frame
 *       } My3DObject;
 *       @endcode
 *
 *   3.  **Update State & Transform (In Game Loop):** Each frame, update your object's position, rotation, and scale based on player input or AI. Then, combine these
 *       into a single `mat4` transformation matrix.
 *       @code
 *       // In your Update() function:
 *       My3DObject* player_ship;
 *       // ... update player_ship->position from input ...
 *       // Calculate its final transform matrix for this frame
 *       glm_mat4_identity(player_ship->final_transform);
 *       glm_translate(player_ship->final_transform, player_ship->position);
 *       glm_euler_to_mat4((vec3){rad(rot.x), rad(rot.y), rad(rot.z)}, rotation_mat);
 *       glm_mat4_mul(player_ship->final_transform, rotation_mat, player_ship->final_transform);
 *       glm_scale(player_ship->final_transform, player_ship->scale);
 *       @endcode
 *
 *   4.  **Render Using the Painter's Algorithm (In Render Loop):** Follow the correct render-pass order to ensure correct lighting and shadowing.
 *       - **Pass 1: Opaque Geometry:** Draw your solid 3D object. This renders it to the screen and populates the depth buffer.
 *         - `RGL_DrawMesh(player_ship->mesh_handle, player_material, player_ship->texture_handle, player_ship->final_transform);`
 *       - **Pass 2: Shadows:** Use the same mesh and transform to cast a shadow. The library uses the mesh's CPU-side vertex data to generate the shadow volume.
 *         - `RGL_CastStencilShadowFromMesh(player_ship->mesh_handle, player_ship->final_transform, &shadow_config);`
 *
 *   5.  **Cleanup:** When the object is no longer needed (e.g., at level unload), be sure to free the resources to prevent memory leaks.
 *       - `RGL_DestroyMesh(&my_object.mesh_handle);`
 *       - `RGL_UnloadTexture(my_object.texture_handle);`
 *
 * @example (Rendering a Player Ship with a Stencil Shadow)
 *   // --- In Render Function ---
 *   RGL_Begin(-1);
 *       RGL_SetCamera3D(...);
 *
 *       // --- PASS 1: OPAQUE ---
 *       RGL_DrawLevel();
 *       // Draw the player ship. It is now part of the lit, solid world.
 *       RGL_DrawMesh(g_player.mesh, g_player.material, g_player.texture, g_player.transform);
 *       
 *       // --- PASS 2: SHADOWS ---
 *       RGLShadowConfig config = { .light_id = g_sun_light, ... };
 *       // The player ship now casts a shadow onto the level.
 *       RGL_CastStencilShadowFromMesh(g_player.mesh, g_player.transform, &config);
 *
 *       // --- PASS 3 & 4: TRANSPARENT & UI ---
 *       RGL_DrawParticles();
 *       // ... draw UI ...
 *   RGL_End();
 */
 /**
 * @section opengl_4_6_migration_plan OpenGL 4.6 Modernization Roadmap (Future)
 *   rgl.h was originally designed with an OpenGL 3.3 feature set, which is robust and widely compatible. To maximize performance on modern hardware and align with the
 *   capabilities of situation.h, a future version of rgl.h will be migrated to leverage core OpenGL 4.6 features. This migration will focus on reducing CPU overhead,
 *   minimizing driver work, and offloading tasks to the GPU.
 *
 *   This section outlines the planned improvements, which will be implemented after the migration to lib_tex.h is complete.
 *
 *   @subsection opengl_4_6_step1 Step 1: Adopting the "Shader Contract" and UBOs
 *     - **Problem:** The current renderer uses `glGetUniformLocation()` at runtime to find shader uniforms. This involves string comparisons and can be a performance bottleneck.
 *     - **Solution:** The shaders will be updated to use explicit locations and Uniform Buffer Objects (UBOs), fully adopting the "Shader Contract" established by situation.h.
 *       - **Uniforms:** `uniform mat4 view;` will become `layout (location = N) uniform mat4 model;`.
 *       - **UBOs:** The renderer will bind to the `ViewData` UBO provided by situation.h (at `binding = 1`) to get camera matrices, and will continue to use its own UBO for lighting data (at `binding = 0`).
 *     - **Benefit:** Eliminates all runtime string lookups for uniforms and allows large data blocks like camera matrices to be updated on the GPU in a single, efficient call.
 *
 *   @subsection opengl_4_6_step2 Step 2: Full Integration of Direct State Access (DSA)
 *     - **Problem:** The current code relies heavily on the "bind-to-edit" pattern (e.g., `glBindTexture`, then `glTexParameteri`). This forces the driver to constantly re-validate state.
 *     - **Solution:** All state modification calls will be converted to their DSA equivalents, which operate directly on an object's ID without changing the currently bound object.
 *       - `glBindTexture` + `glTexParameter*` -> `glTextureParameter*`
 *       - `glBindBuffer` + `glBufferData` -> `glNamedBufferData`
 *       - `glGenerateMipmap` -> `glGenerateTextureMipmap`
 *     - **Benefit:** Dramatically reduces driver overhead and API calls, leading to cleaner code and higher performance by avoiding modification of the global GL state machine.
 *
 *   @subsection opengl_4_6_step3 Step 3: High-Throughput Batching with Persistent Buffers
 *     - **Problem:** The current batch renderer (`_RGL_FlushBatch`) assembles a vertex buffer on the CPU and then copies it to the GPU with `glBufferSubData` every time it flushes. This per-flush copy can cause CPU/GPU synchronization stalls.
 *     - **Solution:** The batching system will be migrated to use a **persistently mapped buffer**.
 *       - At initialization, the VBO will be created with `GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT`.
 *       - It will be mapped **once** using `glMapNamedBufferRange`, and the returned pointer will be stored.
 *       - During `_RGL_FlushBatch`, vertex data will be written **directly** to this pointer, which points to GPU-accessible memory.
 *     - **Benefit:** Completely eliminates the per-flush `glBufferSubData` copy from CPU to GPU memory, significantly improving throughput and reducing stalls.
 *
 *   @subsection opengl_4_6_step4 Step 4: Offloading Draw Loops with Multi-Draw Indirect (MDI)
 *     - **Problem:** The `_RGL_FlushBatch` function currently loops through the sorted commands on the CPU and issues multiple `glDrawArrays` calls. This CPU loop is an overhead that can be eliminated.
 *     - **Solution:** The renderer will be updated to use `glMultiDrawArraysIndirect`.
 *       - In `_RGL_FlushBatch`, instead of a C loop, the renderer will build a small array of `DrawArraysIndirectCommand` structs.
 *       - This small command buffer will be uploaded to a dedicated GPU buffer (an "indirect buffer").
 *       - A **single** call to `glMultiDrawArraysIndirect` will tell the GPU to read the commands from the buffer and execute the entire sequence of draws itself.
 *     - **Benefit:** This is the ultimate optimization for batched rendering. It offloads the entire draw loop from the CPU to the GPU, achieving the absolute minimum CPU overhead possible for rendering the scene.
 */
#ifndef RGL_H
#define RGL_H

#include "situation.h" // Must be included first for SITAPI, types, and error handling.
#include "lib_tex.h"
#include <cglm/cglm.h>
#include <glad/glad.h>

#include "dynamo.h"

// Pre-flight check to ensure situation.h was included and SITAPI is defined.
#ifndef SITAPI
    #error "rgl.h requires situation.h to be included first."
#endif

// --- Configuration ---
#define RGL_DEFAULT_BATCH_CAPACITY 8192
#define RGL_MAX_BATCH_CAPACITY 65536
#define RGL_DEFAULT_FOV_DEGREES 60.0f
#define RGL_DEFAULT_NEAR_PLANE 0.1f
#define RGL_DEFAULT_FAR_PLANE 3000.0f
#define RGL_SHAPE_SEGMENTS 36

// RGLTexture is an alias for the more robust LTTexture from lib_tex.h
typedef LTTexture RGLTexture;

/**
 * @brief A handle to a managed 3D mesh object containing GPU and CPU data.
 * This is the definitive, corrected structure.
 */
typedef struct {
    int id; // Publicly readable ID. 0 means invalid/unloaded.
    
    // Publicly readable data
    int vertex_count;
    int index_count;

    // The handle to the underlying GPU resources, managed by situation.h
    SituationMesh gpu_mesh; 

    // CPU-side data, owned by RGL, for shadows/physics/exporting.
    // NOTE: This duplicates data but decouples RGL's CPU logic from the GPU layout.
    // This is a valid and often necessary design choice.
    vec3* cpu_vertices;
    vec2* cpu_texcoords;
    vec3* cpu_normals;
    unsigned int* cpu_indices;
} RGLMesh;

// --- Public Types and Structs ---

/**
 * @brief Represents a drawable sprite: a texture and a sub-rectangle.
 * This is the primary object used for all 2D texture drawing functions.
 */
typedef struct {
    RGLTexture texture;
    Rectangle source_rect;
} RGLSprite;

/**
 * @brief Defines the properties of a single particle managed by the RGL particle system.
 */
typedef struct {
    DynamoBody physics_body; // A particle IS a physics body
    RGLSprite sprite;
    vec2 size;
    Color tint;
    float lifetime;         // How long the particle lives, in seconds.
    float rotation_speed;   // How fast it spins, in degrees per second.
    float current_rotation;
    bool is_active;
} RGLParticle;

/**
 * @brief Defines the properties for spawning a burst of particles.
 * Used with RGL_EmitParticles().
 */
typedef struct {
    vec3 position;
    vec3 velocity_range_min, velocity_range_max;  // Random velocity bounds
    float spawn_rate;           // Particles per second
    float particle_lifetime;    // How long each particle lives
    RGLSprite base_sprite;      // Template sprite for new particles
    Color tint_start, tint_end; // Color fade over lifetime
    vec3 size_start, size_end;  // Size change over lifetime
    vec3 gravity_direction;  // Not just down - could be any 3D direction!
    int max_active;            // Max particles this emitter can have
} RGLParticleEmitter;

// --- Font Types ---
typedef struct {
    RGLTexture atlas_texture;
    int char_width;
    int char_height;
    int chars_per_row;
    int chars_per_col;
    int first_char;        // ASCII code of first character (usually 32 for space)
    int char_count;        // Number of characters in atlas
    float char_spacing;    // Additional spacing between characters
    float line_spacing;    // Additional spacing between lines
} RGLBitmapFont;

/**
 * @brief Configuration for packed bitmap font loading
 */
typedef struct {
    int char_width;           // Character width in pixels
    int char_height;          // Character height in data rows  
    int display_height;       // Total display height including padding
    int char_count;           // Number of characters in font
    int first_char;           // ASCII code of first character
    int chars_per_row;        // Characters per row in source data (for 2D layouts)
    
    // Bit layout configuration
    int bits_per_row;         // Total bits per row entry (16, 32, etc.)
    int data_bits;            // Actual data bits used per row (12, 8, etc.)
    int data_bit_offset;      // Offset to data bits (0 = LSB aligned, >0 = MSB aligned)
    bool bit_order_msb_first; // true = MSB is leftmost pixel, false = LSB is leftmost
    
    // Padding configuration  
    int top_padding;          // Empty rows at top
    int bottom_padding;       // Empty rows at bottom
    int left_padding;         // Empty pixels at left
    int right_padding;        // Empty pixels at right
    
    // Atlas configuration
    int atlas_chars_per_row;  // Characters per row in output atlas (0 = auto)
    int atlas_chars_per_col;  // Characters per col in output atlas (0 = auto)
    
    bool enable_outline;      // Enable outline generation
    int outline_thickness;    // Outline thickness in pixels (1-3 recommended)
    unsigned char outline_r;  // Outline color - Red (0-255)
    unsigned char outline_g;  // Outline color - Green (0-255)  
    unsigned char outline_b;  // Outline color - Blue (0-255)
    unsigned char outline_a;  // Outline color - Alpha (0-255)
    unsigned char font_r;     // Font color - Red (0-255, default 255)
    unsigned char font_g;     // Font color - Green (0-255, default 255)
    unsigned char font_b;     // Font color - Blue (0-255, default 255)
    unsigned char font_a;     // Font color - Alpha (0-255, default 255)
} RGLPackedFontConfig;

typedef struct {
    RGLTexture atlas_texture;
    Rectangle char_rects[256];  // UV rectangles for each character
    vec2 char_offsets[256];     // Offset from baseline for each char
    float char_advances[256];   // How far to advance cursor after each char
    float font_size;           // Original font size
    float line_height;         // Height of a line
    float baseline;            // Distance from top to baseline
} RGLTrueTypeFont;

#define RGL_MAX_LIGHTS 64
#define MAX_SHADER_LIGHTS 32
#define RGL_LIGHT_CULLING_BIAS 30.0f // Keep lights active up to 30 units behind the camera

/**
 * @brief Defines the type of a light source.
 */
typedef enum {
    RGL_LIGHT_TYPE_POINT = 1,
    RGL_LIGHT_TYPE_DIRECTIONAL = 2,
    RGL_LIGHT_TYPE_SPOT = 3
} RGLLightType;

/**
 * @brief Defines a dynamic light in the world. Can be a point, directional, or spot light.
 */
typedef struct {
    int id;                 // Unique identifier (1-based)
    bool is_active;
    RGLLightType type;

    vec3 position;          // For Point, Spot
    vec3 direction;         // For Directional, Spot

    Color color;
    float intensity;        // Brightness multiplier

    // Type-specific parameters
    float radius;           // For Point, Spot: The maximum distance the light can reach
    float spot_outer_angle; // For Spot: Outer cone angle in degrees (hard cutoff)
    float spot_inner_angle; // For Spot: Inner cone angle in degrees (start of falloff)

    float culling_bias;     // Per-light distance to keep it active behind camera
} RGLLight;

typedef struct {
    int light_id;
    Color color;
    float extrusion_length;
} RGLShadowConfig;

/**
 * @brief Simple material properties for 3D rendering
 */

typedef struct {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
} RGLVertex3D;

typedef struct {
    Color diffuse;  // Diffuse color
    float ambient; // Ambient light factor (0.0 to 1.0)
} RGLMaterial;

typedef struct {
    float x, y, z; // 3D position
} RGLVertex3D_pos;

typedef struct {
    int start_vertex; // Index into level->vertices
    int end_vertex;
    float bottom_y; // Bottom height
    float top_y;    // Top height
    RGLSprite texture; // Wall texture
    float u_scale; // Horizontal texture scale
    float v_scale; // Vertical texture scale
    float brightness; // Lighting factor (0.0 to 1.0)
    int tag; // For triggers (e.g., doors, switches)
} RGLWall;

typedef struct {
    int* vertex_indices; // Indices into level->vertices
    size_t vertex_count;
    float y; // Height (floor or ceiling)
    RGLSprite texture; // Flat texture
    float u_scale; // Texture scale
    float v_scale;
    float brightness; // Lighting factor (0.0 to 1.0)
    int tag; // For triggers (e.g., moving platforms)
} RGLFlat;

typedef struct {
    float x, y, z; // 3D position
    RGLSprite texture; // Sprite texture
    float scale; // Size multiplier
    int frame; // Animation frame (0 for static)
    float brightness; // Lighting factor (0.0 to 1.0)
    int tag; // For gameplay (e.g., enemy type)
    int attached_light_id; // ID of an RGLLight, or 0 if none
} RGLThing;

typedef struct {
    char name[32];
    
    // --- World Transform for the entire level ---
    vec3 position;              // The world-space origin (X, Y, Z) of the level.
    vec3 rotation_eul_deg;      // The world-space rotation {pitch, yaw, roll} of the level.
    
    RGLVertex3D_pos* vertices;
    
    size_t vertex_count;
    size_t vertex_capacity;
    RGLWall* walls;
    size_t wall_count;
    size_t wall_capacity;
    RGLFlat* flats;
    size_t flat_count;
    size_t flat_capacity;
    RGLThing* things;
    size_t thing_count;
    size_t thing_capacity;
} RGLLevel;

// --- Path & Path System Types ---

#define RGL_MAX_SCENERY_TYPES 256 // Define a max number of styles we can register

/** @brief Defines the type of a scenery object attached to a path point. */
typedef enum {
    RGL_SCENERY_NONE = 0,
    RGL_SCENERY_SPRITE,             // A standard billboard sprite (e.g., tree, sign).
    RGL_SCENERY_ARCH,               // A fixed 3D quad for tunnels, bridges, or walls.
    RGL_SCENERY_EVENT_MARKER,       // An invisible trigger for generic gameplay logic (e.g., play sound).
    RGL_SCENERY_JUNCTION_TRIGGER,   // An invisible trigger for path-switching logic.
    RGL_SCENERY_LEVEL_ENTRANCE,     // A Level Entrance checkpoint
    RGL_SCENERY_LIGHT_SOURCE,
    RGL_SCENERY_CUSTOM = 100 // Users can define their own types like:
                             // #define MY_NEON_SIGN (RGL_SCENERY_CUSTOM + 0)
} RGLSceneryType;

/**
 * @brief Defines a visual style for rendering a specific type of scenery.
 */
typedef struct {
    RGLSceneryDrawCallback draw_func;
    void* user_data;
} RGLSceneryStyle;

/**
 * @brief Defines the geometric arrangement of a path junction.
 * This determines how paths connect and what player choices are available.
 */
typedef enum {
    RGL_JUNCTION_FORK_EXIT, /** @brief A one-way fork where the player can choose to exit the main path. (e.g., Highway Off-Ramp) */
    RGL_JUNCTION_MERGE_JOIN, /** @brief A one-way merge where a side path joins the main path. (e.g., Highway On-Ramp) */
    RGL_JUNCTION_T_INTERSECTION, /** @brief A classic 3-way intersection where the path ends and the player must turn left or right. */
    RGL_JUNCTION_CROSSROADS /** @brief A 4-way intersection where the player can go left, right, or straight. */
} RGLJunctionType;

/** @brief A union holding data for different scenery types. */
typedef union {
    struct { RGLSprite sprite; vec2 size_in_world_units; } visual; /** @brief Data for visual scenery like sprites (RGL_SCENERY_SPRITE) or fixed geometry (RGL_SCENERY_ARCH). */
    struct { char name[32]; int32_t id; } event; /** @brief Data for invisible gameplay triggers (RGL_SCENERY_EVENT_MARKER). */
    struct { /** @brief Data for defining a topological connection between paths (RGL_SCENERY_JUNCTION_TRIGGER). */
        RGLJunctionType type; /** @brief The geometric type of the junction, which dictates the available choices. */
        struct { char path_name[32]; float z_pos; } connect_left; /** @brief Defines the connection for turning left. Set path_name to "" if no left turn is possible. */
        struct { char path_name[32]; float z_pos; } connect_right; /** @brief Defines the connection for turning right. Set path_name to "" if no right turn is possible. */
        struct { char path_name[32]; float z_pos; } connect_straight; /** @brief Defines the connection for going straight. Set path_name to "" if going straight is not possible (e.g., at a T-Intersection). */
    } junction;
    struct { char target_level_name[32]; int entrance_id; } level_portal; /** @brief Data for connecting the path system to a level system (RGL_SCENERY_LEVEL_ENTRANCE). */
    struct { Color color; float radius; float intensity; int light_id; } light; /** @brief Data for attaching a dynamic light source to the path (RGL_SCENERY_LIGHT_SOURCE). */
                                                 // This is a runtime-only field, set to 0 in data.
} RGLSceneryData;

/** @brief Defines a single piece of scenery attached to the path. */
typedef struct {
    RGLSceneryType type;
    float x_offset;        ///< Relative X offset from the Path's centerline.
    float y_offset;        ///< Relative Y offset from the Path surface.
    RGLSceneryData data;   ///< Type-specific data.
} RGLScenery;

/**
 * @brief Holds the complete state of a path junction returned by a query.
 * This struct contains all possible connections (left, right, straight) available at the junction.
 */
typedef struct {
    bool is_valid;                 // True if a junction was found at the query location.
    RGLJunctionType type;          // The geometric type of the junction.

    // Connection info for each possible choice. A path_name[0] == '\0' indicates no valid choice in that direction.
    struct { char path_name[32]; float z_pos; } choice_left;
    struct { char path_name[32]; float z_pos; } choice_right;
    struct { char path_name[32]; float z_pos; } choice_straight;

} RGLJunctionInfo;

/**
 * @brief Defines a single control point for a Path's geometry and appearance.
 * The Path is constructed by connecting a series of these points.
 */
typedef struct {
    // --- Core Geometry ---
    float world_z;
    float world_x_offset;
    float world_y_offset;

    // --- Banking Control ---
    float path_roll_degrees;    // Bank angle of the Path surface in degrees.

    // --- Lane and Width System ---
    float primary_ribbon_width;        // Width of the main Path segment.
    int   primary_lanes;        // Number of lanes on the main Path.
    
    // --- Path Splitting System ---
    float split_offset;         // Horizontal offset of the split Path's centerline FROM THE PRIMARY CENTERLINE.
    float split_width;          // Width of the split-off Path segment. 0 means no split.
    int   split_lanes;          // Number of lanes on the split Path.

    // --- Appearance ---
    RGLSprite surface_texture;     // Texture for the main Path. If id=0, uses color_surface.
    Color color_surface;           // Primary Path surface color.
    Color color_rumble;         // Rumble strip/shoulder color.
    Color color_lines;          // Lane marking color.
    float rumble_width;         // Width of the shoulder on the outside of all lanes.

    // Appearance for Split Path
    RGLSprite split_surface_texture;  // Texture for the split Path. If id=0, uses split_surface_color.
    Color split_surface_color;        // Fallback color for the split Path surface.
    // You could add split_color_rumble, split_color_lines, split_rumble_width if needed,
    // but let's keep it simple for now and share rumble/line styles.
    
    // --- Scenery System ---
    RGLScenery scenery_left;
    RGLScenery scenery_right;
    RGLScenery scenery_overhead;

    // --- Gameplay Tag ---
    int32_t user_tag;
} RGLPathPoint;

/** @brief Holds information about the ground returned by a query. */
typedef struct {
    bool is_hit;
    enum { RGL_GROUND_TYPE_NONE, RGL_GROUND_TYPE_PATH, RGL_GROUND_TYPE_SHOULDER, RGL_GROUND_TYPE_OFF_PATH } type;
    float ground_y;
    vec3 surface_normal;
} RGLGroundInfo;

/** @brief Holds information about a found event marker. */
typedef struct {
    char name[32];
    int32_t id;
    float distance;
    vec3 world_pos;
} RGLMarkerInfo;

typedef struct {
    Color bg_dark_gray;
    Color grid_white;
    Color bar_light_gray;
    Color bar_yellow;
    Color bar_cyan;
    Color bar_green;
    Color bar_magenta;
    Color bar_red;
    Color bar_blue;
    Color bar_black;
    Color bar_white;
    Color bar_mid_gray;
    Color bar_dark_gray;
    Color bar_orange;
} RGLTestPatternColors;

// Add to your public enums
typedef enum {
    RGL_TESTPATTERN_SMPTE_BARS,     // Classic color bars
    RGL_TESTPATTERN_CHECKERBOARD,   // Resolution/scaling
    RGL_TESTPATTERN_CONVERGENCE,    // Stripe/moire
    RGL_TESTPATTERN_GRADIENTS,      // Color banding
    RGL_TESTPATTERN_GRID_ONLY,      // Background grid
    RGL_TESTPATTERN_PLUGE,          // Black level calibration
    RGL_TESTPATTERN_CROSSHATCH,
    RGL_TESTPATTERN_MULTIBURST,      // Frequency response
    RGL_TESTPATTERN_3D_GRID
} RGLTestPatternType;

// Add the main config struct
typedef struct {
    RGLTestPatternType type;    // Which test pattern to draw
    int width;                  // Target width
    int height;                 // Target height
    
    // General options
    bool show_overlay_circle;   // For SMPTE bars
    
    // Pattern-specific options
    vec2 checker_size;          // For CHECKERBOARD pattern
    float stripe_width;         // For CONVERGENCE pattern
    const float* frequencies;  // For MULTIBURST (array of MHz values)
    int num_frequencies;       // Number of frequency bands
    int grid_size; // For 3D_GRID (e.g., 5 for 5x5 grid)
    
    // Color palette (optional, can be NULL to use defaults)
    const RGLTestPatternColors* colors;
} RGLTestPatternConfig;

typedef struct {
    int active_lights_per_frame;
    float light_ubo_upload_time_ms;
    int downward_shadows_drawn;
    int stencil_volumes_drawn;
} RGLStats;

// --- API Function Declarations ---

//==================================================================================
// Callback Function Types
//==================================================================================
typedef void (*RGLPathSegmentDrawCallback)(const RGLPathPoint* p_near, const RGLPathPoint* p_far, const vec3* normal, void* user_data); // Defines a function that can draw a single segment of a path.
typedef void (*RGLSceneryDrawCallback)(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data); // Defines a function that can draw a single scenery object.
//==================================================================================
// Core Module: Lifecycle, State, and Frame Management
//==================================================================================
SITAPI bool RGL_Init(void);                                                 // Initializes the renderer; must be called after SituationInit().
SITAPI void RGL_Shutdown(void);                                             // Shuts down the renderer and frees all associated resources.
SITAPI void RGL_Begin(int virtual_display_id);                              // Begins a new render frame, targeting a specific virtual display (-1 for main screen).
SITAPI void RGL_End(void);                                                  // Ends the render frame, flushing all batched commands.
SITAPI void RGL_SetTransform(mat4 transform);                               // Sets a global 3D model matrix for subsequent RGL_Draw... calls.
SITAPI void RGL_ResetTransform(void);                                       // Resets the global 3D model matrix to identity.
//==================================================================================
// Camera & View Module
//==================================================================================
SITAPI void RGL_SetCamera2D(vec2 target, float rotation_degrees, float zoom); // Configures an orthographic camera for 2D rendering.
SITAPI void RGL_SetCamera3D(vec3 position, vec3 target, vec3 up, float fov_y_degrees); // Configures a perspective camera for 3D rendering.
SITAPI void RGL_PushMatrix(void);                                           // Pushes the current camera matrices onto a stack, saving the current view.
SITAPI void RGL_PopMatrix(void);                                            // Pops the last saved camera matrices from the stack, restoring the view.
SITAPI void RGL_GetViewMatrix(mat4 out_view);                               // Gets a copy of the current view matrix.
SITAPI void RGL_GetProjectionMatrix(mat4 out_proj);                         // Gets a copy of the current projection matrix.
SITAPI vec2 RGL_WorldToScreen(vec3 world_pos);                              // Projects a 3D world-space coordinate to a 2D screen-space coordinate.
SITAPI vec3 RGL_ScreenToWorld(vec2 screen_pos, float z_depth);              // Un-projects a 2D screen-space coordinate back into the 3D world.
SITAPI Rectangle RGL_GetScreenRect(void);                                   // Returns a rectangle representing the current rendering viewport.
//==================================================================================
// World Systems: Path Management
//==================================================================================
SITAPI bool RGL_CreatePath(const char* path_name);                          // Creates a new, empty, named path for building a world.
SITAPI void RGL_AddPathPoint(const char* path_name, RGLPathPoint point);    // Adds a new control point to the end of a named path.
SITAPI bool RGL_SetPathStyle(const char* path_name, const RGLPathStyle* style); // Assigns a custom drawing style to a named path.
SITAPI bool RGL_SetActivePath(const char* path_name);                       // Sets the currently active path for all drawing and query functions.
SITAPI bool RGL_SetPathLooping(const char* path_name, float z_pos);         // Configures a path to loop back to a specific Z-position.
SITAPI bool RGL_DestroyPathByName(const char* path_name);                   // Destroys a named path and frees its memory.
SITAPI void RGL_DrawPath(float player_z, int draw_distance);                // Draws the active path using its currently assigned custom style.
SITAPI void RGL_DrawPathAsRoad(float player_z, int draw_distance);          // Convenience wrapper to draw the active path as a classic road.
SITAPI void RGL_DrawPathAsMap(RGLTexture target, vec2 center_pos_xz, float world_width, Color bg_color); // Renders a top-down 2D map of the active path to a texture.
SITAPI const RGLPathStyle* RGL_GetDefaultRoadStyle(void);                   // Gets a pointer to the built-in road style, for use with RGL_SetPathStyle.
//==================================================================================
// World Systems: Level Management
//==================================================================================
SITAPI bool RGL_CreateLevel(const char* level_name);                        // Creates a new, empty, named level for building geometry.
SITAPI int RGL_AddVertex(const char* level_name, RGLVertex3D_pos vertex);   // Adds a 2D vertex (for XZ plane) to a named level.
SITAPI bool RGL_AddWall(const char* level_name, RGLWall wall);              // Adds a vertical wall segment to a named level.
SITAPI bool RGL_AddFlat(const char* level_name, RGLFlat flat);              // Adds a horizontal floor or ceiling polygon to a named level.
SITAPI bool RGL_AddThing(const char* level_name, RGLThing thing);           // Adds a billboard-style object (e.g., enemy, item) to a named level.
SITAPI bool RGL_SetActiveLevel(const char* level_name);                     // Sets the currently active level for drawing.
SITAPI bool RGL_DestroyLevelByName(const char* level_name);                 // Destroys a named level and frees all its associated geometry data.
SITAPI void RGL_DrawLevel(void);                                            // Draws the active level, including walls, flats, and things, with dynamic lighting.
SITAPI bool RGL_PlaceLevelOnPath(const char* level_name, const char* path_name, float path_z, vec3 offset, float yaw_offset_degrees); // Positions a level relative to a point on a path.
SITAPI void RGL_DrawWorld(float camera_z, int Path_draw_distance);          // High-level helper to draw both the active path and all loaded levels.
//==================================================================================
// World Systems: Queries & Scenery
//==================================================================================
SITAPI void RGL_RegisterSceneryStyle(RGLSceneryType type, const RGLSceneryStyle* style); // Registers a custom drawing function for a type of scenery.
SITAPI void RGL_UpdatePathScenery(float player_z, float view_distance);     // Updates dynamic scenery state, such as creating lights from scenery definitions.
SITAPI bool RGL_GetPathPropertiesAt(float z_pos, RGLPathPoint* out_point);  // Gets the interpolated geometry and properties of the active path at a Z-position.
SITAPI bool RGL_GetGroundAt(vec2 world_xz, RGLGroundInfo* out_info);        // Finds the ground surface (Y-position and normal) at a world XZ coordinate.
SITAPI bool RGL_QueryJunction(float player_z, float search_radius, RGLJunctionInfo* out_info); // Queries the active path for a junction trigger and returns its connection info.
SITAPI bool RGL_GetDistanceToMarker(float player_z, const char* marker_name, float* out_distance); // Finds the distance to the next event marker with a specific name.
SITAPI int RGL_FindMarkersInRange(float start_z, float end_z, RGLMarkerInfo out_markers[], int max_markers); // Finds all event markers within a Z-range.
SITAPI int RGL_FindSceneryInRange(float start_z, float end_z, RGLScenery* out_scenery[], int max_scenery); // Finds all scenery objects within a Z-range.
SITAPI int RGL_FindSceneryInRadius(vec3 world_pos, float radius, RGLScenery* out_objects[], int max_objects); // Finds all scenery objects within a 3D radius.
SITAPI vec3 RGL_LevelToWorld(const char* level_name, vec3 local_pos);       // Converts a 3D coordinate from a level's local space to global world space.
SITAPI vec3 RGL_WorldToLevel(const char* level_name, vec3 world_pos);       // Converts a 3D coordinate from global world space to a level's local space.
//==================================================================================
// Mesh & Resource Management
//==================================================================================
SITAPI RGLTexture RGL_LoadTexture(const char* filename, GLenum wrap_mode, GLenum filter_mode); // Loads a texture from a file with basic parameters.
SITAPI RGLTexture RGL_LoadTextureWithParams(const char* filename, const LTTextureParams* params); // Loads a texture from a file with advanced parameters.
SITAPI RGLTexture RGL_CreateRenderTexture(int width, int height);           // Creates a new texture that can be used as a rendering target.
SITAPI void RGL_SetRenderTarget(RGLTexture texture);                        // Sets the current rendering target to a specific texture.
SITAPI void RGL_ResetRenderTarget(void);                                    // Resets the rendering target back to the main screen or virtual display.
SITAPI void RGL_UnloadTexture(RGLTexture texture);                          // Unloads a texture from memory.
SITAPI void RGL_DestroyRenderTexture(RGLTexture texture);                   // Destroys a render texture and its associated framebuffer object.
SITAPI Rectangle RGL_GetTextureRect(RGLTexture texture);                    // Returns a rectangle representing the full dimensions of a texture.
SITAPI RGLMesh RGL_LoadMeshFromFile(const char* filename);                  // Loads a 3D model from a .obj file into a manageable mesh object.
SITAPI RGLMesh RGL_CreateMeshFromLevel(const char* level_name);             // Creates a CPU-only mesh from a level's geometry, for use with stencil shadows.
SITAPI bool RGL_SaveMeshToFile(RGLMesh mesh, const char* filename);         // Saves a mesh's CPU-side geometry to a .obj file.
SITAPI void RGL_DestroyMesh(RGLMesh* mesh);                                 // Frees all CPU and GPU resources associated with a mesh.
//==================================================================================
// Procedural Mesh Generation
//==================================================================================
SITAPI RGLMesh RGL_GenMeshPlane(float width, float length, int subdivisions_x, int subdivisions_z); // Generates a flat plane mesh on the XZ axis.
SITAPI RGLMesh RGL_GenMeshCube(float width, float height, float depth);     // Generates a cube mesh with the specified dimensions.
SITAPI RGLMesh RGL_GenMeshSphere(float radius, int slices, int stacks);     // Generates a sphere mesh.
SITAPI RGLMesh RGL_GenMeshCylinder(float radius, float height, int slices); // Generates a cylinder mesh oriented along the Y-axis.
SITAPI RGLMesh RGL_GenMeshTorus(float major_radius, float tube_radius, int major_segments, int tube_segments); // Generates a torus (donut) mesh.
SITAPI RGLMesh RGL_GenMeshCapsule(float radius, float height, int slices, int stacks); // Generates a capsule mesh (cylinder with hemispherical caps).
SITAPI RGLMesh RGL_GenMeshIcosahedron(float radius);                        // Generates a 20-sided icosahedron mesh.
SITAPI RGLMesh RGL_GenMeshDodecahedron(float radius);                       // Generates a 12-sided dodecahedron mesh.
SITAPI RGLMesh RGL_GenMeshKnot(float major_radius, float tube_radius, int major_segments, int tube_segments); // Generates a trefoil knot mesh.
SITAPI RGLMesh RGL_GenMeshRock(float radius, int subdivisions, int seed);   // Generates a randomized, rocky asteroid-like mesh.
//==================================================================================
// Dynamic Lighting
//==================================================================================
SITAPI void RGL_SetAmbientLight(Color color);                               // Sets the global ambient light color for the entire scene.
SITAPI int RGL_CreatePointLight(vec3 position, Color color, float radius, float intensity); // Creates a new point light that radiates in all directions.
SITAPI int RGL_CreateDirectionalLight(vec3 direction, Color color, float intensity); // Creates a new directional light (e.g., sun) that affects the whole scene.
SITAPI int RGL_CreateSpotLight(vec3 position, vec3 direction, Color color, float radius, float intensity, float outer_angle_deg, float inner_angle_deg); // Creates a new cone-shaped spot light.
SITAPI int RGL_CreatePointLightYPQ(vec3 position, ColorYPQA ypq_color, float radius, float intensity); // Creates a point light using a YPQ color, for retro aesthetics.
SITAPI void RGL_SetLightActive(int light_id, bool active);                  // Activates or deactivates a light.
SITAPI void RGL_SetLightPosition(int light_id, vec3 position);              // Updates the world position of a point or spot light.
SITAPI void RGL_SetLightDirection(int light_id, vec3 direction);            // Updates the direction of a directional or spot light.
SITAPI void RGL_SetLightDirectionFromYPR(int light_id, vec3 ypr_degrees);   // Updates a light's direction using Yaw, Pitch, and Roll angles.
SITAPI void RGL_SetLightColor(int light_id, Color color);                   // Updates the color of a light.
SITAPI void RGL_SetLightIntensity(int light_id, float intensity);           // Updates the intensity (brightness) of a light.
SITAPI void RGL_AnimateLight(int light_id, float time, float frequency, float amplitude); // Applies a simple sinusoidal flicker to a light's intensity.
SITAPI void RGL_DestroyLight(int light_id);                                 // Destroys a light and frees its slot.
//==================================================================================
// Shadow Rendering
//==================================================================================
SITAPI void RGL_CastStencilShadowFromMesh(RGLMesh mesh, mat4 transform, const RGLShadowConfig* config); // Casts a high-quality, perspective-correct stencil shadow from a mesh.
SITAPI void RGL_DrawSpriteWithShadow(RGLSprite sprite, vec3 world_pos, vec2 size, const RGLShadowConfig* config); // Convenience wrapper to cast a stencil shadow from a billboard sprite.
SITAPI void RGL_DrawSpriteWithSimpleShadow(RGLSprite sprite, vec3 world_pos, vec2 size, int light_id); // Simplified helper to cast a default stencil shadow from a light.
SITAPI void RGL_DrawSpriteDownwardShadow(RGLSprite sprite, vec3 world_pos, vec2 size, Color shadow_tint); // Draws a fast, simple "blob" shadow projected vertically onto the ground.
//==================================================================================
// 2D & 3D Primitive Drawing
//==================================================================================
// --- 2D Primitives ---
SITAPI void RGL_DrawPixel(vec2 position, Color color);                      // Draws a single pixel at a 2D coordinate.
SITAPI void RGL_DrawLine(vec2 start, vec2 end, Color color);                // Draws a 1-pixel-thick line between two points.
SITAPI void RGL_DrawLineEx(vec2 start_pos, vec2 end_pos, float thick, Color color); // Draws a line with a specified thickness.
SITAPI void RGL_DrawLineBezier(vec2 start, vec2 end, vec2 control1, vec2 control2, float thickness, Color color); // Draws a smooth, cubic Bezier curve.
SITAPI void RGL_DrawPolyline(vec2* points, int point_count, float thickness, Color color, bool closed); // Draws a series of connected lines.
SITAPI void RGL_DrawRectangle(Rectangle rect, float roll_degrees, Color color); // Draws a color-filled rectangle with optional rotation.
SITAPI void RGL_DrawRectangleOutline(Rectangle rect, float thickness, Color color); // Draws the outline of a rectangle.
SITAPI void RGL_DrawRectangleRounded(Rectangle rect, float roundness, Color color); // Draws a color-filled rectangle with rounded corners.
SITAPI void RGL_DrawRectangleRoundedOutline(Rectangle rect, float roundness, float thickness, Color color); // Draws the outline of a rectangle with rounded corners.
SITAPI void RGL_DrawRectangleGradient(Rectangle rect, Color top_left, Color top_right, Color bottom_left, Color bottom_right); // Draws a rectangle with a smooth, per-vertex color gradient.
SITAPI void RGL_DrawCircle(vec2 center, float radius, Color color);         // Draws a color-filled circle.
SITAPI void RGL_DrawCircleOutline(vec2 center, float radius, float thickness, Color color); // Draws the outline of a circle.
SITAPI void RGL_DrawEllipse(vec2 center, vec2 radii, Color color);          // Draws a color-filled ellipse.
SITAPI void RGL_DrawRing(vec2 center, float inner_radius, float outer_radius, Color color); // Draws a color-filled ring (donut shape).
SITAPI void RGL_DrawArc(vec2 center, float radius, float start_angle, float end_angle, Color color); // Draws a color-filled arc or pie-slice shape.
SITAPI void RGL_DrawPolygon(vec2* points, int point_count, float z_depth, Color color); // Draws a filled, convex polygon on a specific Z-plane in world space.
SITAPI void RGL_DrawPolygonScreen(vec2* points, int point_count, Color color); // Draws a filled, convex polygon in screen space for UI.
SITAPI void RGL_DrawCircleYPQ(vec2 center, float radius, ColorYPQA color);  // CRT-specific color
SITAPI void RGL_DrawRectangleYPQ(Rectangle rect, ColorYPQA color);          // CRT-specific color
// 3D Primitive & Sprite Drawing
SITAPI void RGL_DrawTriangle3D(vec3 p1, vec3 p2, vec3 p3, vec3 normal, vec2 uv1, vec2 uv2, vec2 uv3, RGLSprite sprite, Color tint, float base_light); // Draws a single lit, textured 3D triangle with explicit UVs.
SITAPI void RGL_DrawQuad3D(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 normal, RGLSprite sprite, Color tint, float base_light); // Draws a single lit, textured 3D quad defined by four corner points.
SITAPI void RGL_DrawCube(vec3 position, float size, RGLMaterial material);  // Draws a solid-colored, dynamically lit 3D cube.
SITAPI void RGL_DrawLine3D(vec3 start, vec3 end, float thickness, Color color); // Draws a 3D line with specified thickness.
SITAPI void RGL_DrawSprite(RGLSprite sprite, vec2 position, float roll_degrees, float scale, Color tint); // Draws a simple 2D sprite with rotation and scaling.
SITAPI void RGL_DrawTexturePro(RGLSprite sprite, Rectangle dest_rect, vec2 origin, float rotation_degrees, Color tint); // Draws a textured quad with transformation options.
SITAPI void RGL_DrawSpritePro(RGLSprite sprite, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]); // The ultimate low-level sprite/quad drawing function with full options.
SITAPI void RGL_DrawBillboard(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint); // Draws a sprite in 3D that always faces the camera (spherical).
SITAPI void RGL_DrawBillboardCylindricalY(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint); // Draws a sprite in 3D that only pivots on the Y-axis to face the camera.
SITAPI void RGL_DrawPanoramaBackground(RGLTexture texture, float scroll_offset_x, float y_offset_pct, float height_scale, Color tint); // Draws a horizontally-scrolling panoramic background.
SITAPI void RGL_DrawQuadPro(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]); // DEPRECATED - Use DrawSpritePro.
SITAPI void RGL_DrawQuad(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color tint); // DEPRECATED - Use DrawTexturePro or DrawSpritePro.
//==================================================================================
// Particle System
//==================================================================================
SITAPI void RGL_InitParticles(size_t max_particles);                        // Initializes the particle system with a maximum capacity.
SITAPI void RGL_EmitParticles(RGLParticleEmitter emitter);                  // Emits a burst of particles from a defined emitter.
SITAPI void RGL_UpdateParticles(float delta_time);                          // Updates the physics and lifetime of all active particles.
SITAPI void RGL_DrawParticles(void);                                        // Draws all active particles as billboards.
//==================================================================================
// Font & Text Module
//==================================================================================
// --- Font Creation ---
SITAPI RGLBitmapFont RGL_LoadBitmapFont(const char* texture_filepath, int char_width, int char_height, int first_char); // Loads a font from a pre-made grid texture atlas.
SITAPI RGLTrueTypeFont RGL_LoadTrueTypeFont(const char* font_path, float font_size); // Loads a .ttf font and bakes it into a texture atlas for high-speed rendering.
SITAPI RGLBitmapFont RGL_CreateCP437Font(const unsigned char* font_data_8x16); // Creates a classic 8x16 CP437 terminal font from memory.
SITAPI RGLBitmapFont RGL_CreatePackedBitmapFont(const void* packed_data, const RGLPackedFontConfig* config); // Creates a bitmap font from custom-packed bit data.
SITAPI RGLBitmapFont RGL_CreateVCRFont(const uint16_t* font_data);          // Creates a 12x14 VCR-style font from custom packed data.
SITAPI RGLBitmapFont RGL_CreateVCRFontWithOutline(const uint16_t* data, int outline_thickness); // Creates a VCR-style font with an outline.
SITAPI RGLBitmapFont RGL_CreateVGA8x8Font(const unsigned char* font_data);  // Creates an 8x8 VGA-style font from custom packed data.
SITAPI RGLBitmapFont RGL_CreateVGA8x8FontWithOutline(const unsigned char* data, int outline_thickness); // Creates a VGA-style font with an outline.
SITAPI RGLBitmapFont RGL_CreateBitmapFontFromSystemFont(const char* font_name, int font_size, int char_width, int char_height);
SITAPI RGLBitmapFont RGL_CreateTerminalFont(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char);
SITAPI RGLBitmapFont RGL_CreateTerminalFontEx(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char, float char_spacing, float line_spacing);
SITAPI RGLBitmapFont RGL_CreateASCIIFont(const unsigned char* font_data, int char_width, int char_height);
// --- Font Rendering ---
SITAPI RGLTexture RGL_StampTextToTextureAdvanced(const char* text, RGLTrueTypeFont font, Color text_color, Color bg_color, float wrap_width, int* out_width, int* out_height); // Renders TTF text to a texture with word wrapping.
SITAPI RGLTexture RGL_StampTextToTexture(const char* text, RGLBitmapFont font, Color text_color, Color bg_color, int* out_width, int* out_height); // Renders text to a new texture.
SITAPI void RGL_DrawTextEx(const char* text, vec2 position, RGLBitmapFont font, float spacing, Color color); // Draws bitmap text with custom character spacing.
SITAPI void RGL_DrawText(const char* text, vec2 position, RGLBitmapFont font, Color color); // Draws text using a bitmap font.
SITAPI void RGL_DrawTextTTF(const char* text, vec2 position, RGLTrueTypeFont font, Color color); // Draws text using a high-quality baked TrueType font.
SITAPI void RGL_DrawTextBoxed(const char* text, Rectangle bounds, RGLBitmapFont font, Color color, bool word_wrap); // Draws text within a rectangle, with optional word wrapping.
SITAPI void RGL_UnloadBitmapFont(RGLBitmapFont font);                       // Unloads a bitmap font's texture atlas.
SITAPI void RGL_UnloadTrueTypeFont(RGLTrueTypeFont font);                   // Unloads a TrueType font's texture atlas.
// --- Font Effects ---
SITAPI void RGL_DrawTextWithShadow(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color shadow_color, vec2 shadow_offset); // Draws text with a drop shadow.
SITAPI void RGL_DrawTextWithOutline(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color outline_color, float outline_thickness); // Draws text with an outline.
SITAPI void RGL_DrawTextGradient(const char* text, vec2 position, RGLBitmapFont font, Color top_color, Color bottom_color); // Draws text with a vertical color gradient.
SITAPI void RGL_DrawTextWave(const char* text, vec2 position, RGLBitmapFont font, Color color, float wave_amplitude, float wave_frequency, float time); // Draws text with a sinusoidal wave effect.
// --- Font Utilities ---
SITAPI vec2 RGL_MeasureText(const char* text, RGLBitmapFont font);          // Measures the pixel dimensions of a string for a bitmap font.
SITAPI vec2 RGL_MeasureTextTTF(const char* text, RGLTrueTypeFont font);     // Measures the pixel dimensions of a string for a TrueType font.
SITAPI int RGL_GetTextLineCount(const char* text, RGLBitmapFont font, float max_width);
//==================================================================================
// Math & Color Utilities
//==================================================================================
// --- General Math ---
SITAPI float RGL_Lerp(float a, float b, float t);                           // Linearly interpolates between two float values.
SITAPI float RGL_Clamp(float value, float min, float max);                  // Clamps a float value between a minimum and a maximum.
SITAPI float RGL_Normalize(float value, float start, float end);            // Normalizes a value from a given range to the [0, 1] range.
SITAPI float RGL_Remap(float value, float input_start, float input_end, float output_start, float output_end); // Remaps a value from one range to another.
SITAPI vec2 RGL_Vector2Lerp(vec2 a, vec2 b, float t);                       // Linearly interpolates between two 2D vectors.
SITAPI vec2 RGL_Vector2Rotate(vec2 v, float angle_degrees);                 // Rotates a 2D vector by a given angle in degrees.
SITAPI float RGL_Vector2Angle(vec2 v);                                      // Calculates the angle of a 2D vector in degrees.
SITAPI bool RGL_IsPointInRectangle(vec2 point, Rectangle rect);             // Checks if a 2D point is inside a rectangle.
SITAPI bool RGL_IsPointInCircle(vec2 point, vec2 center, float radius);     // Checks if a 2D point is inside a circle.
// --- RGB Color ---
SITAPI Color RGL_ColorFromHSV(float hue, float saturation, float value);    // Creates a Color from Hue, Saturation, and Value components.
SITAPI Color RGL_ColorFromHex(unsigned int hex_value);                      // Creates a Color from a 24-bit or 32-bit hexadecimal value.
SITAPI vec3 RGL_ColorToHSV(Color color);                                    // Converts an RGBA Color to a vec3 of Hue, Saturation, and Value.
SITAPI unsigned int RGL_ColorToHex(Color color);                            // Converts a Color to its 32-bit hexadecimal value (AARRGGBB).
SITAPI Color RGL_ColorLerp(Color c1, Color c2, float t);                    // Linearly interpolates between two colors.
SITAPI Color RGL_FadeColor(Color color, float alpha);                       // Creates a faded version of a color by modifying its alpha.
SITAPI Color RGL_ColorMultiply(Color c1, Color c2);                         // Multiplies two colors component-wise.
SITAPI Color RGL_ColorAdd(Color c1, Color c2);                              // Adds two colors component-wise, clamping at 255.
SITAPI Color RGL_ColorSubtract(Color c1, Color c2);                         // Subtracts one color from another, clamping at 0.
SITAPI Color RGL_ColorBrightness(Color color, float factor);                // Adjusts the brightness of a color by a multiplicative factor.
SITAPI Color RGL_ColorContrast(Color color, float contrast);                // Adjusts the contrast of a color.
SITAPI Color RGL_ColorSaturate(Color color, float saturation);              // Adjusts the saturation of a color.
SITAPI Color RGL_ColorDesaturate(Color color);                              // Converts a color to its perceptually-weighted grayscale equivalent.
SITAPI Color RGL_ColorInvert(Color color);                                  // Inverts the RGB channels of a color.
SITAPI Color RGL_ColorGamma(Color color, float gamma);                      // Applies gamma correction to a color.
// --- Color Palettes & Schemes ---
SITAPI Color RGL_ColorFromPalette(const Color* palette, int palette_size, float t); // Samples a color from a palette using linear interpolation.
SITAPI void RGL_GenerateGradientPalette(Color start, Color end, Color* out_palette, int steps); // Generates a palette by creating a linear gradient between two colors.
SITAPI void RGL_GenerateRainbowPalette(Color* out_palette, int steps);      // Generates a vibrant rainbow palette by cycling through hue.
// --- Color Analysis ---
SITAPI float RGL_ColorLuminance(Color color);                               // Calculates the perceptual luminance (brightness) of a color.
SITAPI float RGL_ColorDistance(Color color1, Color color2);                 // Calculates the Euclidean distance between two colors in RGB space.
SITAPI bool RGL_ColorEquals(Color color1, Color color2, float tolerance);   // Checks if two colors are similar within a given distance tolerance.
SITAPI Color RGL_ColorClosest(Color target, const Color* palette, int palette_size); // Finds the color in a palette that is closest to a target color.
// --- YPQ Color Space Utilities (For CRT/NTSC Emulation) ---
SITAPI ColorYPQA RGL_YPQLerp(ColorYPQA color1, ColorYPQA color2, float t);  // Interpolates between two YPQ colors, preserving hue correctly.
SITAPI ColorYPQA RGL_YPQAdjustLuminance(ColorYPQA color, float luminance_factor); // Adjusts the brightness (Y channel) of a YPQ color.
SITAPI ColorYPQA RGL_YPQAdjustPhase(ColorYPQA color, int phase_shift);      // Rotates the hue (P channel) of a YPQ color.
SITAPI ColorYPQA RGL_YPQAdjustQuadrature(ColorYPQA color, float quad_factor); // Adjusts the saturation (Q channel) of a YPQ color.
SITAPI ColorYPQA RGL_YPQMultiply(ColorYPQA color1, ColorYPQA color2);       // Multiplies two YPQ colors component-wise.
SITAPI Color RGL_ColorScanline(ColorYPQA color, float scanline_y, float intensity); // Applies a scanline effect by dimming a YPQ color on alternating lines.
SITAPI Color RGL_ColorTVNoise(ColorYPQA base_color, float noise_strength, vec2 screen_pos); // Applies a procedural TV noise effect to a YPQ color.
SITAPI Color RGL_ColorCRTBloom(ColorYPQA color, float bloom_strength);      // Applies a CRT phosphor bloom effect to a YPQ color.
SITAPI Color RGL_ColorTVGhost(ColorYPQA color, float ghost_offset, float ghost_strength); // Applies a TV signal ghosting effect to a YPQ color.
// --- YPQ Palettes & Gradients ---
SITAPI void RGL_GenerateYPQGradient(ColorYPQA start, ColorYPQA end, Color* out_palette, int steps); // Generates a palette by interpolating in YPQ space for more natural results.
SITAPI ColorYPQA RGL_YPQFromTVChannel(int channel, float signal_strength);  // Generates a pseudo-random TV-like color based on channel and signal strength.
SITAPI Color SituationColorFromYPQPalette(const ColorYPQA* ypq_palette, int palette_size, float t); // Samples a color from a YPQ palette.
// --- YPQ Analysis & Utilities ---
SITAPI float RGL_YPQGetLuminance(ColorYPQA color) { return (float)color.y / 255.0f;} // Gets the luminance (Y channel) of a YPQ color as a normalized float [0-1].
SITAPI float RGL_YPQGetChroma(ColorYPQA color) { return (float)color.q / 255.0f;}  // Gets the chroma/saturation (Q channel) of a YPQ color as a normalized float [0-1].
SITAPI float RGL_YPQGetHue(ColorYPQA color) { return ((float)color.p / 255.0f) * 360.0f;}  // Gets the hue/phase (P channel) of a YPQ color as degrees [0-360].
SITAPI bool RGL_YPQEquals(ColorYPQA color1, ColorYPQA color2, unsigned char tolerance); // Checks if two YPQ colors are similar within a tolerance.
SITAPI ColorYPQA RGL_YPQClosest(ColorYPQA target, const ColorYPQA* palette, int palette_size); // Finds the closest color in a YPQ palette to a target YPQ color.
// --- YPQ Constants for Classic ANSi Colors ---
SITAPI ColorYPQA RGL_GetYPQBlack(void)   { return SituationColorToYPQ((Color){  0,   0,   0, 255}; } // Gets the YPQ representation of ANSI Black
SITAPI ColorYPQA RGL_GetYPQRed(void)     { return SituationColorToYPQ((Color){170,   0,   0, 255}; } // Gets the YPQ representation of ANSI Red
SITAPI ColorYPQA RGL_GetYPQGreen(void)   { return SituationColorToYPQ((Color){  0, 170,   0, 255}; } // Gets the YPQ representation of ANSI Green
SITAPI ColorYPQA RGL_GetYPQYellow(void)  { return SituationColorToYPQ((Color){170,  85,   0, 255}; } // Gets the YPQ representation of ANSI Yellow (often brown)
SITAPI ColorYPQA RGL_GetYPQBlue(void)    { return SituationColorToYPQ((Color){  0,   0, 170, 255}; } // Gets the YPQ representation of ANSI Blue
SITAPI ColorYPQA RGL_GetYPQMagenta(void) { return SituationColorToYPQ((Color){170,   0, 170, 255}; } // Gets the YPQ representation of ANSI Magenta
SITAPI ColorYPQA RGL_GetYPQCyan(void)    { return SituationColorToYPQ((Color){  0, 170, 170, 255}; } // Gets the YPQ representation of ANSI Cyan
SITAPI ColorYPQA RGL_GetYPQWhite(void)   { return SituationColorToYPQ((Color){170, 170, 170, 255}; } // Gets the YPQ representation of ANSI White (light gray)
SITAPI ColorYPQA RGL_GetYPQBBlack(void)  { return SituationColorToYPQ((Color){ 85,  85,  85, 255}; } // Gets the YPQ representation of ANSI Bright Black (dark gray)
SITAPI ColorYPQA RGL_GetYPQBRed(void)    { return SituationColorToYPQ((Color){255,  85,  85, 255}; } // Gets the YPQ representation of ANSI Bright Red
SITAPI ColorYPQA RGL_GetYPQBGreen(void)  { return SituationColorToYPQ((Color){ 85, 255,  85, 255}; } // Gets the YPQ representation of ANSI Bright Green
SITAPI ColorYPQA RGL_GetYPQBYellow(void) { return SituationColorToYPQ((Color){255, 255,  85, 255}; } // Gets the YPQ representation of ANSI Bright Yellow
SITAPI ColorYPQA RGL_GetYPQBBlue(void)   { return SituationColorToYPQ((Color){ 85,  85, 255, 255}; } // Gets the YPQ representation of ANSI Bright Blue
SITAPI ColorYPQA RGL_GetYPQBMagenta(void){ return SituationColorToYPQ((Color){255,  85, 255, 255}; } // Gets the YPQ representation of ANSI Bright Magenta
SITAPI ColorYPQA RGL_GetYPQBCyan(void)   { return SituationColorToYPQ((Color){ 85, 255, 255, 255}; } // Gets the YPQ representation of ANSI Bright Cyan
SITAPI ColorYPQA RGL_GetYPQBWhite(void)  { return SituationColorToYPQ((Color){255, 255, 255, 255}; } // Gets the YPQ representation of ANSI Bright White
//==================================================================================
// Debug & Calibration Module
//==================================================================================
SITAPI void RGL_SetDebugDrawTriggers(bool enabled);                         // Sets whether invisible triggers (junctions, events) are visualized.
SITAPI bool RGL_GetDebugDrawTriggers(void);                                 // Gets the current state of trigger visualization.
SITAPI void RGL_ToggleDebugDrawTriggers(void);                              // Toggles the visualization of invisible triggers.
SITAPI void RGL_DrawPerformanceOverlay(void);                               // Draws an overlay with real-time rendering statistics (FPS, draw calls, batch stats, etc.).
SITAPI void RGL_DrawWireframeBounds(vec3 min_bounds, vec3 max_bounds, Color color); // Draws a 3D wireframe bounding box for debugging.
SITAPI void RGL_DrawPathDebugInfo(float player_z, bool show_control_points, bool show_splines); // Renders an in-world debug view of the active path's structure.
SITAPI void RGL_DrawShadowVolumeDebug(vec3 world_pos, vec2 size, const RGLShadowConfig* config); // Renders a visualization of a stencil shadow volume for debugging.
SITAPI void RGL_DrawLevelDebug(void); // Renders a wireframe debug view of the active level's geometry.
SITAPI void RGL_DrawTestPattern(const RGLTestPatternConfig* config);        // Draws a standard video test pattern for calibration and testing.
SITAPI RGLTestPatternConfig RGL_GetDefaultTestPatternConfig(RGLTestPatternType type); // Gets a default configuration for a specific test pattern type.
SITAPI void RGL_DrawGrid(vec2 spacing, vec2 offset, Color color);           // Draws a 2D grid of lines for calibration
SITAPI void RGL_DrawCheckerboard(Rectangle rect, vec2 tile_size, Color color1, Color color2); // Fills a rectangle with a checkerboard pattern.
SITAPI void RGL_DrawStripes(Rectangle rect, float stripe_width, bool vertical, Color color1, Color color2); // Fills a rectangle with a stripe pattern.
SITAPI void RGL_DrawSafeArea(Rectangle screen, float overscan_pct, Color color); // Draws an outline representing the TV-safe area.
SITAPI void RGL_DrawCrosshair(vec2 center, float size, float thickness, Color color); // Draws a crosshair marker.
SITAPI void RGL_DrawArrow(vec2 start, vec2 end, float head_size, float thickness, Color color); // Draws a line with an arrowhead at the end.
SITAPI void RGL_DrawRuler(vec2 start, vec2 end, float tick_spacing, float tick_length, Color color); // Draws a ruler with tick marks for calibration.
SITAPI void RGL_DrawLabeledRectangle(Rectangle rect, const char* label, RGLBitmapFont font, Color rect_color, Color text_color); // Draws a rectangle with a centered text label.

// ===================================================================================
// --- IMPLEMENTATION ---
// ===================================================================================
#ifdef RGL_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h> // For FLT_EPSILON

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif
#include <ext/stb_image.h>
#include <ext/tiny_obj_loader_c.h>

#define PAR_SHAPES_IMPLEMENTATION
#include "ext/par_shapes.h"

#include "font_data.h"
// Create an alias. The internal debug system will use FONT_DATA_8X8,
// which the preprocessor will replace with the specific font from your header.
#define FONT_DATA_8X8 ibm_font_8x8

// Helper for linear interpolation (lerp)
static float _lerp(float a, float b, float t) { return a + (b - a) * t; }

// Helper for Catmull-Rom spline interpolation
static float _catmull_rom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

// Internal helper for clamping an integer to the range of an unsigned char.
static inline unsigned char _RGL_ClampToU8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

// Internal helper to clamp a float to the [0, 1] range.
static inline float _RGL_Clamp01(float value) {
    return RGL_Clamp(value, 0.0f, 1.0f);
}

// --- Internal State and Structs ---

typedef struct {
    vec3 pos;
    vec2 tex;
    vec3 norm;
} RGLVertex3D;

static const RGLTextureParams RGL_DEFAULT_TEXTURE_PARAMS = {
    .wrap_s = GL_REPEAT,
    .wrap_t = GL_REPEAT,
    .filter_min = GL_LINEAR_MIPMAP_LINEAR,
    .filter_mag = GL_LINEAR,
    .generate_mipmaps = true
};

// Internal representation of a draw command, derived from RGLDrawCall
typedef struct {
    RGLTexture texture;
    float z_depth;           // For Z-sorting
    bool is_triangle;        // True if this command represents a single triangle

    vec3 world_positions[4]; // Max 4 vertices for quads
    vec3 normals[4];         // Vertex normals
    vec2 tex_coords[4];      // Texture coordinates
    vec4 colors[4];          // Vertex colors
    float light_levels[4]; // Base light contribution (0.0 to 1.0)
} RGLInternalDraw;

#define RGL_MATRIX_STACK_DEPTH 10

// This holds a snapshot of the camera state
typedef struct {
    mat4 view;
    mat4 projection;
} RGLMatrixState;

// --- Internal State ---

/**
 * @brief Defines a visual style for rendering a path.
 */
typedef struct {
    RGLPathSegmentDrawCallback draw_segment;
    void* user_data;
} RGLPathStyle;

// --- Define the static, default style instance ---
// This object acts as a "preset". It bundles the road-drawing function
// with its name so it can be assigned to a path.
static const RGLPathStyle RGL_DEFAULT_ROAD_STYLE = {
    .draw_path_func = _RGL_DrawPathScene_Road, // The function pointer is assigned the address of our road-drawing function.
    .user_data = NULL
};

// Holds all data related to the procedural Path system.
typedef struct {
    RGLPathPoint* points;
    size_t num_points;
    size_t capacity;

    int last_segment_index_cache;
    float loop_to_z;

    // --- THE MISSING LINK ---
    /** @brief A pointer to the style definition used to render this path. Defaults to the road style. */
    const RGLPathStyle* style;

} RGLPathData;

typedef struct {
    char name[32];
    RGLPathData data;
} RGLNamedPath;

typedef struct {
    SituationShader main_shader;
    GLint loc_view;
    GLint loc_projection;
    GLint loc_texture_sampler;
    GLint loc_use_texture;

    // --- State for the shadow shader ---
    SituationShader shadow_shader;
    GLint loc_shadow_view;
    GLint loc_shadow_projection;
    GLint loc_shadow_texture;
    GLint loc_shadow_tint;

    // --- Stencil Shadow State ---
    SituationShader shadow_volume_shader;
    GLint loc_sv_view;
    GLint loc_sv_projection;

    SituationShader shadow_darken_shader;
    GLint loc_sd_shadow_color;
    GLuint fullscreen_quad_vao;
    
    // --- Shader locations for lighting ---
    GLint loc_camera_pos;
    GLint loc_ambient_light_color;
    GLint loc_active_lights;
    GLint loc_light_pos;
    GLint loc_light_color;
    GLint loc_light_radius;
    GLint loc_light_intensity;
    GLuint light_ubo;

    // --- Light management system ---
    RGLLight lights[RGL_MAX_LIGHTS];
    vec3 ambient_light_color;
    ma_mutex light_mutex;

    GLuint batch_vao;
    GLuint batch_vbo;
    GLint default_fbo;
    
    RGLInternalDraw* commands;
    size_t command_count;
    size_t command_capacity;

    float* cpu_vertex_buffer;
    size_t cpu_vertex_buffer_floats_capacity;

    mat4 current_projection_matrix;
    mat4 current_view_matrix;
    vec3 camera_position;

    // --- Camera and Matrix Stack ---
    RGLMatrixState matrix_stack[RGL_MATRIX_STACK_DEPTH];
    int matrix_stack_ptr;
    
    mat4 transform; // Current 3D transform matrix
    bool use_transform; // Flag to apply transform
    
    bool is_initialized;
    bool is_batching;
    int active_virtual_display_id;
    
        // --- Camera and Viewport State ---
    mat4 view;
    mat4 projection;
    Rectangle viewport; // Stores {x, y, width, height} of the current render area
    
    RGLNamedPath* Paths;
    size_t Path_count;
    size_t Path_capacity;
    int active_Path_index; // The index of the Path we are currently on. -1 if none.

    RGLLevel* levels; // Array of levels
    size_t level_count;
    size_t level_capacity;
    int active_level_index; // Current level for drawing

    RGLMesh* meshes;
    size_t mesh_count;
    size_t mesh_capacity;
    
        // --- Performance pathing and debug state ---
    struct {
        uint64_t frames_rendered;
        uint64_t total_draw_calls;
        uint64_t total_vertices_drawn;
        uint64_t batch_flushes;
        uint64_t memory_reallocations;
        float last_frame_time_ms;
        float avg_draw_calls_per_frame;
        float avg_vertices_per_frame;
        float avg_batch_efficiency;
    } stats;
    
    struct {
        // Wireframe rendering state
        GLuint wireframe_shader;
        GLuint wireframe_vao;
        GLuint wireframe_vbo;
        GLint wireframe_mvp_loc;
        GLint wireframe_color_loc;
        bool debug_initialized;

        // Debug font state
        RGLBitmapFont font;
        bool font_initialized;
    } debug;
    
} RGLState;

static RGLState RGL;

// --- Shader Source ---
static const char* RGL_VERTEX_SHADER =
    "#version 330 core\n"
    // -- Vertex Attributes --
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"
    "layout (location = 3) in vec4 aColor;\n"
    "layout (location = 4) in float aBaseLightLevel;\n"

    // -- Outputs to Fragment Shader --
    "out vec2 vTexCoord;\n"
    "out vec4 vColor;\n"
    "out vec3 vLightColor;\n"

    // -- Standard Uniforms --
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 u_camera_pos;\n"

    // -- Global Lighting Uniforms --
    "uniform vec3 u_ambient_light_color;\n"
    "uniform int u_active_lights;\n"

    // -- UBO for Unified Light Data --
    "#define MAX_LIGHTS 16\n"
    "#define LIGHT_TYPE_POINT 1\n"
    "#define LIGHT_TYPE_DIRECTIONAL 2\n"
    "#define LIGHT_TYPE_SPOT 3\n"
    "layout(std140, binding = 0) uniform LightBlock {\n"
    "    // .xyz = position (point/spot), .w = type ID\n"
    "    vec4 u_light_pos_type[MAX_LIGHTS];\n"
    "    // .rgb = color, .a = intensity\n"
    "    vec4 u_light_color_intensity[MAX_LIGHTS];\n"
    "    // .xyz = direction (directional/spot)\n"
    "    vec4 u_light_direction[MAX_LIGHTS];\n"
    "    // .x=radius, .y=cos(outer_angle), .z=cos(inner_angle)\n"
    "    vec4 u_light_params[MAX_LIGHTS];\n"
    "};\n"

    "void main() {\n"
    "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "    vTexCoord = aTexCoord;\n"
    "    vColor = aColor;\n"

    "    // --- Lighting Calculation --- \n"
    "    vec3 world_pos = aPos;\n"
    "    vec3 normal = normalize(aNormal);\n"
    "    vec3 total_light_contrib = u_ambient_light_color * aBaseLightLevel;\n"

    "    for (int i = 0; i < u_active_lights; i++) {\n"
    "        int light_type = int(u_light_pos_type[i].w);\n"
    "        vec3 light_color = u_light_color_intensity[i].rgb;\n"
    "        float intensity = u_light_color_intensity[i].a;\n"
    "        vec3 diffuse_color = vec3(0.0);\n"

    "        if (light_type == LIGHT_TYPE_POINT) {\n"
    "            vec3 light_pos = u_light_pos_type[i].xyz;\n"
    "            float radius = u_light_params[i].x;\n"
    "            vec3 light_dir = light_pos - world_pos;\n"
    "            float dist = length(light_dir);\n"
    "            if (dist < radius) {\n"
    "                light_dir = normalize(light_dir);\n"
    "                float attenuation = 1.0 - smoothstep(0.8, 1.0, dist / radius);\n"
    "                attenuation /= (1.0 + 0.1*dist + 0.05*dist*dist);\n"
    "                float diff = max(dot(normal, light_dir), 0.0);\n"
    "                diffuse_color = diff * light_color * intensity * attenuation;\n"
    "            }\n"
    "        } else if (light_type == LIGHT_TYPE_DIRECTIONAL) {\n"
    "            vec3 light_dir = normalize(u_light_direction[i].xyz);\n"
    "            float diff = max(dot(normal, -light_dir), 0.0);\n"
    "            diffuse_color = diff * light_color * intensity;\n"
    "        } else if (light_type == LIGHT_TYPE_SPOT) {\n"
    "            vec3 light_pos = u_light_pos_type[i].xyz;\n"
    "            float radius = u_light_params[i].x;\n"
    "            vec3 light_to_frag = light_pos - world_pos;\n"
    "            float dist = length(light_to_frag);\n"
    "            if (dist < radius) {\n"
    "                vec3 light_to_frag_norm = normalize(light_to_frag);\n"
    "                vec3 spot_dir = normalize(u_light_direction[i].xyz);\n"
    "                float theta = dot(light_to_frag_norm, -spot_dir);\n"
    "                float cos_outer = u_light_params[i].y;\n"
    "                float cos_inner = u_light_params[i].z;\n"
    "                if (theta > cos_outer) {\n"
    "                    float spot_effect = smoothstep(cos_outer, cos_inner, theta);\n"
    "                    float attenuation = 1.0 - smoothstep(0.8, 1.0, dist / radius);\n"
    "                    attenuation /= (1.0 + 0.1*dist + 0.05*dist*dist);\n"
    "                    float diff = max(dot(normal, light_to_frag_norm), 0.0);\n"
    "                    diffuse_color = diff * light_color * intensity * attenuation * spot_effect;\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "        total_light_contrib += diffuse_color;\n"
    "    }\n"
    "    vLightColor = total_light_contrib;\n"
    "}\n";

// --- Shader Source ---
static const char* RGL_FRAGMENT_SHADER =
    "#version 330 core\n"
    "out vec4 FragColor;\n"

    // -- Inputs from Vertex Shader --
    "in vec2 vTexCoord;\n"
    "in vec4 vColor;\n"
    "in vec3 vLightColor; // <-- MODIFIED: Receive full light color\n"

    // -- Uniforms --
    "uniform sampler2D textureSampler;\n"
    "uniform bool useTexture;\n"

    "void main() {\n"
    "    // Get base color from texture or vertex color\n"
    "    vec4 base_color = useTexture ? texture(textureSampler, vTexCoord) : vec4(1.0);\n"
    "    vec4 final_color = base_color * vColor;\n"

    "    // --- MODIFIED: Apply the lighting --- \n"
    "    // Modulate the fragment's RGB by the calculated light color.\n"
    "    // The alpha component is preserved from the original color.\n"
    "    final_color.rgb *= vLightColor;\n"

    "    FragColor = final_color;\n"
    
    "    // Discard transparent pixels\n"
    "    if (FragColor.a < 0.01) discard;\n"
    "}\n";

// Shaders for the projected shadow effect
static const char* RGL_SHADOW_VERTEX_SHADER =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec2 vTexCoord;\n"
    "void main() {\n"
    "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "    vTexCoord = aTexCoord;\n"
    "}\n";

static const char* RGL_SHADOW_FRAGMENT_SHADER =
    "#version 330 core\n"
    "in vec2 vTexCoord;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 shadowTint;\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture0, vTexCoord);\n"
    "    // Only proceed if the source texture pixel is opaque enough\n"
    "    if (texColor.a < 0.5) discard;\n"
    "    // Use the tint's RGB, but modulate its alpha by the texture's alpha\n"
    "    // This allows for semi-transparent shadows based on the sprite's shape.\n"
    "    finalColor = vec4(shadowTint.rgb, shadowTint.a * texColor.a);\n"
    "}\n";

static const char* RGL_SHADOW_VOLUME_VERTEX_SHADER_ROBUST =
    "#version 330 core\n"
    "layout (location = 0) in vec4 aPos; // Use vec4: .xyz = position, .w = 0 for normal, 1 for extruded\n"
    
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 u_light_pos;\n"

    "void main()\n"
    "{\n"
    "    vec3 position = aPos.xyz;\n"
    "    if (aPos.w > 0.5) { // If this is an extruded vertex\n"
    "        // Extrude it from the light source out to infinity.\n"
    "        // A directional vector from the light through the vertex position.\n"
    "        vec3 light_dir = position - u_light_pos;\n"
    "        // Projecting to infinity is done by setting W to 0 in the final clip space position.\n"
    "        position = light_dir; // Treat as a direction, not a position\n"
    "    }\n"
    "    \n"
    "    vec4 clip_pos = projection * view * vec4(position, aPos.w > 0.5 ? 0.0 : 1.0);\n"
    "    gl_Position = clip_pos;\n"
    "}\n";

// We will also need a shader to darken the scene.
static const char* RGL_SHADOW_PASS_FRAGMENT_SHADER =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 u_shadow_color;\n"
    "void main()\n"
    "{\n"
    "    FragColor = u_shadow_color;\n"
    "}\n";

static const char* RGL_WIREFRAME_VERTEX_SHADER = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos; uniform mat4 mvp;"
    "void main() { gl_Position = mvp * vec4(aPos, 1.0); }";

static const char* RGL_WIREFRAME_FRAGMENT_SHADER = "#version 330 core\n"
    "out vec4 FragColor; uniform vec4 color;"
    "void main() { FragColor = color; }";


//RGL Internal Static Helper Functions
//==================================================================================
// Core Batching & Rendering Helpers
//==================================================================================
static void _RGL_FlushBatch(void); // Processes all queued commands, sorts them, and issues batched draw calls to the GPU.
static bool _RGL_EnsureCommandCapacity(size_t required_command_count); // Checks if the command buffer has space; if not, flushes the batch and/or grows the buffer.
static int _RGL_CompareDrawCommands(const void* a, const void* b); // The qsort comparison function for sorting draw commands by Z-depth and texture ID.
//==================================================================================
// Dynamic Lighting Helpers
//==================================================================================
static int _RGL_FindFreeLightSlot(void); // Scans the internal lights array to find the first available empty slot.
static int _RGL_CompareRankedLights(const void* a, const void* b); // The qsort comparison function for sorting visible lights by priority/distance.
static void _RGL_ExtractFrustumPlanes(const mat4 vp_matrix, vec4 out_frustum_planes[6]); // Calculates the six planes of the view frustum from a combined view-projection matrix.
static bool _RGL_FrustumIntersectsSphere(const vec4 p[6], vec3 center, float radius, float bias); // Checks if a sphere is visible within the view frustum, with an optional near-plane bias.
//==================================================================================
// World System: Path Helpers
//==================================================================================
static int _RGL_FindPathIndex(const char* path_name); // Finds the internal array index for a path given its unique string name.
static RGLPathData* _RGL_GetActivePathData(void); // Gets a direct pointer to the data of the currently active path.
static int _RGL_FindPathPointIndexAt(RGLPathPoint* points, size_t num_points, float z_pos); // Performs a fast, O(log n) search to find the path segment index for a given Z-position.
static void _RGL_CalculateBankedSurface(RGLPathPoint* point, float lateral_offset, vec3 out_normal); // Calculates the 3D surface normal for a path, accounting for its bank angle.
static void _RGL_DrawSegment_Road(const RGLPathPoint* p_near, const RGLPathPoint* p_far, const vec3* normal, void* user_data); // The specific drawing logic for a single segment of a "road" style path.
static void _RGL_DrawPathScene_Road(float player_z, int draw_distance, void* user_data); // The master drawing function for the default "road" style, which loops and calls _RGL_DrawSegment_Road.
//==================================================================================
// World System: Level Helpers
//==================================================================================
static int _RGL_FindLevelIndex(const char* level_name); // Finds the internal array index for a level given its unique string name.
static bool _RGL_TriangulateFlat(const RGLFlat* flat, const RGLVertex3D_pos* vertices, int* triangle_indices, size_t* triangle_count); // Converts a potentially non-convex polygon (a "flat") into a list of drawable triangles.
static void _RGL_DrawLevelDebug(const RGLLevel* level); // The internal implementation for drawing a level's wireframe debug view.
//==================================================================================
// World System: Scenery Helpers
//==================================================================================
static void _RGL_DrawPathScenery(RGLPathPoint* path_point, RGLScenery* scenery); // The core scenery dispatcher; looks up the registered style for a scenery object and calls its drawing function.
//==================================================================================
// 3D Primitive & Mesh Helpers
//==================================================================================
static void _RGL_Draw3DQuad(vec3 center, vec3 right_vec, vec3 up_vec, float x_start, float x_end, float z_near, float z_far, RGLSprite sprite, Color color); // Queues a 3D quad by center, orientation vectors, and extents. Used by the Path system.
static void _RGL_DrawCubeFaces(vec3 position, float size, RGLMaterial material); // Queues the 6 individual, correctly-lit faces of a 3D cube for drawing.
static void _RGL_DrawLineQuad(vec3 start, vec3 end, float thickness, Color color); // Calculates the 4 vertices of a 3D quad that represents a thick line and queues it.
static RGLMesh _RGL_CreateMeshFromParShape(par_shapes_mesh* shape); // The core mesh integration function; converts a par_shapes mesh into a fully managed RGLMesh.
//==================================================================================
// Debug & Calibration Helpers
//==================================================================================
static bool _RGL_InitDebugRendering(void); // Initializes shaders and buffers for wireframe debug drawing. Called on first use.
static void _RGL_ShutdownDebugRendering(void); // Frees all resources used by the wireframe debug rendering system.
static bool _RGL_InitDebugTextSystem(void); // Initializes the built-in bitmap font for debug text. Called on first use.
static void _RGL_ShutdownDebugTextSystem(void); // Frees the texture used by the debug text system.
static void _RGL_DrawDebugText(const char* text, int x, int y, int size, Color color); // The internal implementation for drawing text with the debug font.
static void _RGL_DrawSmpteBars(const RGLTestPatternConfig* config); // The internal implementation for drawing the SMPTE color bars test pattern.
static void _RGL_DrawPluge(const RGLTestPatternConfig* config); // The internal implementation for drawing the PLUGE (black level) test pattern.
static void _RGL_DrawMultiburst(const RGLTestPatternConfig* config); // The internal implementation for drawing the multiburst (frequency response) test pattern.
static void _RGL_DrawCrosshatch(const RGLTestPatternConfig* config); // The internal implementation for drawing the crosshatch/grid test pattern.
static void _RGL_Draw3DGrid(const RGLTestPatternConfig* config); // The internal implementation for drawing the 3D grid test pattern.
//==================================================================================
// Miscellaneous Math & Internal Helpers
//==================================================================================
static float _lerp(float a, float b, float t); // Basic linear interpolation between two floats.
static float _catmull_rom(float p0, float p1, float p2, float p3, float t); // Calculates a point on a Catmull-Rom spline for smooth path curves.
static inline unsigned char _RGL_ClampToU8(int value); // Clamps an integer to the valid range for an unsigned char [0-255].
static inline float _RGL_Clamp01(float value); // Clamps a float to the normalized range [0.0-1.0].


// --- Static Helper Implementations ---

static RGLMesh _RGL_CreateMeshFromParShape(par_shapes_mesh* shape) {
    RGLMesh rgl_mesh = {0}; // Always zero-initialize
    if (!shape || shape->npoints == 0) {
        if (shape) par_shapes_free_mesh(shape);
        return rgl_mesh;
    }

    // --- 1. Prepare Interleaved Vertex Data on the CPU ---
    int vertex_count = shape->npoints;
    RGLVertex3D* vertex_data = malloc(sizeof(RGLVertex3D) * vertex_count);
    if (!vertex_data) {
        par_shapes_free_mesh(shape);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to alloc vertex data for par_shape mesh.");
        return rgl_mesh;
    }

    for (int i = 0; i < vertex_count; i++) {
        memcpy(vertex_data[i].pos, shape->points + (i * 3), sizeof(vec3));
        
        if (shape->normals) {
            memcpy(vertex_data[i].norm, shape->normals + (i * 3), sizeof(vec3));
        } else {
            glm_vec3_zero(vertex_data[i].norm);
        }

        if (shape->tcoords) {
            // Flip V coordinate for OpenGL convention
            vertex_data[i].tex_coord[0] = shape->tcoords[i * 2 + 0];
            vertex_data[i].tex_coord[1] = 1.0f - shape->tcoords[i * 2 + 1];
        } else {
            glm_vec2_zero(vertex_data[i].tex_coord);
        }
    }
    
    // --- 2. Upload Data to GPU using situation.h primitives ---
    // This is the clean, correct way to do it. RGL asks situation.h to make the GPU mesh.
    rgl_mesh.gpu_mesh = SituationCreateMesh(vertex_data, vertex_count, sizeof(RGLVertex3D),
                                            shape->triangles, shape->ntriangles * 3);

    // If GPU mesh creation failed, situation.h has already set the error.
    if (rgl_mesh.gpu_mesh.id == 0) {
        free(vertex_data);
        par_shapes_free_mesh(shape);
        return (RGLMesh){0};
    }

    // --- 3. Store CPU-side copies for shadows/physics ---
    // This is where RGL takes ownership of the data for its own CPU-side logic.
    rgl_mesh.vertex_count = vertex_count;
    rgl_mesh.index_count = shape->ntriangles * 3;

    // Allocate all CPU buffers
    rgl_mesh.cpu_vertices = malloc(sizeof(vec3) * vertex_count);
    rgl_mesh.cpu_texcoords = malloc(sizeof(vec2) * vertex_count);
    rgl_mesh.cpu_normals = malloc(sizeof(vec3) * vertex_count);
    rgl_mesh.cpu_indices = malloc(sizeof(unsigned int) * rgl_mesh.index_count);

    if (!rgl_mesh.cpu_vertices || !rgl_mesh.cpu_texcoords || !rgl_mesh.cpu_normals || !rgl_mesh.cpu_indices) {
        // Handle allocation failure: we must destroy the GPU mesh we just made.
        SituationDestroyMesh(&rgl_mesh.gpu_mesh);
        free(rgl_mesh.cpu_vertices); free(rgl_mesh.cpu_texcoords);
        free(rgl_mesh.cpu_normals); free(rgl_mesh.cpu_indices);
        free(vertex_data);
        par_shapes_free_mesh(shape);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to alloc CPU copies for par_shape mesh.");
        return (RGLMesh){0};
    }

    // Copy the data from the interleaved buffer into RGL's separate CPU buffers.
    for (int i = 0; i < vertex_count; i++) {
        glm_vec3_copy(vertex_data[i].pos, rgl_mesh.cpu_vertices[i]);
        glm_vec2_copy(vertex_data[i].tex_coord, rgl_mesh.cpu_texcoords[i]);
        glm_vec3_copy(vertex_data[i].norm, rgl_mesh.cpu_normals[i]);
    }
    memcpy(rgl_mesh.cpu_indices, shape->triangles, sizeof(unsigned int) * rgl_mesh.index_count);

    // --- 4. Cleanup ---
    free(vertex_data); // Free the temporary interleaved buffer
    par_shapes_free_mesh(shape);

    // --- 5. Finalize ---
    // TODO: Add the mesh to a managed list and assign a real ID.
    // For now, a non-zero ID indicates success.
    rgl_mesh.id = 1;

    return rgl_mesh;
}

// VISUAL TYPES (Always draw)
static void _RGL_DrawScenery_Sprite(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data) {
    (void)path_point; (void)user_data;
    RGL_DrawBillboardCylindricalY(scenery->data.visual.sprite, *world_pos, scenery->data.visual.size_in_world_units, WHITE);
}

static void _RGL_DrawScenery_Arch(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data) {
    (void)path_point; (void)user_data;
    RGL_DrawQuadPro(scenery->data.visual.sprite, *world_pos, scenery->data.visual.size_in_world_units, (vec2){0.5f, 0.5f}, (vec3){0,0,0}, (vec2){0,0}, (Color[4]){WHITE,WHITE,WHITE,WHITE}, NULL);
}

static bool g_debug_draw_triggers = false;

// DEBUG VISUALIZATION TYPES (Only draw if g_debug_draw_triggers is true)
static void _RGL_DrawScenery_EventMarker(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data) {
    if (!g_debug_draw_triggers) return;
    (void)scenery; (void)path_point; (void)user_data;
    vec3 min_b, max_b;
    glm_vec3_sub_s(*world_pos, 0.5f, min_b);
    glm_vec3_add_s(*world_pos, 0.5f, max_b);
    RGL_DrawWireframeBounds(min_b, max_b, (Color){0, 255, 0, 150}); // Green cube for events
}

static void _RGL_DrawScenery_JunctionTrigger(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data) {
    if (!g_debug_draw_triggers) return;
    (void)scenery; (void)path_point; (void)user_data;
    vec3 min_b, max_b;
    glm_vec3_sub_s(*world_pos, 0.8f, min_b);
    glm_vec3_add_s(*world_pos, 0.8f, max_b);
    RGL_DrawWireframeBounds(min_b, max_b, (Color){255, 255, 0, 150}); // Yellow cube for junctions
}

static void _RGL_DrawScenery_LevelEntrance(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data) {
    if (!g_debug_draw_triggers) return;
    (void)scenery; (void)path_point; (void)user_data;
    vec3 min_b, max_b;
    glm_vec3_sub_s(*world_pos, 1.0f, min_b);
    glm_vec3_add_s(*world_pos, 1.0f, max_b);
    RGL_DrawWireframeBounds(min_b, max_b, (Color){0, 150, 255, 150}); // Blue cube for level portals
}

// --- Default Style Objects (Private) ---
static RGLSceneryStyle g_default_sprite_style         = { .draw_func = _RGL_DrawScenery_Sprite, .user_data = NULL };
static RGLSceneryStyle g_default_arch_style           = { .draw_func = _RGL_DrawScenery_Arch, .user_data = NULL };
static RGLSceneryStyle g_default_event_marker_style   = { .draw_func = _RGL_DrawScenery_EventMarker, .user_data = NULL };
static RGLSceneryStyle g_default_junction_style       = { .draw_func = _RGL_DrawScenery_JunctionTrigger, .user_data = NULL };
static RGLSceneryStyle g_default_level_entrance_style = { .draw_func = _RGL_DrawScenery_LevelEntrance, .user_data = NULL };

// --- The Scenery Style Registry (Global, Private) ---
static RGLSceneryStyle* g_scenery_styles[RGL_MAX_SCENERY_TYPES];

SITAPI void RGL_SetDebugDrawTriggers(bool enabled) {
    g_debug_draw_triggers = enabled;
}

SITAPI bool RGL_GetDebugDrawTriggers(void) {
    return g_debug_draw_triggers;
}

SITAPI void RGL_ToggleDebugDrawTriggers(void) {
    g_debug_draw_triggers = !g_debug_draw_triggers;
}

static int _RGL_CompareDrawCommands(const void* a, const void* b) {
    const RGLInternalDraw* cmd_a = (const RGLInternalDraw*)a;
    const RGLInternalDraw* cmd_b = (const RGLInternalDraw*)b;

    // Primary sort: Z-depth, back-to-front for correct alpha blending.
    if (cmd_a->z_depth > cmd_b->z_depth) return -1;
    if (cmd_a->z_depth < cmd_b->z_depth) return 1;
    
    // Secondary sort: Group by texture to minimize binding changes.
    if (cmd_a->texture.id < cmd_b->texture.id) return -1;
    if (cmd_a->texture.id > cmd_b->texture.id) return 1;

    // Tertiary sort: Group triangles and quads together.
    if (cmd_a->is_triangle && !cmd_b->is_triangle) return -1;
    if (!cmd_a->is_triangle && cmd_b->is_triangle) return 1;

    return 0;
}

/**
 * @brief (INTERNAL) Ensures the command buffer has capacity for a minimum number of commands.
 * Automatically grows the command and vertex buffers if needed. This is the core of the
 * dynamic batching system, preventing crashes from buffer overflows.
 * 
 * @param required_command_count The minimum number of available command slots needed.
 * @return True on success, false on a fatal memory allocation failure.
 */
static bool _RGL_EnsureCommandCapacity(size_t required_command_count) {
    if (RGL.command_count + required_command_count <= RGL.command_capacity) return true; // Already have enough capacity

    size_t new_capacity = RGL.command_capacity;
    if (new_capacity == 0) new_capacity = RGL_DEFAULT_BATCH_CAPACITY;
    
    // Use a 1.5x growth factor until we have enough space.
    while (new_capacity < RGL.command_count + required_command_count) {
        new_capacity = (new_capacity * 3) / 2;
    }
    
    // Clamp to a safe maximum to prevent runaway allocations.
    if (new_capacity > RGL_MAX_BATCH_CAPACITY) new_capacity = RGL_MAX_BATCH_CAPACITY;

    if (new_capacity <= RGL.command_capacity) {
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Cannot grow batch buffer: maximum capacity reached.");
        return false;
    }

    RGLInternalDraw* new_commands = realloc(RGL.commands, sizeof(RGLInternalDraw) * new_capacity);
    if (!new_commands) {
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to reallocate command buffer");
        return false;
    }
    RGL.commands = new_commands;
    
    // Each command is a quad (6 vertices), with 10 floats each.
    const int floats_per_vertex = 10;
    size_t new_vbo_floats = new_capacity * 6 * floats_per_vertex;
    
    float* new_vbo = realloc(RGL.cpu_vertex_buffer, new_vbo_floats * sizeof(float));
    if (!new_vbo) {
        // This is not ideal, but not fatal. We can't grow the VBO, so we'll have to flush more often.
        _SituationSetWarning("Failed to grow vertex buffer, performance may be impacted.");
        // We can't update the GPU buffer size, so we leave the old capacities in place.
    } else {
        RGL.cpu_vertex_buffer = new_vbo;
        RGL.cpu_vertex_buffer_floats_capacity = new_vbo_floats;
        
        // Update the size of the buffer on the GPU to match.
        glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
        glBufferData(GL_ARRAY_BUFFER, new_vbo_floats * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    RGL.command_capacity = new_capacity;
    RGL.stats.memory_reallocations++;
    return true;
}

/**
 * @brief (INTERNAL) Processes and draws all queued commands with dynamic lighting.
 *
 * This is the heart of the renderer's performance. It follows a high-performance strategy:
 * 1.  Sorts all queued commands by Z-depth (for correct alpha blending) and then by texture
 *     (to minimize GPU state changes).
 * 2.  Assembles a single, large vertex buffer on the CPU for the new 13-float format
 *     (pos, normal, uv, color, light_level).
 * 3.  Performs frustum culling and priority sorting on all active lights in the scene.
 * 4.  Packs the data for the most important lights into a temporary struct.
 * 5.  Uploads the lighting data to the GPU's Uniform Buffer Object (UBO) in one transfer.
 * 6.  Uploads the entire vertex data buffer to the GPU's VBO in one transfer.
 * 7.  Iterates through the sorted commands, issuing the minimum number of batched 'glDrawArrays' calls.
 */
static void _RGL_FlushBatch(void) {
    if (!RGL.is_batching || RGL.command_count == 0) return;

    RGL.stats.batch_flushes++; // Path this statistic

    // --- 1. Sort Commands (from your original logic) ---
    qsort(RGL.commands, RGL.command_count, sizeof(RGLInternalDraw), _RGL_CompareDrawCommands);

    // --- 2. Assemble CPU Vertex Buffer (for the NEW 13-float format) ---
    const int floats_per_vertex = 13; // 3(pos)+3(norm)+2(uv)+4(color)+1(light)
    float* vertex_ptr = RGL.cpu_vertex_buffer;
    size_t vertices_written = 0;

    for (size_t i = 0; i < RGL.command_count; i++) {
        RGLInternalDraw* cmd = &RGL.commands[i];
        
        size_t verts_in_cmd = cmd->is_triangle ? 3 : 6;
        if ((vertices_written + verts_in_cmd) * floats_per_vertex > RGL.cpu_vertex_buffer_floats_capacity) {
            _SituationSetWarning("RGL batch capacity reached. Some draw commands were dropped.");
            break;
        }

        if (cmd->is_triangle) {
            for (int v = 0; v < 3; v++) {
                memcpy(vertex_ptr, cmd->world_positions[v], sizeof(vec3)); vertex_ptr += 3;
                memcpy(vertex_ptr, cmd->normals[v],         sizeof(vec3)); vertex_ptr += 3;
                memcpy(vertex_ptr, cmd->tex_coords[v],       sizeof(vec2)); vertex_ptr += 2;
                memcpy(vertex_ptr, cmd->colors[v],           sizeof(vec4)); vertex_ptr += 4;
                *vertex_ptr++ = cmd->light_levels[v];
            }
            vertices_written += 3;
        } else {
            const int indices[] = { 0, 1, 2, 0, 2, 3 };
            for (int v = 0; v < 6; v++) {
                int idx = indices[v];
                memcpy(vertex_ptr, cmd->world_positions[idx], sizeof(vec3)); vertex_ptr += 3;
                memcpy(vertex_ptr, cmd->normals[idx],         sizeof(vec3)); vertex_ptr += 3;
                memcpy(vertex_ptr, cmd->tex_coords[idx],       sizeof(vec2)); vertex_ptr += 2;
                memcpy(vertex_ptr, cmd->colors[idx],           sizeof(vec4)); vertex_ptr += 4;
                *vertex_ptr++ = cmd->light_levels[idx];
            }
            vertices_written += 6;
        }
    }

    // --- 3. Setup OpenGL State & Common Uniforms ---
    glUseProgram(RGL.main_shader.gl_program_id);
    glUniformMatrix4fv(RGL.loc_view, 1, GL_FALSE, (const GLfloat*)RGL.current_view_matrix);
    glUniformMatrix4fv(RGL.loc_projection, 1, GL_FALSE, (const GLfloat*)RGL.current_projection_matrix);
    glUniform1i(RGL.loc_texture_sampler, 0);

    // --- NEW: Set Lighting Uniforms (from the patch) ---
    glUniform3fv(RGL.loc_camera_pos, 1, RGL.camera_position);
    glUniform3fv(RGL.loc_ambient_light_color, 1, RGL.ambient_light_color);

    // --- 4. Cull, Sort, and Upload Light Data to UBO (from the patch) ---
    // This entire block is new and self-contained.
    {
        // Define the UBO data structure on the CPU side.
        struct {
            vec4 pos_type[MAX_SHADER_LIGHTS];
            vec4 color_intensity[MAX_SHADER_LIGHTS];
            vec4 direction[MAX_SHADER_LIGHTS];
            vec4 params[MAX_SHADER_LIGHTS];
        } light_block_data;
        memset(&light_block_data, 0, sizeof(light_block_data));

        // PASS 1: Extract Frustum Planes
        vec4 frustum_planes[6];
        mat4 view_proj;
        glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, view_proj);
        _RGL_ExtractFrustumPlanes(view_proj, frustum_planes);

        // PASS 2: Find all lights inside the frustum and score them
        typedef struct { int light_index; float score; } RGLRankedLight;
        RGLRankedLight potential_lights[RGL_MAX_LIGHTS];
        int potential_count = 0;

        ma_mutex_lock(&RGL.light_mutex);
        for (int i = 0; i < RGL_MAX_LIGHTS; i++) {
            if (RGL.lights[i].is_active) {
                bool is_visible = false;
                float score = 0.0f;

                if (RGL.lights[i].type == RGL_LIGHT_TYPE_DIRECTIONAL) {
                    is_visible = true; score = -1.0f; // Always visible, highest priority
                } else {
                    float bias_to_use = (RGL.lights[i].culling_bias > 0.0f) ? RGL.lights[i].culling_bias : RGL_LIGHT_CULLING_BIAS;
                    if (_RGL_FrustumIntersectsSphere(frustum_planes, RGL.lights[i].position, RGL.lights[i].radius, bias_to_use)) {
                        is_visible = true; score = glm_vec3_distance2(RGL.camera_position, RGL.lights[i].position);
                    }
                }
                
                if (is_visible && potential_count < RGL_MAX_LIGHTS) {
                    potential_lights[potential_count].light_index = i;
                    potential_lights[potential_count].score = score;
                    potential_count++;
                }
            }
        }
        ma_mutex_unlock(&RGL.light_mutex);

        // PASS 3: Sort the visible lights by their score (distance or priority)
        if (potential_count > 1) {
            // (You will need to add this comparison function helper)
            qsort(potential_lights, potential_count, sizeof(RGLRankedLight), _RGL_CompareRankedLights);
        }

        // PASS 4: Pack and Upload the BEST lights to the UBO
        int lights_to_upload = potential_count > MAX_SHADER_LIGHTS ? MAX_SHADER_LIGHTS : potential_count;
        glUniform1i(RGL.loc_active_lights, lights_to_upload);
        RGL.stats.active_lights_per_frame = lights_to_upload;

        if (lights_to_upload > 0) {
            ma_mutex_lock(&RGL.light_mutex);
            for (int i = 0; i < lights_to_upload; i++) {
                RGLLight* light = &RGL.lights[potential_lights[i].light_index];
                SituationConvertColorToVec4(light->color, light_block_data.color_intensity[i]);
                light_block_data.color_intensity[i][3] = light->intensity;
                light_block_data.pos_type[i][3] = (float)light->type;
                switch (light->type) {
                    case RGL_LIGHT_TYPE_POINT:
                        glm_vec3_to_vec4(light->position, 1.0f, light_block_data.pos_type[i]);
                        light_block_data.params[i][0] = light->radius;
                        break;
                    case RGL_LIGHT_TYPE_DIRECTIONAL:
                        glm_vec3_to_vec4(light->direction, 0.0f, light_block_data.direction[i]);
                        break;
                    case RGL_LIGHT_TYPE_SPOT:
                        glm_vec3_to_vec4(light->position, 1.0f, light_block_data.pos_type[i]);
                        glm_vec3_to_vec4(light->direction, 0.0f, light_block_data.direction[i]);
                        light_block_data.params[i][0] = light->radius;
                        light_block_data.params[i][1] = cosf(glm_rad(light->spot_outer_angle));
                        light_block_data.params[i][2] = cosf(glm_rad(light->spot_inner_angle));
                        break;
                }
            }
            ma_mutex_unlock(&RGL.light_mutex);

            glBindBuffer(GL_UNIFORM_BUFFER, RGL.light_ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, lights_to_upload * sizeof(vec4) * 4, &light_block_data);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    } // End of lighting block

    // --- 5. Upload Vertex Data and Issue Draw Calls (from your original logic) ---
    glBindVertexArray(RGL.batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_written * floats_per_vertex * sizeof(float), RGL.cpu_vertex_buffer);
    
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);

    size_t vertex_offset = 0;
    for (size_t i = 0; i < RGL.command_count; ) {
        GLuint current_texture_id = RGL.commands[i].texture.id;
        size_t vertices_in_batch = 0;
        size_t j = i;
        while (j < RGL.command_count && RGL.commands[j].texture.id == current_texture_id) {
            vertices_in_batch += RGL.commands[j].is_triangle ? 3 : 6;
            j++;
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, current_texture_id);
        glUniform1i(RGL.loc_use_texture, current_texture_id != 0);
        
        if (vertices_in_batch > 0) {
            glDrawArrays(GL_TRIANGLES, vertex_offset, vertices_in_batch);
            RGL.stats.total_draw_calls++;
            RGL.stats.total_vertices_drawn += vertices_in_batch;
        }
        vertex_offset += vertices_in_batch;
        i = j;
    }

    // --- 6. Cleanup (from your original logic) ---
    glBindVertexArray(0);
    glUseProgram(0);
    RGL.command_count = 0;
}

/**
 * @brief (INTERNAL) Calculates the 3D surface normal for a banked Path surface.
 * @param point The path point containing banking and elevation information.
 * @param lateral_offset Distance from Path centerline (-1.0 = left edge, +1.0 = right edge).
 * @param out_normal A vec3 to store the resulting surface normal vector.
 */
static void _RGL_CalculateBankedSurface(RGLPathPoint* point, float lateral_offset, vec3 out_normal) {
    vec3 up = {0.0f, 1.0f, 0.0f};
    if (fabsf(point->path_roll_degrees) < 0.01f) {
        glm_vec3_copy(up, out_normal);
        return;
    }
    
    mat4 bank_matrix;
    glm_rotate_make(bank_matrix, glm_rad(point->path_roll_degrees), (vec3){0.0f, 0.0f, 1.0f});
    glm_mat4_mulv3(bank_matrix, up, 1.0f, out_normal);
}

/**
 * @brief (INTERNAL) Initializes debug rendering resources. Called automatically when needed.
 */
static bool _RGL_InitDebugRendering(void) {
    if (RGL.debug.debug_initialized) return true;
    
    // --- STEP 1: DELEGATE SHADER CREATION TO situation.h ---
    // We now use the public, robust Situation API.
    RGL.debug.wireframe_shader = SituationLoadShaderFromMemory(RGL_WIREFRAME_VERTEX_SHADER, RGL_WIREFRAME_FRAGMENT_SHADER);
    
    // The returned type is now SituationShader, so we need to check its `id` field.
    if (RGL.debug.wireframe_shader.gl_program_id == 0) {
        // No need to set an error here, situation.h already did!
        // We can just log it for RGL-specific context if we want.
        fprintf(stderr, "RGL Error: Failed to initialize debug wireframe shader.\n");
        return false;
    }
    
    // --- STEP 2: DELEGATE UNIFORM LOCATION TO situation.h ---
    RGL.debug.wireframe_mvp_loc = SituationGetShaderLocation(RGL.debug.wireframe_shader, "mvp");
    RGL.debug.wireframe_color_loc = SituationGetShaderLocation(RGL.debug.wireframe_shader, "color");
    
    // --- STEP 3: RGL KEEPS ITS OWN RESPONSIBILITY ---
    // This part is unchanged, because managing the VAO/VBO for drawing
    // wireframes is the specific job of the RGL debug renderer.
    glGenVertexArrays(1, &RGL.debug.wireframe_vao);
    glGenBuffers(1, &RGL.debug.wireframe_vbo);
    glBindVertexArray(RGL.debug.wireframe_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.debug.wireframe_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 24, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    RGL.debug.debug_initialized = true;
    return true;
}

/**
 * @brief (INTERNAL) Frees all resources used by the debug rendering system.
 */
static void _RGL_ShutdownDebugRendering(void) {
    if (RGL.debug.debug_initialized) {
        glDeleteProgram(RGL.debug.wireframe_shader);
        glDeleteVertexArrays(1, &RGL.debug.wireframe_vao);
        glDeleteBuffers(1, &RGL.debug.wireframe_vbo);
        RGL.debug.debug_initialized = false;
    }
}

// --- Public API Implementations ---

/**
 * @brief Initializes the RGL renderer. Must be called after SituationInit().
 *
 * This function sets up all necessary OpenGL resources, including compiling shaders,
 * generating GPU buffers (VAO/VBO), and allocating client-side memory for the
 * high-performance command batching system.
 *
 * @return True on success, false on failure. Errors are reported via SituationGetLastErrorMsg().
 */
SITAPI bool RGL_Init(void) {
    if (RGL.is_initialized) return true;
    if (!SituationIsInitialized()) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL_Init requires SituationInit() to be called first.");
        return false;
    }

    LTInitInfo tex_init_info = { .renderer_type = LT_RENDERER_OPENGL };
    if (LTInit(&tex_init_info) != LT_SUCCESS) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INIT_FAILED, "Failed to initialize lib_tex for RGL.");
        return false;
    }
    
    memset(&RGL, 0, sizeof(RGLState));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &RGL.default_fbo);

    // 1. --- Compile Shader and Get Uniform Locations (from the patch) ---
    RGL.main_shader = SituationLoadShaderFromMemory(RGL_VERTEX_SHADER, RGL_FRAGMENT_SHADER);
    if (RGL.main_shader.gl_program_id == 0) return false;

    RGL.loc_view = SituationGetShaderLocation(RGL.main_shader, "view");
    RGL.loc_projection = SituationGetShaderLocation(RGL.main_shader, "projection");
    RGL.loc_texture_sampler = SituationGetShaderLocation(RGL.main_shader, "textureSampler");
    RGL.loc_use_texture = SituationGetShaderLocation(RGL.main_shader, "useTexture");
    RGL.loc_camera_pos = SituationGetShaderLocation(RGL.main_shader, "u_camera_pos");
    RGL.loc_ambient_light_color = SituationGetShaderLocation(RGL.main_shader, "u_ambient_light_color");
    RGL.loc_active_lights = SituationGetShaderLocation(RGL.main_shader, "u_active_lights");
    
    // 2. --- Allocate CPU-side Buffers (from the patch) ---
    RGL.command_capacity = RGL_DEFAULT_BATCH_CAPACITY;
    RGL.commands = (RGLInternalDraw*)malloc(sizeof(RGLInternalDraw) * RGL.command_capacity);
    const int floats_per_vertex = 13; // Using the new 13-float format
    RGL.cpu_vertex_buffer_floats_capacity = RGL.command_capacity * 6 * floats_per_vertex;
    RGL.cpu_vertex_buffer = (float*)malloc(RGL.cpu_vertex_buffer_floats_capacity * sizeof(float));

    if (!RGL.commands || !RGL.cpu_vertex_buffer) {
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to create RGL command/vertex buffers.");
        free(RGL.commands); free(RGL.cpu_vertex_buffer);
        SituationUnloadShader(RGL.main_shader);
        memset(&RGL, 0, sizeof(RGLState));
        return false;
    }

    // 3. --- Setup GPU Buffers (VAO and VBO) (from the patch) ---
    size_t vbo_capacity_bytes = RGL.cpu_vertex_buffer_floats_capacity * sizeof(float);
    glGenVertexArrays(1, &RGL.batch_vao);
    glGenBuffers(1, &RGL.batch_vbo);
    glBindVertexArray(RGL.batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
    glBufferData(GL_ARRAY_BUFFER, vbo_capacity_bytes, NULL, GL_DYNAMIC_DRAW);

    // 4. --- Set Vertex Attribute Pointers (from the patch) ---
    size_t stride = floats_per_vertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);    // aPos (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);    // aNormal (vec3)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);    // aTexCoord (vec2)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);    // aColor (vec4)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(12 * sizeof(float))); glEnableVertexAttribArray(4);    // aBaseLightLevel (float)
    glBindVertexArray(0);

    // 5. --- Initialize Lighting System (from the patch) ---
    memset(RGL.lights, 0, sizeof(RGL.lights));
    glm_vec3_copy((vec3){0.1f, 0.1f, 0.1f}, RGL.ambient_light_color);
    ma_mutex_init(&RGL.light_mutex);

    // Create and configure the Uniform Buffer Object (UBO) for lights
    glGenBuffers(1, &RGL.light_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, RGL.light_ubo);
    glBufferData(GL_UNIFORM_BUFFER, RGL_MAX_LIGHTS * sizeof(vec4) * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // Bind the UBO to the global binding point 0
    GLuint block_index = glGetUniformBlockIndex(RGL.main_shader.gl_program_id, "LightBlock");
    glUniformBlockBinding(RGL.main_shader.gl_program_id, block_index, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, RGL.light_ubo);

    // 6. --- Initialize Shadow Shader ---
    RGL.shadow_shader = SituationCreateShader(RGL_SHADOW_VERTEX_SHADER, RGL_SHADOW_FRAGMENT_SHADER);
    if (RGL.shadow_shader.gl_program_id == 0) {
        SIT_Log(SIT_ERROR, "RGL: Failed to create shadow shader program.");
        RGL_Shutdown(); // Clean up what was already initialized
        return false;
    }
    // Get shadow shader uniform locations
    glUseProgram(RGL.shadow_shader.gl_program_id);
    RGL.loc_shadow_view = glGetUniformLocation(RGL.shadow_shader.gl_program_id, "view");
    RGL.loc_shadow_projection = glGetUniformLocation(RGL.shadow_shader.gl_program_id, "projection");
    RGL.loc_shadow_texture = glGetUniformLocation(RGL.shadow_shader.gl_program_id, "texture0");
    RGL.loc_shadow_tint = glGetUniformLocation(RGL.shadow_shader.gl_program_id, "shadowTint");

    RGL.shadow_volume_shader = SituationCreateShader(RGL_SHADOW_VOLUME_VERTEX_SHADER, NULL); // Vertex only
    if (RGL.shadow_volume_shader.gl_program_id == 0) { SIT_Log(SIT_ERROR, "Failed to create shadow volume shader."); return false; }
    RGL.loc_sv_view = glGetUniformLocation(RGL.shadow_volume_shader.gl_program_id, "view");
    RGL.loc_sv_projection = glGetUniformLocation(RGL.shadow_volume_shader.gl_program_id, "projection");

    RGL.shadow_darken_shader = SituationCreateShader(RGL_WIREFRAME_VERTEX_SHADER, RGL_SHADOW_PASS_FRAGMENT_SHADER); // Re-use simple VS
    if (RGL.shadow_darken_shader.gl_program_id == 0) { SIT_Log(SIT_ERROR, "Failed to create shadow darken shader."); return false; }
    RGL.loc_sd_shadow_color = glGetUniformLocation(RGL.shadow_darken_shader.gl_program_id, "u_shadow_color");
    glGenVertexArrays(1, &RGL.fullscreen_quad_vao);

    // 7. --- Initialize the Scenery Style Registry ---
    memset(g_scenery_styles, 0, sizeof(g_scenery_styles));
    RGL_RegisterSceneryStyle(RGL_SCENERY_SPRITE,           &g_default_sprite_style);
    RGL_RegisterSceneryStyle(RGL_SCENERY_ARCH,             &g_default_arch_style);
    RGL_RegisterSceneryStyle(RGL_SCENERY_EVENT_MARKER,     &g_default_event_marker_style);
    RGL_RegisterSceneryStyle(RGL_SCENERY_JUNCTION_TRIGGER, &g_default_junction_style);
    RGL_RegisterSceneryStyle(RGL_SCENERY_LEVEL_ENTRANCE,   &g_default_level_entrance_style);
    
    // 8. --- Final State Setup (from your original logic) ---
    RGL.Paths = NULL; RGL.Path_count = 0; RGL.Path_capacity = 0; RGL.active_Path_index = -1;
    RGL.levels = NULL; RGL.level_count = 0; RGL.level_capacity = 0; RGL.active_level_index = -1;
    glm_mat4_identity(RGL.transform);
    RGL.use_transform = false;
    RGL.is_initialized = true;

    return true;
}

/**
 * @brief Shuts down the RGL renderer and frees all associated resources.
 * This function ensures a complete cleanup of all CPU and GPU resources,
 * including the batching system, Path and level data, debug systems,
 * and the dynamic lighting system (UBO and mutex).
 */
SITAPI void RGL_Shutdown(void) {
    if (!RGL.is_initialized) return;

    // 1. --- Shutdown Sub-systems (from your original logic) ---
    _RGL_ShutdownDebugRendering();
    _RGL_ShutdownDebugTextSystem();
    
    // 2. --- Destroy High-Level World Data (from your original logic) ---
    for (size_t i = 0; i < RGL.Path_count; i++) {
        free(RGL.Paths[i].data.points);
    }
    free(RGL.Paths);

    for (size_t i = 0; i < RGL.level_count; i++) {
        RGLLevel* level = &RGL.levels[i];
        free(level->vertices);
        free(level->walls);
        free(level->things);
        for (size_t j = 0; j < level->flat_count; j++) {
            free(level->flats[j].vertex_indices);
        }
        free(level->flats);
    }
    free(RGL.levels);
    
    // 3. --- Destroy Lighting System Resources (from the patch) ---
    ma_mutex_uninit(&RGL.light_mutex);
    glDeleteBuffers(1, &RGL.light_ubo);

    // 4. --- Destroy Core OpenGL Objects (from your original logic) ---
    glDeleteVertexArrays(1, &RGL.batch_vao);
    glDeleteBuffers(1, &RGL.batch_vbo);
    SituationUnloadShader(RGL.main_shader);
    SituationUnloadShader(RGL.shadow_shader);
    SituationDestroyShader(RGL.shadow_volume_shader);
    SituationDestroyShader(RGL.shadow_darken_shader);
    glDeleteVertexArrays(1, &RGL.fullscreen_quad_vao);
    
    // 5. --- Free Core CPU-side Memory (from your original logic) ---
    free(RGL.commands);
    free(RGL.cpu_vertex_buffer);

    // 6 --- Shutdown lib_tex
    LTShutdown();
    
    // 7. --- Zero out the global state (from your original logic) ---
    memset(&RGL, 0, sizeof(RGLState));
}

SITAPI void RGL_Begin(int virtual_display_id) {
    if (!RGL.is_initialized) { _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized"); return; }
    if (RGL.is_batching) _RGL_FlushBatch();

    // Reset per-frame stats
    RGL.stats.total_draw_calls = 0;
    RGL.stats.total_vertices_drawn = 0;
    RGL.stats.batch_flushes = 0;
    
    RGL.is_batching = true;
    RGL.command_count = 0;
    RGL.active_virtual_display_id = virtual_display_id;

    // Get the viewport size ONCE at the beginning of the render pass.
    int width, height;
    SituationGetVirtualDisplaySize(virtual_display_id, &width, &height);

    // Store it in the RGL state for other functions to use.
    RGL.viewport = (Rectangle){ 0.0f, 0.0f, (float)width, (float)height };
    
    // Apply the viewport to OpenGL.
    glViewport(0, 0, width, height);

    // Set a default 2D camera. This function will now use the RGL.viewport we just set.
    RGL_SetCamera2D((vec2){(float)width / 2.0f, (float)height / 2.0f}, 0.0f, 1.0f);
}

SITAPI void RGL_End(void) {
    if (!RGL.is_initialized) { _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized"); return; }
    if (!RGL.is_batching) return;
    _RGL_FlushBatch();
    if (RGL.active_virtual_display_id >= 0) SituationSetVirtualDisplayDirty(RGL.active_virtual_display_id, true);
    RGL.is_batching = false;
}

/**
 * @brief Pushes the current view and projection matrices onto the camera stack.
 * This saves the current camera state, allowing it to be restored later with RGL_PopMatrix().
 * Useful for functions that need to temporarily switch to a 2D UI camera.
 */
SITAPI void RGL_PushMatrix(void) {
    if (RGL.matrix_stack_ptr >= RGL_MATRIX_STACK_DEPTH - 1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_STACK_OVERFLOW, "RGL matrix stack overflow. Too many nested Pushes.");
        return;
    }

    // Save the current camera state to the stack
    glm_mat4_copy(RGL.current_view_matrix, RGL.matrix_stack[RGL.matrix_stack_ptr].view);
    glm_mat4_copy(RGL.current_projection_matrix, RGL.matrix_stack[RGL.matrix_stack_ptr].projection);

    RGL.matrix_stack_ptr++;
}

/**
 * @brief Pops the last saved view and projection matrices from the camera stack.
 * This restores the camera state that was saved by the last call to RGL_PushMatrix().
 */
SITAPI void RGL_PopMatrix(void) {
    if (RGL.matrix_stack_ptr <= 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_STACK_UNDERFLOW, "RGL matrix stack underflow. Pop without a Push.");
        return;
    }

    RGL.matrix_stack_ptr--;

    // Restore the camera state from the stack
    glm_mat4_copy(RGL.matrix_stack[RGL.matrix_stack_ptr].view, RGL.current_view_matrix);
    glm_mat4_copy(RGL.matrix_stack[RGL.matrix_stack_ptr].projection, RGL.current_projection_matrix);
}

SITAPI void RGL_SetAmbientLight(Color color) {
    if (!RGL.is_initialized) return;
    vec4 norm_color;
    SituationConvertColorToVec4(color, norm_color);
    glm_vec3_copy(norm_color, RGL.ambient_light_color);
}

static int _RGL_CompareRankedLights(const void* a, const void* b) {
    const RGLRankedLight* light_a = (const RGLRankedLight*)a;
    const RGLRankedLight* light_b = (const RGLRankedLight*)b;
    if (light_a->score < light_b->score) return -1;
    if (light_a->score > light_b->score) return 1;
    return 0;
}

static int _RGL_FindFreeLightSlot(void) {
    for (int i = 0; i < RGL_MAX_LIGHTS; i++) {
        if (RGL.lights[i].id == 0) return i;
    }
    _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Maximum number of RGL lights reached.");
    return -1;
}

SITAPI int RGL_CreatePointLight(vec3 position, Color color, float radius, float intensity) {
    ma_mutex_lock(&RGL.light_mutex);
    int index = _RGL_FindFreeLightSlot();
    if (index != -1) {
        RGLLight* light = &RGL.lights[index];
        memset(light, 0, sizeof(RGLLight));
        light->id = index + 1;
        light->is_active = true;
        light->type = RGL_LIGHT_TYPE_POINT;
        glm_vec3_copy(position, light->position);
        light->color = color;
        light->radius = fmaxf(0.01f, radius);
        light->intensity = fmaxf(0.0f, intensity);
    }
    ma_mutex_unlock(&RGL.light_mutex);
    return (index != -1) ? (index + 1) : -1;
}

SITAPI int RGL_CreateDirectionalLight(vec3 direction, Color color, float intensity) {
    ma_mutex_lock(&RGL.light_mutex);
    int index = _RGL_FindFreeLightSlot();
    if (index != -1) {
        RGLLight* light = &RGL.lights[index];
        memset(light, 0, sizeof(RGLLight));
        light->id = index + 1;
        light->is_active = true;
        light->type = RGL_LIGHT_TYPE_DIRECTIONAL;
        glm_vec3_normalize_to(direction, light->direction);
        light->color = color;
        light->intensity = fmaxf(0.0f, intensity);
    }
    ma_mutex_unlock(&RGL.light_mutex);
    return (index != -1) ? (index + 1) : -1;
}

SITAPI int RGL_CreateSpotLight(vec3 position, vec3 direction, Color color, float radius, float intensity, float outer_angle_deg, float inner_angle_deg) {
    ma_mutex_lock(&RGL.light_mutex);
    int index = _RGL_FindFreeLightSlot();
    if (index != -1) {
        RGLLight* light = &RGL.lights[index];
        memset(light, 0, sizeof(RGLLight));
        light->id = index + 1;
        light->is_active = true;
        light->type = RGL_LIGHT_TYPE_SPOT;
        glm_vec3_copy(position, light->position);
        glm_vec3_normalize_to(direction, light->direction);
        light->color = color;
        light->radius = fmaxf(0.01f, radius);
        light->intensity = fmaxf(0.0f, intensity);
        light->spot_outer_angle = fmaxf(0.0f, outer_angle_deg);
        light->spot_inner_angle = fmaxf(0.0f, fminf(inner_angle_deg, outer_angle_deg));
    }
    ma_mutex_unlock(&RGL.light_mutex);
    return (index != -1) ? (index + 1) : -1;
}

/**
 * @brief Creates a new dynamic point light using the YPQ color space.
 * This is a convenience wrapper for retro/CRT display emulation aesthetics.
 * 
 * @param position The 3D world-space position of the light.
 * @param ypq_color The color in the YPQ (NTSC) color space.
 * @param radius The maximum distance the light can reach.
 * @param intensity The brightness multiplier.
 * @return The ID of the new light, or -1 on failure.
 */
SITAPI int RGL_CreatePointLightYPQ(vec3 position, ColorYPQA ypq_color, float radius, float intensity) {
    // Assumes a function SituationColorFromYPQStruct exists, as it did in previous versions.
    Color rgb_color = SituationColorFromYPQ(ypq_color);
    return RGL_CreatePointLight(position, rgb_color, radius, intensity);
}

/**
 * @brief Destroys a light, freeing its slot for a new one. Thread-safe.
 */
SITAPI void RGL_DestroyLight(int light_id) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        memset(&RGL.lights[light_id - 1], 0, sizeof(RGLLight));
    }
    ma_mutex_unlock(&RGL.light_mutex);
}

SITAPI void RGL_AnimateLight(int light_id, float time, float frequency, float amplitude) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS || RGL.lights[light_id - 1].id == 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid light ID for animation.");
        return;
    }
    ma_mutex_lock(&RGL.light_mutex);
    int index = light_id - 1;
    float new_intensity = RGL.lights[index].intensity * (1.0f + amplitude * sinf(time * frequency));
    RGL.lights[index].intensity = fmaxf(0.0f, new_intensity);
    ma_mutex_unlock(&RGL.light_mutex);
}

/**
 * @brief Activates or deactivates a light. Thread-safe.
 */
SITAPI void RGL_SetLightActive(int light_id, bool active) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid light ID for activation.");
        return;
    }
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        RGL.lights[light_id - 1].is_active = active;
    }
    ma_mutex_unlock(&RGL.light_mutex); // CORRECTED
}

SITAPI void RGL_SetLightColor(int light_id, Color color) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        RGL.lights[light_id - 1].color = color;
    }
    ma_mutex_unlock(&RGL.light_mutex);
}

SITAPI void RGL_SetLightIntensity(int light_id, float intensity) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        RGL.lights[light_id - 1].intensity = fmaxf(0.0f, intensity);
    }
    ma_mutex_unlock(&RGL.light_mutex);
}

SITAPI void RGL_SetLightPosition(int light_id, vec3 position) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        glm_vec3_copy(position, RGL.lights[light_id - 1].position);
    }
    ma_mutex_unlock(&RGL.light_mutex);
}

SITAPI void RGL_SetLightDirection(int light_id, vec3 direction) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;
    ma_mutex_lock(&RGL.light_mutex);
    if (RGL.lights[light_id - 1].id == light_id) {
        glm_vec3_normalize_to(direction, RGL.lights[light_id - 1].direction);
    }
    ma_mutex_unlock(&RGL.light_mutex);
}

/**
 * @brief Sets a light's direction using Yaw, Pitch, and Roll angles in degrees.
 * This is a convenience function that converts human-readable angles into a direction
 * vector for the lighting engine.
 * 
 * @param light_id The ID of the light to update.
 * @param ypr_degrees A vec3 where:
 *                    - x = Pitch (up/down rotation)
 *                    - y = Yaw (left/right rotation)
 *                    - z = Roll (sideways tilt)
 */
SITAPI void RGL_SetLightDirectionFromYPR(int light_id, vec3 ypr_degrees) {
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS) return;

    // 1. Define a base "forward" direction. In a typical right-handed system,
    //    positive Z points "out of the screen" towards the viewer. A light
    //    pointing "into" the scene would be along the -Z axis. Let's use that.
    const vec3 base_direction = {0.0f, 0.0f, -1.0f};

    // 2. Convert the input degrees to radians, as cglm's math functions expect them.
    vec3 ypr_radians;
    glm_vec3_copy((vec3){glm_rad(ypr_degrees[0]), glm_rad(ypr_degrees[1]), glm_rad(ypr_degrees[2])}, ypr_radians);

    // 3. Create a rotation matrix from the Euler angles.
    //    cglm's glm_euler_to_mat4 typically applies rotations in Yaw (Y), then Pitch (X), then Roll (Z) order.
    mat4 rotation_matrix;
    glm_euler_to_mat4(ypr_radians, rotation_matrix);

    // 4. Multiply the base direction vector by the rotation matrix to get the new direction.
    //    The 'w' component of 0.0f is crucial, as it tells the math to treat this as a
    //    direction vector (unaffected by translation) rather than a point.
    vec3 final_direction;
    glm_mat4_mulv3(rotation_matrix, base_direction, 0.0f, final_direction);

    // 5. Call the core, vector-based function. It will handle thread-safety and normalization.
    RGL_SetLightDirection(light_id, final_direction);
}


/**
 * @brief Projects a 3D world position to a 2D screen position.
 * @param world_pos The 3D coordinate in world space.
 * @return The 2D coordinate on the screen. Y-axis is top-to-bottom.
 *         Returns {-1, -1} if the point is behind the camera's near plane.
 */
SITAPI vec2 RGL_WorldToScreen(vec3 world_pos) {
    // Combine view and projection matrices
    mat4 view_projection;
    glm_mat4_mul(RGL.projection, RGL.view, view_projection);

    // Transform world position to clip space
    vec4 world_pos_h = { world_pos[0], world_pos[1], world_pos[2], 1.0f };
    vec4 clip_pos;
    glm_mat4_mulv(view_projection, world_pos_h, clip_pos);
    
    // Check if the point is behind the camera (w <= 0)
    if (clip_pos[3] <= 0.0f) {
        return (vec2){ -1.0f, -1.0f };
    }
    
    // Perform perspective divide to get Normalized Device Coordinates (NDC) [-1, 1]
    vec3 ndc_pos;
    ndc_pos[0] = clip_pos[0] / clip_pos[3];
    ndc_pos[1] = clip_pos[1] / clip_pos[3];
    // ndc_pos[2] = clip_pos[2] / clip_pos[3]; // z is not needed for 2D screen pos

    // Map NDC to screen coordinates
    vec2 screen_pos;
    screen_pos[0] = RGL.viewport.x + (ndc_pos[0] + 1.0f) * 0.5f * RGL.viewport.width;
    // Invert Y axis because NDC Y is bottom-to-top, but screen Y is top-to-bottom
    screen_pos[1] = RGL.viewport.y + (1.0f - ndc_pos[1]) * 0.5f * RGL.viewport.height;

    return screen_pos;
}

/**
 * @brief Unprojects a 2D screen position back into a 3D world position.
 * @param screen_pos The 2D coordinate on the screen.
 * @param z_depth_normalized A value from 0.0 (near plane) to 1.0 (far plane)
 *                           determining the depth of the unprojected point.
 * @return The corresponding 3D coordinate in world space.
 */
SITAPI vec3 RGL_ScreenToWorld(vec2 screen_pos, float z_depth_normalized) {
    // Calculate inverse view-projection matrix
    mat4 view_projection;
    glm_mat4_mul(RGL.projection, RGL.view, view_projection);
    mat4 inv_view_projection;
    glm_mat4_inv(view_projection, inv_view_projection);
    
    // Convert screen coordinates to Normalized Device Coordinates (NDC)
    vec4 ndc_pos;
    ndc_pos[0] = (screen_pos[0] - RGL.viewport.x) / RGL.viewport.width * 2.0f - 1.0f;
    // Invert Y axis from screen to NDC space
    ndc_pos[1] = (1.0f - (screen_pos[1] - RGL.viewport.y) / RGL.viewport.height) * 2.0f - 1.0f;
    // Map normalized z-depth from [0, 1] to [-1, 1]
    ndc_pos[2] = z_depth_normalized * 2.0f - 1.0f;
    ndc_pos[3] = 1.0f;
    
    // Unproject NDC coordinates to world space
    vec4 world_pos_h;
    glm_mat4_mulv(inv_view_projection, ndc_pos, world_pos_h);
    
    vec3 world_pos = { 0.0f, 0.0f, 0.0f };
    // Perform perspective divide if w is not zero
    if (fabsf(world_pos_h[3]) > FLT_EPSILON) glm_vec3_scale(world_pos_h, 1.0f / world_pos_h[3], world_pos);
    
    return world_pos;
}

/**
 * @brief Checks if a 2D point is inside an axis-aligned rectangle.
 * @param point The point to check.
 * @param rect The rectangle.
 * @return True if the point is inside or on the edge of the rectangle.
 */
SITAPI bool RGL_IsPointInRectangle(vec2 point, Rectangle rect) {
    return (point[0] >= rect.x && point[0] <= (rect.x + rect.width) && point[1] >= rect.y && point[1] <= (rect.y + rect.height));
}

/**
 * @brief Checks if a 2D point is inside a circle.
 * @param point The point to check.
 * @param center The center of the circle.
 * @param radius The radius of the circle.
 * @return True if the point is inside or on the edge of the circle.
 */
SITAPI bool RGL_IsPointInCircle(vec2 point, vec2 center, float radius) {
    float dx = point[0] - center[0];
    float dy = point[1] - center[1];
    // Use squared distance to avoid a costly sqrt() call
    return (dx*dx + dy*dy) <= (radius*radius);
}

/**
 * @brief Returns a Rectangle representing the full dimensions of a texture.
 * @param texture The RGLTexture.
 * @return A Rectangle { 0, 0, width, height }.
 */
SITAPI Rectangle RGL_GetTextureRect(RGLTexture texture) {
    return (Rectangle){ 0.0f, 0.0f, (float)texture.width, (float)texture.height };
}

/**
 * @brief Returns a Rectangle representing the current screen/render target viewport.
 * @return The Rectangle stored in the RGL internal state.
 */
SITAPI Rectangle RGL_GetScreenRect(void) {
    return RGL.viewport;
}

SITAPI void RGL_SetCamera2D(vec2 target, float rotation_degrees, float zoom) {
    if (!RGL.is_initialized) { _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized"); return; }
    
    // REMOVED: No longer need to call SituationGetVirtualDisplaySize here.
    // int width, height;
    // SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &width, &height);

    // MODIFIED: Use the stored viewport dimensions.
    glm_ortho(0.0f, RGL.viewport.width, RGL.viewport.height, 0.0f, -1.0f, 1.0f, RGL.current_projection_matrix);

    // This logic is YOURS and is perfectly preserved.
    glm_mat4_identity(RGL.current_view_matrix);
    glm_translate(RGL.current_view_matrix, (vec3){target[0], target[1], 0.0f});
    glm_rotate_z(RGL.current_view_matrix, glm_rad(-rotation_degrees), RGL.current_view_matrix);
    glm_scale(RGL.current_view_matrix, (vec3){zoom, zoom, 1.0f});
    glm_translate(RGL.current_view_matrix, (vec3){-target[0], -target[1], 0.0f});
    
    // This is also preserved.
    glm_vec3_copy((vec3){target[0], target[1], 0.0f}, RGL.camera_position);
}

SITAPI void RGL_SetCamera3D(vec3 position, vec3 target, vec3 up, float fov_y_degrees) {
    if (!RGL.is_initialized) { _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized"); return; }
    
    // This is preserved.
    glm_vec3_copy(position, RGL.camera_position);
    
    // REMOVED: No longer need to call SituationGetVirtualDisplaySize here.
    // int width, height;
    // SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &width, &height);

    // MODIFIED: Use the stored viewport dimensions to calculate aspect ratio.
    float aspect = (RGL.viewport.height > 0) ? RGL.viewport.width / RGL.viewport.height : 1.0f;
    
    // These are preserved and are correct.
    glm_perspective(glm_rad(fov_y_degrees), aspect, RGL_DEFAULT_NEAR_PLANE, RGL_DEFAULT_FAR_PLANE, RGL.current_projection_matrix);
    glm_lookat(position, target, up, RGL.current_view_matrix);
}

// --- Coordinate Transformations & Geometry ---

SITAPI vec2 RGL_WorldToScreen(vec3 world_pos) {
    mat4 view_projection;
    glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, view_projection);

    vec4 world_pos_h = { world_pos[0], world_pos[1], world_pos[2], 1.0f };
    vec4 clip_pos;
    glm_mat4_mulv(view_projection, world_pos_h, clip_pos);
    
    if (clip_pos[3] <= 0.0f) {
        return (vec2){ -1.0f, -1.0f }; // Point is behind the camera
    }
    
    // Perspective divide
    vec3 ndc_pos;
    ndc_pos[0] = clip_pos[0] / clip_pos[3];
    ndc_pos[1] = clip_pos[1] / clip_pos[3];

    // Map from NDC [-1, 1] to screen [0, viewport_size]
    vec2 screen_pos;
    screen_pos[0] = RGL.viewport.x + (ndc_pos[0] + 1.0f) * 0.5f * RGL.viewport.width;
    screen_pos[1] = RGL.viewport.y + (1.0f - ndc_pos[1]) * 0.5f * RGL.viewport.height; // Invert Y

    return screen_pos;
}

SITAPI vec3 RGL_ScreenToWorld(vec2 screen_pos, float z_depth_normalized) {
    mat4 view_projection;
    glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, view_projection);
    mat4 inv_view_projection;
    glm_mat4_inv(view_projection, inv_view_projection);
    
    // Map from screen [0, viewport_size] to NDC [-1, 1]
    vec4 ndc_pos;
    ndc_pos[0] = (screen_pos[0] - RGL.viewport.x) / RGL.viewport.width * 2.0f - 1.0f;
    ndc_pos[1] = (1.0f - (screen_pos[1] - RGL.viewport.y) / RGL.viewport.height) * 2.0f - 1.0f; // Invert Y
    ndc_pos[2] = z_depth_normalized * 2.0f - 1.0f;
    ndc_pos[3] = 1.0f;
    
    vec4 world_pos_h;
    glm_mat4_mulv(inv_view_projection, ndc_pos, world_pos_h);
    
    vec3 world_pos = { 0.0f, 0.0f, 0.0f };
    if (fabsf(world_pos_h[3]) > FLT_EPSILON) {
        glm_vec3_scale(world_pos_h, 1.0f / world_pos_h[3], world_pos);
    }
    
    return world_pos;
}

SITAPI bool RGL_IsPointInRectangle(vec2 point, Rectangle rect) {
    return (point[0] >= rect.x && point[0] <= (rect.x + rect.width) && point[1] >= rect.y && point[1] <= (rect.y + rect.height));
}

SITAPI bool RGL_IsPointInCircle(vec2 point, vec2 center, float radius) {
    float dx = point[0] - center[0];
    float dy = point[1] - center[1];
    return (dx*dx + dy*dy) <= (radius*radius);
}

SITAPI Rectangle RGL_GetTextureRect(RGLTexture texture) {
    return (Rectangle){ 0.0f, 0.0f, (float)texture.width, (float)texture.height };
}

SITAPI Rectangle RGL_GetScreenRect(void) {
    return RGL.viewport;
}

/**
 * @brief Creates a new texture that can be used as a rendering target.
 *
 * This function generates an OpenGL Framebuffer Object (FBO) and attaches a new,
 * empty texture to it. You can then use RGL_SetRenderTarget() to draw into this texture.
 *
 * @param width The width of the render texture in pixels.
 * @param height The height of the render texture in pixels.
 * @return An RGLTexture configured as a render target. On failure, id and fbo_id will be 0.
 */
SITAPI RGLTexture RGL_CreateRenderTexture(int width, int height) {
    // The RGLTexture we return IS an LTTexture.
    // We just need to support a default format.
    return LTCreateRenderTexture(width, height, LT_FORMAT_RGBA8);
}

/**
 * @brief Destroys a render texture and its associated OpenGL objects.
 * @param texture The render texture to destroy.
 */
SITAPI void RGL_DestroyRenderTexture(RGLTexture texture) {
    LTTexture lt_tex = texture; // Cast for clarity
    LTDestroyTexture(&LT_tex);
}

/**
 * @brief Sets the current rendering target to a specified render texture.
 * All subsequent drawing operations will be directed to this texture instead of the screen.
 *
 * @param texture The render texture to set as the target.
 */
SITAPI void RGL_SetRenderTarget(RGLTexture texture) {
    _RGL_FlushBatch(); // RGL is responsible for flushing its own batch!
    LTSetRenderTarget(texture); // lib_tex handles the GL calls.
}

/**
 * @brief Resets the rendering target back to the main screen/window.
 */
SITAPI void RGL_ResetRenderTarget(void) {
    _RGL_FlushBatch(); // RGL's responsibility.
    LTResetRenderTarget(); // lib_tex handles the GL calls.
}

/**
 * @brief (INTERNAL HELPER) Generates and queues a 3D quad for Path segments.
 * This is now simplified to take a start and end offset from a centerline.
 *
 * @param center The 3D world-space center of the quad's near edge.
 * @param right_vec The normalized vector pointing to the "right" of the Path surface.
 * @param x_offset1 Offset from center for the first vertical edge.
 * @param x_offset2 Offset from center for the second vertical edge.
 * @param z_near World Z of the near edge.
 * @param z_far World Z of the far edge.
 * @param sprite The sprite to texture the quad with (id=0 for solid color).
 * @param color The solid color to use if not textured.
 */
static void _RGL_Draw3DQuad(vec3 center, vec3 right_vec, float x_offset1, float x_offset2, float z_near, float z_far, RGLSprite sprite, Color color) {
    if (RGL.command_count >= RGL.command_capacity) _RGL_FlushBatch();
    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];

    // Calculate the four 3D corner vertices
    vec3 p_near1, p_near2, p_far1, p_far2;
    glm_vec3_scale(right_vec, x_offset1, p_near1);
    glm_vec3_add(center, p_near1, p_near1);

    glm_vec3_scale(right_vec, x_offset2, p_near2);
    glm_vec3_add(center, p_near2, p_near2);
    
    glm_vec3_copy(p_near1, p_far1); p_far1[2] = z_far;
    glm_vec3_copy(p_near2, p_far2); p_far2[2] = z_far;

    cmd->texture = sprite.texture;
    cmd->z_depth = z_near;
    cmd->is_triangle = false;

    // Top-Left, Bottom-Left, Bottom-Right, Top-Right (for RGL_DrawSpritePro compatibility)
    glm_vec3_copy(p_near1, cmd->world_positions[0]);
    glm_vec3_copy(p_far1,  cmd->world_positions[1]);
    glm_vec3_copy(p_far2,  cmd->world_positions[2]);
    glm_vec3_copy(p_near2, cmd->world_positions[3]);

    // Assign texture coordinates
    cmd->tex_coords[0][0] = 0.0f; cmd->tex_coords[0][1] = 0.0f;
    cmd->tex_coords[1][0] = 0.0f; cmd->tex_coords[1][1] = 1.0f;
    cmd->tex_coords[2][0] = 1.0f; cmd->tex_coords[2][1] = 1.0f;
    cmd->tex_coords[3][0] = 1.0f; cmd->tex_coords[3][1] = 0.0f;

    vec4 v4_color;
    SituationConvertColorToVec4(color, v4_color);
    for(int v=0; v<4; v++) {
        glm_vec4_copy(v4_color, cmd->colors[v]);
        cmd->light_levels[v] = 1.0f; // Default full light for Path
    }

    RGL.command_count++;
}

SITAPI void RGL_DrawQuadPro(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]) {
    RGLSprite sprite = { .texture = texture, .source_rect = source_rect };
    RGL_DrawSpritePro(sprite, position, size, origin_pct, rotation_eul_deg, skew, colors, light_levels);
}

SITAPI void RGL_DrawQuad(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, Color tint) {
    Color colors[4] = {tint, tint, tint, tint};
    RGLSprite sprite = { .texture = texture, .source_rect = source_rect };
    RGL_DrawSpritePro(sprite, position, size, (vec2){0.0f, 0.0f}, (vec3){0,0,0}, (vec2){0,0}, colors, NULL);
}


/**
 * @brief Draws a textured or untextured quad with full 3D transformation, per-vertex coloring, and dynamic lighting.
 *
 * This is the ultimate low-level drawing function that all other quad-based wrappers call. It
 * correctly calculates and provides vertex normals for the lighting system and respects the global
 * transform matrix set by RGL_SetTransform.
 *
 * @param sprite The sprite (texture and source rect) to draw. For an untextured quad, use an RGLSprite with texture.id = 0.
 * @param position The 3D world-space position (X, Y, Z) where the quad's origin is placed.
 * @param size The 2D width and height of the quad in world units.
 * @param origin_pct Pivot point for rotation and scaling, as a percentage {0-1} of size (e.g., {0.5, 0.5} is center).
 * @param rotation_eul_deg Euler angles in DEGREES for {pitch(x), yaw(y), roll(z)}.
 * @param skew Skew factors {sx, sy} for shearing, applied before rotation.
 * @param colors An array of 4 Colors for each vertex in the order: [TopLeft, BottomLeft, BottomRight, TopRight].
 * @param light_levels An array of 4 base light levels [0.0 - 1.0] for each vertex, or NULL for default (1.0).
 */
SITAPI void RGL_DrawSpritePro(RGLSprite sprite, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]) {
    // 1. --- Pre-flight Checks (from your original logic) ---
    if (!RGL.is_initialized || !RGL.is_batching) return;
    if (!_RGL_EnsureCommandCapacity(1)) return; // Ensure we have room in the batch

    // Get a pointer to the next available command slot in our batch queue.
    // NOTE: We do NOT increment the count here. We only increment it at the very end on success.
    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];

    // 2. --- Calculate Model and Rotation-Only Matrices (from your original code) ---
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    glm_translate(model_matrix, position);

    vec3 origin_offset = { origin_pct[0] * size[0], origin_pct[1] * size[1], 0.0f };
    glm_translate(model_matrix, origin_offset);

    mat4 rotation_matrix;
    vec3 rad_angles = { glm_rad(rotation_eul_deg[0]), glm_rad(rotation_eul_deg[1]), glm_rad(rotation_eul_deg[2]) };
    glm_euler_to_mat4(rad_angles, rotation_matrix);
    glm_mat4_mul(model_matrix, rotation_matrix, model_matrix);

    if (skew[0] != 0.0f || skew[1] != 0.0f) {
        mat4 shear_matrix = { {1.0f, skew[1], 0.0f, 0.0f}, {skew[0], 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f} };
        glm_mat4_mul(model_matrix, shear_matrix, model_matrix);
    }

    vec3 neg_origin_offset;
    glm_vec3_negate_to(origin_offset, neg_origin_offset);
    glm_translate(model_matrix, neg_origin_offset);

    // --- NEW: Apply the global RGL transform if it's active ---
    if (RGL.use_transform) {
        // Multiply the final model_matrix by the global transform.
        glm_mat4_mul(RGL.transform, model_matrix, model_matrix);
        
        // Also apply the global rotation to our isolated rotation_matrix for correct normal calculation.
        mat4 global_rot_only;
        glm_mat4_copy(RGL.transform, global_rot_only);
        global_rot_only[3][0] = global_rot_only[3][1] = global_rot_only[3][2] = 0.0f; // Zero out translation part
        glm_mat4_mul(global_rot_only, rotation_matrix, rotation_matrix);
    }

    // 3. --- Define Local Vertices and Transform to World Space (from your original code) ---
    // The order here is TL, BL, BR, TR, which is what the batcher expects.
    vec3 local_verts[4] = {
        {-size[0] * origin_pct[0], -size[1] * origin_pct[1], 0.0f},
        {-size[0] * origin_pct[0],  size[1] * (1.0f - origin_pct[1]), 0.0f},
        { size[0] * (1.0f - origin_pct[0]),  size[1] * (1.0f - origin_pct[1]), 0.0f},
        { size[0] * (1.0f - origin_pct[0]), -size[1] * origin_pct[1], 0.0f}
    };
    for (int i = 0; i < 4; ++i) {
        glm_mat4_mulv3(model_matrix, local_verts[i], 1.0f, cmd->world_positions[i]);
    }
    
    // 4. --- Calculate and Transform Normal Vector (from the patch) ---
    const vec3 base_normal = {0.0f, 0.0f, 1.0f};
    vec3 final_normal;
    glm_mat4_mulv3(rotation_matrix, base_normal, 0.0f, final_normal);
    glm_vec3_normalize(final_normal);

    // 5. --- Populate Rest of the Command Data ---
    cmd->texture = sprite.texture;
    cmd->z_depth = position[2]; // Use original Z for depth sorting, not transformed Z
    cmd->is_triangle = false;

    float u1 = 0.0f, v1 = 0.0f, u2 = 1.0f, v2 = 1.0f;
    if (sprite.texture.id != 0 && sprite.texture.width > 0 && sprite.texture.height > 0) {
        u1 = sprite.source_rect.x / sprite.texture.width;
        v1 = sprite.source_rect.y / sprite.texture.height;
        u2 = (sprite.source_rect.x + sprite.source_rect.width) / sprite.texture.width;
        v2 = (sprite.source_rect.y + sprite.source_rect.height) / sprite.texture.height;
    }
    
    vec2 uvs[4] = {{u1, v1}, {u1, v2}, {u2, v2}, {u2, v1}};

    for (int i = 0; i < 4; i++) {
        glm_vec3_copy(final_normal, cmd->normals[i]);
        glm_vec2_copy(uvs[i], cmd->tex_coords[i]);
        SituationConvertColorToVec4(colors[i], cmd->colors[i]);
        cmd->light_levels[i] = light_levels ? light_levels[i] : 1.0f;
    }

    // 6. --- COMMIT THE COMMAND (THE MISSING PIECE) ---
    // Now that the command is fully populated, officially add it to the batch by incrementing the counter.
    RGL.command_count++;
}

/**
 * @brief Convenience wrapper to draw a simple, tinted 2D sprite with scaling and rotation.
 * @param sprite The sprite (texture + source rect) to draw.
 * @param position The top-left 2D screen-space or world-space position of the sprite. Z is assumed to be 0.
 * @param roll_degrees The 2D rotation in degrees around the Z-axis.
 * @param scale Uniform scaling factor applied to the sprite's source size.
 * @param tint A single color to tint the entire sprite.
 */
SITAPI void RGL_DrawSprite(RGLSprite sprite, vec2 position, float roll_degrees, float scale, Color tint) {
    if (scale <= 0.0f) { _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Scale must be positive"); return; }
    vec2 size = { sprite.source_rect.width * scale, sprite.source_rect.height * scale };
    Color colors[4] = {tint, tint, tint, tint};
    RGL_DrawSpritePro(
        sprite,
        (vec3){position[0], position[1], 0.0f},
        size,
        (vec2){0.0f, 0.0f}, // Top-left origin
        (vec3){0.0f, 0.0f, roll_degrees},
        (vec2){0.0f, 0.0f}, // No skew
        colors,
        NULL
    );
}

/**
 * @brief Draws a textured quad with full transformation options.
 * This is a convenience wrapper that maps the common "destination rectangle and origin"
 * paradigm to the more flexible "position, size, and percentage origin" system used
 * by the core `RGL_DrawSpritePro` function.
 *
 * @param sprite The sprite (texture and source rect) to draw.
 * @param dest_rect The destination rectangle on the screen {x, y, width, height}.
 * @param origin The rotation pivot point in pixels, relative to the top-left of dest_rect.
 * @param rotation_degrees The 2D rotation in degrees (roll).
 * @param tint The color to tint the sprite.
 */
SITAPI void RGL_DrawTexturePro(RGLSprite sprite, Rectangle dest_rect, vec2 origin, float rotation_degrees, Color tint) {
    // 1. Check for invalid size.
    if (dest_rect.width == 0 || dest_rect.height == 0) return;

    // 2. Convert parameters to the format RGL_DrawSpritePro needs.
    vec3 position = { dest_rect.x, dest_rect.y, 0.0f };
    vec2 size = { dest_rect.width, dest_rect.height };
    
    // Calculate the origin as a percentage of the size.
    vec2 origin_pct = {
        origin[0] / size[0],
        origin[1] / size[1]
    };
    
    // Rotation is just the roll component.
    vec3 rotation_eul_deg = { 0.0f, 0.0f, rotation_degrees };
    
    // Prepare color array.
    Color colors[4] = { tint, tint, tint, tint };

    // 3. Call the master function.
    RGL_DrawSpritePro(
        sprite,
        position,
        size,
        origin_pct,
        rotation_eul_deg,
        (vec2){0.0f, 0.0f}, // No skew
        colors,
        NULL // Default full lighting
    );
}

/**
 * @brief Draws a color-filled rectangle with optional rotation.
 * The rectangle is rotated around its top-left corner.
 *
 * @param rect The rectangle's position and size {x, y, width, height}.
 * @param roll_degrees The 2D rotation in degrees.
 * @param color The fill color of the rectangle.
 */
SITAPI void RGL_DrawRectangle(Rectangle rect, float roll_degrees, Color color) {
    if (!RGL.is_initialized || !RGL.is_batching) return;
    
    // Draw an untextured shape by passing a zero-ID sprite and a zero-pixel origin.
    RGL_DrawTexturePro((RGLSprite){0}, rect, (vec2){0.0f, 0.0f}, roll_degrees, color);
}

/**
 * @brief Draws a line with specified thickness and color, compatible with the lighting pipeline.
 *
 * REASON FOR CHANGE: The previous implementation used a long chain of abstractions
 * (DrawLineEx -> DrawRectangle -> DrawTexturePro -> DrawSpritePro), which involved
 * unnecessary matrix calculations. This new version directly calculates the four
 * vertices of a quad representing the line and queues a single, low-level command.
 * It is significantly more performant and provides the necessary data (normals, light levels)
 * for the new lighting system.
 *
 * @param start_pos The starting 2D coordinate of the line.
 * @param end_pos The ending 2D coordinate of the line.
 * @param thick The thickness of the line in world/screen units.
 * @param color The color of the line.
 */
SITAPI void RGL_DrawLineEx(vec2 start_pos, vec2 end_pos, float thick, Color color) {
    // 1. --- Pre-flight Checks ---
    if (!RGL.is_batching || thick <= 0.0f) return;
    if (!_RGL_EnsureCommandCapacity(1)) return;

    // 2. --- Direct Vertex Calculation (from the patch) ---
    // This is the efficient core of the new function.
    vec2 delta;
    glm_vec2_sub(end_pos, start_pos, delta);
    float length = glm_vec2_norm(delta);
    if (length < 0.001f) return; // Don't draw zero-length lines.

    // Calculate a normalized direction vector and its perpendicular
    vec2 dir = { delta[0] / length, delta[1] / length };
    vec2 perp = { -dir[1], dir[0] }; // Rotated 90 degrees
    glm_vec2_scale(perp, thick * 0.5f, perp);

    // Calculate the 4 corners of the quad in 3D space (Z=0 for 2D lines)
    vec3 p1, p2, p3, p4; // bl, br, tr, tl
    glm_vec3_copy((vec3){ start_pos[0] - perp[0], start_pos[1] - perp[1], 0.0f }, p1);
    glm_vec3_copy((vec3){ end_pos[0] - perp[0],   end_pos[1] - perp[1],   0.0f }, p2);
    glm_vec3_copy((vec3){ end_pos[0] + perp[0],   end_pos[1] + perp[1],   0.0f }, p3);
    glm_vec3_copy((vec3){ start_pos[0] + perp[0],   start_pos[1] + perp[1],   0.0f }, p4);

    // 3. --- Queue the Command Directly ---
    // Use the safe "populate-then-commit" pattern.
    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];
    
    cmd->texture.id = 0; // Untextured
    cmd->is_triangle = false;
    cmd->z_depth = 0.0f; // Assume 2D UI element, drawn on top.

    // Vertices in the order expected by the batcher (TL, BL, BR, TR)
    glm_vec3_copy(p4, cmd->world_positions[0]);
    glm_vec3_copy(p1, cmd->world_positions[1]);
    glm_vec3_copy(p2, cmd->world_positions[2]);
    glm_vec3_copy(p3, cmd->world_positions[3]);

    // 4. --- Populate All Vertex Attributes for the New Pipeline ---
    vec4 v4_color;
    SituationConvertColorToVec4(color, v4_color);
    const vec3 normal = {0.0f, 0.0f, 1.0f}; // Standard normal for 2D UI elements.

    for (int i = 0; i < 4; i++) {
        glm_vec2_zero(cmd->tex_coords[i]);      // Dummy UVs
        glm_vec4_copy(v4_color, cmd->colors[i]);
        glm_vec3_copy(normal, cmd->normals[i]); // The essential normal vector
        cmd->light_levels[i] = 1.0f;            // 2D lines should always be fully bright
    }

    // 5. --- Commit the Command ---
    RGL.command_count++;
}

/**
 * @brief Creates a faded version of a color (RGL equivalent of Raylib's Fade)
 */
SITAPI Color RGL_FadeColor(Color color, float alpha) {
    // Clamp alpha to valid range
    alpha = fmaxf(0.0f, fminf(1.0f, alpha));
    
    return (Color){
        color.r,
        color.g, 
        color.b,
        (unsigned char)(color.a * alpha)
    };
}

/**
 * @brief Creates a Color from Hue, Saturation, and Value components.
 * @param hue The hue angle, in degrees (0.0f to 360.0f).
 * @param saturation The saturation (0.0f for grayscale, 1.0f for full color).
 * @param value The value/brightness (0.0f for black, 1.0f for full intensity).
 * @return The corresponding RGBA Color struct. Alpha is always 255.
 */
SITAPI Color RGL_ColorFromHSV(float hue, float saturation, float value) {
    Color c = { 0, 0, 0, 255 };
    saturation = _RGL_Clamp01(saturation);
    value = _RGL_Clamp01(value);
    
    // If grayscale, just set all components to value
    if (saturation <= 0.0f) {
        unsigned char v = (unsigned char)(value * 255.0f + 0.5f);
        c.r = v; c.g = v; c.b = v;
        return c;
    }

    hue = fmodf(hue, 360.0f);
    if (hue < 0.0f) hue += 360.0f;
    hue /= 60.0f; // Scale hue to [0, 6)

    int i = (int)floorf(hue);
    float f = hue - i;
    float p = value * (1.0f - saturation);
    float q = value * (1.0f - (saturation * f));
    float t = value * (1.0f - (saturation * (1.0f - f)));
    
    float r = 0.0f, g = 0.0f, b = 0.0f;

    switch (i) {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break; // case 5
    }
    
    c.r = (unsigned char)(r * 255.0f + 0.5f);
    c.g = (unsigned char)(g * 255.0f + 0.5f);
    c.b = (unsigned char)(b * 255.0f + 0.5f);
    
    return c;
}

/**
 * @brief Converts an RGBA Color to a vec3 representing Hue, Saturation, and Value.
 * @param color The input Color struct.
 * @return A vec3 where: x=hue(0-360), y=saturation(0-1), z=value(0-1).
 */
SITAPI vec3 RGL_ColorToHSV(Color color) {
    vec3 hsv = { 0.0f, 0.0f, 0.0f };
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    float max_val = fmaxf(r, fmaxf(g, b));
    float min_val = fminf(r, fminf(g, b));
    float delta = max_val - min_val;
    
    // Value
    hsv[2] = max_val;
    
    // Saturation
    if (max_val > FLT_EPSILON) {
        hsv[1] = delta / max_val;
    } else {
        // r = g = b = 0, so S = 0, V = 0, H is undefined (but usually 0)
        hsv[1] = 0.0f;
        hsv[0] = 0.0f;
        return hsv;
    }
    
    // Hue
    if (delta > FLT_EPSILON) {
        if (max_val == r) {
            hsv[0] = (g - b) / delta;
        } else if (max_val == g) {
            hsv[0] = 2.0f + (b - r) / delta;
        } else { // max_val == b
            hsv[0] = 4.0f + (r - g) / delta;
        }
        
        hsv[0] *= 60.0f;
        if (hsv[0] < 0.0f) hsv[0] += 360.0f;
    } else {
        hsv[0] = 0.0f; // Grayscale, hue is undefined
    }
    
    return hsv;
}

/**
 * @brief Creates a Color from a 24-bit or 32-bit hexadecimal value.
 * @param hex_value The hexadecimal color code (e.g., 0xFF0000 for red).
 *                  Handles 0xRRGGBB (alpha defaults to 255) and 0xAARRGGBB formats.
 * @return The corresponding RGBA Color struct.
 */
SITAPI Color RGL_ColorFromHex(unsigned int hex_value) {
    Color c;
    // Check if an alpha channel is likely present (value is larger than 24-bit max)
    if (hex_value > 0xFFFFFF) {
        c.a = (hex_value >> 24) & 0xFF;
        c.r = (hex_value >> 16) & 0xFF;
        c.g = (hex_value >> 8) & 0xFF;
        c.b = (hex_value) & 0xFF;
    } else {
        c.a = 255;
        c.r = (hex_value >> 16) & 0xFF;
        c.g = (hex_value >> 8) & 0xFF;
        c.b = (hex_value) & 0xFF;
    }
    return c;
}

/**
 * @brief Converts a Color struct to a 32-bit hexadecimal value (AARRGGBB).
 * @param color The input Color struct.
 * @return The 32-bit unsigned integer representation.
 */
SITAPI unsigned int RGL_ColorToHex(Color color) {
    return ((unsigned int)color.a << 24) |
           ((unsigned int)color.r << 16) |
           ((unsigned int)color.g << 8)  |
           ((unsigned int)color.b);
}

/**
 * @brief Linearly interpolates between two colors.
 * @param c1 The starting color.
 * @param c2 The ending color.
 * @param t The interpolation factor, clamped to [0.0, 1.0].
 * @return The interpolated color.
 */
SITAPI Color RGL_ColorLerp(Color c1, Color c2, float t) {
    t = _RGL_Clamp01(t);
    Color result;
    result.r = (unsigned char)(RGL_Lerp((float)c1.r, (float)c2.r, t) + 0.5f);
    result.g = (unsigned char)(RGL_Lerp((float)c1.g, (float)c2.g, t) + 0.5f);
    result.b = (unsigned char)(RGL_Lerp((float)c1.b, (float)c2.b, t) + 0.5f);
    result.a = (unsigned char)(RGL_Lerp((float)c1.a, (float)c2.a, t) + 0.5f);
    return result;
}

/**
 * @brief Multiplies two colors component-wise (standard "multiply" blend mode).
 * @param c1 The first color.
 * @param c2 The second color.
 * @return The resulting color.
 */
SITAPI Color RGL_ColorMultiply(Color c1, Color c2) {
    Color result;
    result.r = (unsigned char)(((float)c1.r/255.0f * (float)c2.r/255.0f) * 255.0f + 0.5f);
    result.g = (unsigned char)(((float)c1.g/255.0f * (float)c2.g/255.0f) * 255.0f + 0.5f);
    result.b = (unsigned char)(((float)c1.b/255.0f * (float)c2.b/255.0f) * 255.0f + 0.5f);
    result.a = (unsigned char)(((float)c1.a/255.0f * (float)c2.a/255.0f) * 255.0f + 0.5f);
    return result;
}

/**
 * @brief Adds two colors component-wise, clamping each channel at 255.
 * @param c1 The first color.
 * @param c2 The second color.
 * @return The resulting color.
 */
SITAPI Color RGL_ColorAdd(Color c1, Color c2) {
    Color result;
    result.r = _RGL_ClampToU8((int)c1.r + (int)c2.r);
    result.g = _RGL_ClampToU8((int)c1.g + (int)c2.g);
    result.b = _RGL_ClampToU8((int)c1.b + (int)c2.b);
    result.a = _RGL_ClampToU8((int)c1.a + (int)c2.a);
    return result;
}

/**
 * @brief Subtracts the second color from the first, clamping each channel at 0.
 * @param c1 The first color.
 * @param c2 The second color.
 * @return The resulting color.
 */
SITAPI Color RGL_ColorSubtract(Color c1, Color c2) {
    Color result;
    result.r = _RGL_ClampToU8((int)c1.r - (int)c2.r);
    result.g = _RGL_ClampToU8((int)c1.g - (int)c2.g);
    result.b = _RGL_ClampToU8((int)c1.b - (int)c2.b);
    result.a = _RGL_ClampToU8((int)c1.a - (int)c2.a);
    return result;
}

/**
 * @brief Adjusts the brightness of a color by a multiplicative factor.
 * @param color The input color.
 * @param factor The brightness factor. >1.0 is brighter, <1.0 is darker.
 * @return The adjusted color. Alpha is unchanged.
 */
SITAPI Color RGL_ColorBrightness(Color color, float factor) {
    Color result;
    result.r = _RGL_ClampToU8((int)((float)color.r * factor));
    result.g = _RGL_ClampToU8((int)((float)color.g * factor));
    result.b = _RGL_ClampToU8((int)((float)color.b * factor));
    result.a = color.a;
    return result;
}

/**
 * @brief Adjusts the contrast of a color.
 * @param color The input color.
 * @param contrast The contrast factor. >1.0 increases contrast, [0, 1) decreases it.
 * @return The adjusted color. Alpha is unchanged.
 */
SITAPI Color RGL_ColorContrast(Color color, float contrast) {
    contrast = fmaxf(0.0f, contrast); // Contrast cannot be negative
    Color result;
    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;

    r = _RGL_Clamp01(((r - 0.5f) * contrast) + 0.5f);
    g = _RGL_Clamp01(((g - 0.5f) * contrast) + 0.5f);
    b = _RGL_Clamp01(((b - 0.5f) * contrast) + 0.5f);

    result.r = (unsigned char)(r * 255.0f + 0.5f);
    result.g = (unsigned char)(g * 255.0f + 0.5f);
    result.b = (unsigned char)(b * 255.0f + 0.5f);
    result.a = color.a;
    return result;
}

/**
 * @brief Adjusts the saturation of a color.
 * @param color The input color.
 * @param saturation The saturation factor. >1.0 increases saturation, [0, 1) decreases it.
 * @return The adjusted color. Alpha is unchanged.
 */
SITAPI Color RGL_ColorSaturate(Color color, float saturation) {
    vec3 hsv = RGL_ColorToHSV(color);
    hsv[1] *= saturation; // Modify saturation
    Color result = RGL_ColorFromHSV(hsv[0], hsv[1], hsv[2]);
    result.a = color.a;
    return result;
}

/**
 * @brief Desaturates a color completely, turning it into a shade of gray.
 * @param color The input color.
 * @return The grayscale equivalent. Alpha is unchanged.
 */
SITAPI Color RGL_ColorDesaturate(Color color) {
    // Using the standard NTSC luminance calculation for perceptually accurate grayscale
    float luminance = ((float)color.r * 0.299f + (float)color.g * 0.587f + (float)color.b * 0.114f) / 255.0f;
    unsigned char gray = _RGL_ClampToU8((int)(luminance * 255.0f));
    return (Color){ gray, gray, gray, color.a };
}

/**
 * @brief Inverts the RGB channels of a color.
 * @param color The input color.
 * @return The inverted color. Alpha is unchanged.
 */
SITAPI Color RGL_ColorInvert(Color color) {
    return (Color){ 255 - color.r, 255 - color.g, 255 - color.b, color.a };
}

/**
 * @brief Applies gamma correction to a color.
 * @param color The input color.
 * @param gamma The gamma value. Typical values are around 2.2.
 * @return The gamma-corrected color. Alpha is unchanged.
 */
SITAPI Color RGL_ColorGamma(Color color, float gamma) {
    if (gamma <= 0.0f) return color; // Avoid invalid operation
    float inv_gamma = 1.0f / gamma;
    Color result;
    result.r = (unsigned char)(powf((float)color.r / 255.0f, inv_gamma) * 255.0f + 0.5f);
    result.g = (unsigned char)(powf((float)color.g / 255.0f, inv_gamma) * 255.0f + 0.5f);
    result.b = (unsigned char)(powf((float)color.b / 255.0f, inv_gamma) * 255.0f + 0.5f);
    result.a = color.a;
    return result;
}

/**
 * @brief Calculates the perceptual luminance of a color.
 * @param color The input color.
 * @return The luminance value, from 0.0 (black) to 1.0 (white).
 * @note Uses the standard NTSC/PAL luminance formula (Y = 0.299*R + 0.587*G + 0.114*B)
 *       for perceptually-weighted brightness. The alpha channel is ignored.
 */
SITAPI float RGL_ColorLuminance(Color color) {
    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

/**
 * @brief Calculates the Euclidean distance between two colors in RGB space.
 * @param color1 The first color.
 * @param color2 The second color.
 * @return A float representing the distance. A value of 0.0 means the colors are identical.
 *         The maximum possible distance is approximately 441.67 (sqrt(255^2 * 3)).
 * @note The alpha channel is ignored in this calculation. This is not a perceptually
 *       uniform distance (like CIEDE2000), but is fast and effective for most use cases.
 */
SITAPI float RGL_ColorDistance(Color color1, Color color2) {
    int dr = (int)color1.r - (int)color2.r;
    int dg = (int)color1.g - (int)color2.g;
    int db = (int)color1.b - (int)color2.b;
    // Note: We don't need to cast to float before squaring as int is sufficient.
    // Cast the final result to float for the sqrtf function.
    return sqrtf((float)(dr*dr + dg*dg + db*db));
}

/**
 * @brief Checks if two colors are similar within a given tolerance.
 * @param color1 The first color.
 * @param color2 The second color.
 * @param tolerance The maximum allowed color distance (see RGL_ColorDistance).
 * @return True if the distance between the colors is less than or equal to the tolerance.
 * @note The alpha channel is ignored for the comparison.
 */
SITAPI bool RGL_ColorEquals(Color color1, Color color2, float tolerance) {
    // Fast Path for exact matches, including alpha
    if (color1.r == color2.r && color1.g == color2.g && color1.b == color2.b && color1.a == color2.a) {
        return true;
    }
    
    // Check distance if not an exact match
    return RGL_ColorDistance(color1, color2) <= tolerance;
}

/**
 * @brief Finds the color in a palette that is closest to a target color.
 * @param target The color to match.
 * @param palette A pointer to an array of colors.
 * @param palette_size The number of colors in the palette.
 * @return The color from the palette that has the smallest RGL_ColorDistance to the target.
 *         Returns transparent black {0,0,0,0} if the palette is empty or null.
 */
SITAPI Color RGL_ColorClosest(Color target, const Color* palette, int palette_size) {
    if (!palette || palette_size <= 0) {
        // Return a sensible default for an invalid operation
        return (Color){0, 0, 0, 0};
    }
    
    if (palette_size == 1) {
        return palette[0];
    }
    
    float min_dist_sq = FLT_MAX;
    int best_match_index = 0;
    
    for (int i = 0; i < palette_size; i++) {
        int dr = (int)target.r - (int)palette[i].r;
        int dg = (int)target.g - (int)palette[i].g;
        int db = (int)target.b - (int)palette[i].b;
        
        // Using squared distance avoids a costly sqrtf in every loop iteration.
        // This is a standard and effective optimization.
        float dist_sq = (float)(dr*dr + dg*dg + db*db);
        
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            best_match_index = i;
            
            // Optimization: If we found a perfect match, we can stop searching.
            if (min_dist_sq < FLT_EPSILON) {
                break;
            }
        }
    }
    
    return palette[best_match_index];
}

// --- Color Palettes & Schemes ---

/**
 * @brief Samples a color from a palette using linear interpolation between the two nearest colors.
 * @param palette An array of colors.
 * @param palette_size The number of colors in the palette.
 * @param t A normalized value [0.0, 1.0] to sample with.
 * @return The sampled, interpolated color.
 */
SITAPI Color RGL_ColorFromPalette(const Color* palette, int palette_size, float t) {
    if (!palette || palette_size == 0) return (Color){0, 0, 0, 0};
    if (palette_size == 1) return palette[0];

    t = _RGL_Clamp01(t);
    float float_index = t * (float)(palette_size - 1);
    
    int index1 = (int)floorf(float_index);
    int index2 = (int)ceilf(float_index);
    if (index2 >= palette_size) index2 = palette_size - 1; // Clamp index2
    
    float local_t = fmodf(float_index, 1.0f);
    
    return RGL_ColorLerp(palette[index1], palette[index2], local_t);
}

/**
 * @brief Generates a palette by creating a linear gradient between two colors.
 * @param start The starting color of the gradient.
 * @param end The ending color of the gradient.
 * @param out_palette A pre-allocated array to store the generated colors.
 * @param steps The number of colors to generate in the palette.
 */
SITAPI void RGL_GenerateGradientPalette(Color start, Color end, Color* out_palette, int steps) {
    if (!out_palette || steps <= 0) return;
    if (steps == 1) {
        out_palette[0] = start;
        return;
    }
    
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1);
        out_palette[i] = RGL_ColorLerp(start, end, t);
    }
}

/**
 * @brief Generates a vibrant rainbow palette by cycling through the hue in HSV space.
 * @param out_palette A pre-allocated array to store the generated colors.
 * @param steps The number of colors to generate in the palette.
 */
SITAPI void RGL_GenerateRainbowPalette(Color* out_palette, int steps) {
    if (!out_palette || steps <= 0) return;
    
    for (int i = 0; i < steps; i++) {
        float hue = ((float)i / (float)steps) * 360.0f;
        out_palette[i] = RGL_ColorFromHSV(hue, 1.0f, 1.0f); // Full saturation and value
    }
}


/**
 * @brief YPQ interpolation - this is where YPQ really shines!
 * Interpolating in YPQ space gives much more natural color transitions
 */
SITAPI ColorYPQA RGL_YPQLerp(ColorYPQA color1, ColorYPQA color2, float t) {
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    // Linear interpolation for Y (luminance)
    unsigned char y = (unsigned char)(color1.y + (color2.y - color1.y) * t);
    
    // Circular interpolation for P (phase/hue) - handles wraparound properly
    float p1 = color1.p / 255.0f * 2.0f * M_PI;
    float p2 = color2.p / 255.0f * 2.0f * M_PI;
    
    // Find shortest Path around the circle
    float dp = p2 - p1;
    if (dp > M_PI) dp -= 2.0f * M_PI;
    if (dp < -M_PI) dp += 2.0f * M_PI;
    
    float p_interp = p1 + dp * t;
    if (p_interp < 0) p_interp += 2.0f * M_PI;
    if (p_interp >= 2.0f * M_PI) p_interp -= 2.0f * M_PI;
    
    unsigned char p = (unsigned char)((p_interp / (2.0f * M_PI)) * 255.0f + 0.5f);
    
    // Linear interpolation for Q (amplitude/saturation)
    unsigned char q = (unsigned char)(color1.q + (color2.q - color1.q) * t);
    unsigned char a = (unsigned char)(color1.a + (color2.a - color1.a) * t);
    
    return (ColorYPQA){y, p, q, a};
}

/**
 * @brief Adjust luminance while preserving hue and saturation
 */
SITAPI ColorYPQA RGL_YPQAdjustLuminance(ColorYPQA color, float luminance_factor) {
    float new_y = color.y * luminance_factor;
    return (ColorYPQA){
        (unsigned char)fmaxf(0, fminf(255, new_y)),
        color.p, // Phase unchanged
        color.q, // Amplitude unchanged
        color.a
    };
}

/**
 * @brief Rotate hue by shifting phase
 */
SITAPI ColorYPQA RGL_YPQAdjustPhase(ColorYPQA color, int phase_shift) {
    int new_p = (int)color.p + phase_shift;
    while (new_p < 0) new_p += 256;
    while (new_p >= 256) new_p -= 256;
    
    return (ColorYPQA){color.y, (unsigned char)new_p, color.q, color.a};
}

/**
 * @brief Adjust saturation by scaling amplitude
 */
SITAPI ColorYPQA RGL_YPQAdjustQuadrature(ColorYPQA color, float quad_factor) {
    float new_q = color.q * quad_factor;
    return (ColorYPQA){
        color.y,
        color.p, // Phase unchanged
        (unsigned char)fmaxf(0, fminf(255, new_q)),
        color.a
    };
}

/**
 * @brief Multiplies two ColorYPQA colors component-wise (useful for tinting/modulation)
 * Each component is multiplied and normalized back to 0-255 range
 */
SITAPI ColorYPQA RGL_YPQMultiply(ColorYPQA color1, ColorYPQA color2) {
    // Multiply each component as normalized values (0-255 → 0.0-1.0 → multiply → 0-255)
    // This is equivalent to: (a/255) * (b/255) * 255 = (a * b) / 255
    
    return (ColorYPQA){
        (unsigned char)((color1.y * color2.y) / 255),  // Y (luminance multiply)
        (unsigned char)((color1.p * color2.p) / 255),  // P (phase multiply) 
        (unsigned char)((color1.q * color2.q) / 255),  // Q (amplitude multiply)
        (unsigned char)((color1.a * color2.a) / 255)   // A (alpha multiply)
    };
}

/**
 * @brief Generate smooth gradients in YPQ space (much better than RGB!)
 */
SITAPI void RGL_GenerateYPQGradient(ColorYPQA start, ColorYPQA end, Color* out_palette, int steps) {
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1);
        ColorYPQA ypq_color = RGL_YPQLerp(start, end, t);
        out_palette[i] = SituationColorFromYPQ(ypq_color);
    }
}

/**
 * @brief Classic TV-style colors using YPQ
 */
SITAPI ColorYPQA RGL_YPQFromTVChannel(int channel, float signal_strength) {
    // Simulate different TV channels with characteristic colors
    unsigned char base_y = (unsigned char)(200 * signal_strength); // Luminance affected by signal
    unsigned char p = (unsigned char)((channel * 37) % 256); // Pseudo-random hue per channel
    unsigned char q = (unsigned char)(180 * signal_strength); // Saturation affected by signal
    
    return (ColorYPQA){base_y, p, q, 255};
}

/**
 * @brief Samples a color from a YPQ palette using linear interpolation
 * @param ypq_palette Array of ColorYPQA values representing the palette
 * @param palette_size Number of colors in the palette
 * @param t Interpolation parameter (0.0 to 1.0, values outside are clamped)
 * @return RGB Color interpolated from the palette
 */
SITAPI Color SituationColorFromYPQPalette(const ColorYPQA* ypq_palette, int palette_size, float t) {
    if (!ypq_palette || palette_size <= 0) {
        return (Color){0, 0, 0, 255}; // Return black on invalid input
    }
    
    if (palette_size == 1) {
        return SituationColorFromYPQ(ypq_palette[0]); // Single color palette
    }
    
    // Clamp t to [0.0, 1.0] range
    t = fmax(0.0f, fmin(1.0f, t));
    
    // Scale t to palette index range [0, palette_size-1]
    float scaled_t = t * (palette_size - 1);
    
    // Get the two palette indices to interpolate between
    int index0 = (int)scaled_t;                    // Lower index (floor)
    int index1 = index0 + 1;                       // Upper index
    
    // Handle edge case where t = 1.0 exactly
    if (index1 >= palette_size) {
        return SituationColorFromYPQ(ypq_palette[palette_size - 1]);
    }
    
    // Calculate interpolation factor between the two colors
    float local_t = scaled_t - index0;  // Fractional part (0.0 to 1.0)
    
    // Get the two YPQ colors to interpolate between
    ColorYPQA color0 = ypq_palette[index0];
    ColorYPQA color1 = ypq_palette[index1];
    
    // Interpolate each YPQ component linearly
    ColorYPQA interpolated = {
        (unsigned char)(color0.y + (color1.y - color0.y) * local_t + 0.5f),  // Y with rounding
        (unsigned char)(color0.p + (color1.p - color0.p) * local_t + 0.5f),  // P with rounding
        (unsigned char)(color0.q + (color1.q - color0.q) * local_t + 0.5f),  // Q with rounding
        (unsigned char)(color0.a + (color1.a - color0.a) * local_t + 0.5f)   // A with rounding
    };
    
    // Convert interpolated YPQ to RGB
    return SituationColorFromYPQ(interpolated);
}

/**
 * @brief Scanline effect that dims every other line
 */
SITAPI Color RGL_ColorScanline(ColorYPQA color, float scanline_y, float intensity) {
    int line = (int)scanline_y;
    if (line % 2 == 1) {
        // Dim odd scanlines
        ColorYPQA dimmed = RGL_YPQAdjustLuminance(color, 1.0f - intensity);
        return SituationColorFromYPQ(dimmed);
    }
    return SituationColorFromYPQ(color);
}

/**
 * @brief TV noise effect
 */
SITAPI Color RGL_ColorTVNoise(ColorYPQA base_color, float noise_strength, vec2 screen_pos) {
    // Simple noise based on screen position
    float noise = sinf(screen_pos[0] * 0.1f) * cosf(screen_pos[1] * 0.1f);
    noise = (noise + 1.0f) * 0.5f; // Normalize to 0-1
    
    // Apply noise to luminance
    ColorYPQA noisy = base_color;
    float y_noise = noise * noise_strength * 50.0f;
    noisy.y = (unsigned char)fmaxf(0, fminf(255, noisy.y + y_noise));
    
    return SituationColorFromYPQ(noisy);
}

/**
 * @brief CRT phosphor bloom effect in YPQ space
 */
SITAPI Color RGL_ColorCRTBloom(ColorYPQA color, float bloom_strength) {
    // Increase luminance and slightly reduce saturation for bloom
    ColorYPQA bloomed = RGL_YPQAdjustLuminance(color, 1.0f + bloom_strength);
    bloomed = RGL_YPQAdjustQuadrature(bloomed, 1.0f - bloom_strength * 0.3f);
    
    return SituationColorFromYPQ(bloomed);
}

/**
 * @brief Creates a TV ghosting effect by blending the original color with a phase-shifted version
 * This simulates analog TV signal reflections that cause "ghost" images
 * @param color Original YPQ color
 * @param ghost_offset Phase offset for the ghost (0.0-1.0, represents fraction of full phase cycle)
 * @param ghost_strength Strength of ghosting effect (0.0 = no ghost, 1.0 = full ghost)
 * @return RGB Color with ghosting effect applied
 */
SITAPI Color RGL_ColorTVGhost(ColorYPQA color, float ghost_offset, float ghost_strength) {
    // Clamp parameters to valid ranges
    ghost_offset = fmodf(ghost_offset, 1.0f);  // Wrap to [0.0, 1.0)
    if (ghost_offset < 0.0f) ghost_offset += 1.0f;  // Handle negative values
    ghost_strength = fmax(0.0f, fmin(1.0f, ghost_strength));  // Clamp to [0.0, 1.0]
    
    // Create the ghost color by shifting the phase (P component)
    ColorYPQA ghost_color = color;
    
    // Shift the phase by the ghost offset
    // Convert P to 0.0-1.0 range, add offset, wrap around, convert back to 0-255
    float phase_normalized = (float)color.p / 255.0f;
    phase_normalized += ghost_offset;
    phase_normalized = fmodf(phase_normalized, 1.0f);  // Wrap around
    if (phase_normalized < 0.0f) phase_normalized += 1.0f;
    ghost_color.p = (unsigned char)(phase_normalized * 255.0f + 0.5f);
    
    // Reduce the ghost's luminance and saturation for more realistic effect
    ghost_color.y = (unsigned char)(ghost_color.y * 0.7f + 0.5f);      // Dimmer ghost
    ghost_color.q = (unsigned char)(ghost_color.q * 0.8f + 0.5f);      // Less saturated ghost
    
    // Convert both original and ghost colors to RGB
    Color original_rgb = SituationColorFromYPQ(color);
    Color ghost_rgb = SituationColorFromYPQ(ghost_color);
    
    // Blend the original and ghost colors
    // Use additive blending weighted by ghost_strength
    float inv_strength = 1.0f - ghost_strength;
    unsigned char final_r = (unsigned char)fmin(255.0f, original_rgb.r * inv_strength + ghost_rgb.r * ghost_strength + 0.5f);
    unsigned char final_g = (unsigned char)fmin(255.0f, original_rgb.g * inv_strength + ghost_rgb.g * ghost_strength + 0.5f);
    unsigned char final_b = (unsigned char)fmin(255.0f, original_rgb.b * inv_strength + ghost_rgb.b * ghost_strength + 0.5f);
    return (Color){final_r, final_g, final_b, original_rgb.a};
}

/**
 * @brief Checks if two YPQ colors are equal within a given tolerance
 * @param color1 First YPQ color
 * @param color2 Second YPQ color  
 * @param tolerance Maximum difference allowed for each component (0-255)
 * @return true if colors are within tolerance, false otherwise
 */
SITAPI bool RGL_YPQEquals(ColorYPQA color1, ColorYPQA color2, unsigned char tolerance) {
    // Check each component individually using absolute difference
    int diff_y = abs((int)color1.y - (int)color2.y);
    int diff_p = abs((int)color1.p - (int)color2.p);
    int diff_q = abs((int)color1.q - (int)color2.q);
    int diff_a = abs((int)color1.a - (int)color2.a);
    
    // All components must be within tolerance
    return (diff_y <= tolerance && 
            diff_p <= tolerance && 
            diff_q <= tolerance && 
            diff_a <= tolerance);
}

/**
 * @brief Finds the closest color in a palette to a target YPQ color
 * Uses Euclidean distance in YPQ space for comparison
 * @param target Target YPQ color to match
 * @param palette Array of ColorYPQA values to search
 * @param palette_size Number of colors in the palette
 * @return Closest ColorYPQA from the palette, or black if palette is invalid
 */
SITAPI ColorYPQA RGL_YPQClosest(ColorYPQA target, const ColorYPQA* palette, int palette_size) {
    if (!palette || palette_size <= 0) {
        return (ColorYPQA){0, 0, 0, 255}; // Return black on invalid input
    }
    
    ColorYPQA closest = palette[0];  // Start with first color as closest
    float min_distance_sq = FLT_MAX;  // Path minimum squared distance
    
    for (int i = 0; i < palette_size; i++) {
        ColorYPQA current = palette[i];
        
        // Calculate squared Euclidean distance in YPQ space
        // We use squared distance to avoid expensive sqrt() calls
        float dy = (float)((int)target.y - (int)current.y);
        float dp = (float)((int)target.p - (int)current.p);
        float dq = (float)((int)target.q - (int)current.q);
        float da = (float)((int)target.a - (int)current.a);
        
        // Handle phase wraparound for P component (since it's circular 0-255 → 0-2π)
        // Find the shorter angular distance
        float dp_wrapped = fmin(fabs(dp), 255.0f - fabs(dp));
        
        // Calculate weighted distance (you may want to adjust these weights)
        // Y gets higher weight since luminance is most perceptually important
        // P gets lower weight since hue differences are less critical than brightness
        float distance_sq = (dy * dy * 2.0f) +     // Y: double weight (luminance most important)
                           (dp_wrapped * dp_wrapped * 0.5f) + // P: half weight (hue less critical)
                           (dq * dq * 1.0f) +      // Q: normal weight (saturation)
                           (da * da * 1.0f);       // A: normal weight (alpha)
        
        if (distance_sq < min_distance_sq) {
            min_distance_sq = distance_sq;
            closest = current;
        }
    }
    
    return closest;
}

/**
 * @brief Draws a filled, convex polygon in world-space on a specific Z-plane, with lighting support.
 *
 * This function triangulates a convex polygon using a triangle fan and adds the
 * resulting triangles to the main render batch. It is compatible with the new lighting
 * system, providing a default normal suitable for 2D/UI elements.
 *
 * @param points An array of 2D points defining the polygon vertices in the world-space XY plane.
 * @param point_count The number of points in the array. Must be 3 or more.
 * @param z_depth The Z-coordinate for the entire polygon, used for depth sorting.
 * @param color The solid color to fill the polygon with.
 * @note This function is intended for convex polygons only. For non-convex shapes,
 *       use a more advanced triangulation method and RGL_DrawTriangle3D.
 */
SITAPI void RGL_DrawPolygon(vec2* points, int point_count, float z_depth, Color color) {
    // 1. --- Pre-flight Checks ---
    if (!RGL.is_batching || !points || point_count < 3) return;

    // 2. --- CRITICAL: Capacity Check (The Missing Security Scheme) ---
    // A polygon with N points creates N-2 triangles. We must ensure there is space.
    size_t triangles_to_create = point_count - 2;
    if (!_RGL_EnsureCommandCapacity(triangles_to_create)) {
        // If we can't make enough space even after a flush, we cannot draw this polygon.
        return;
    }

    // 3. --- Prepare Shared Data ---
    vec4 norm_color;
    SituationConvertColorToVec4(color, norm_color);
    const vec3 normal = {0.0f, 0.0f, 1.0f}; // Standard normal for flat 2D shapes.

    // 4. --- Triangulation and Batching Loop ---
    // Use a triangle fan approach to create individual triangles.
    for (int i = 0; i < triangles_to_create; i++) {
        // Use the safe "populate-then-commit" pattern.
        RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];
        
        // --- Populate the Command ---
        cmd->is_triangle = true;
        cmd->texture.id = 0; // Untextured
        cmd->z_depth = z_depth;

        // Define the 3 vertices for this triangle in the fan
        vec3 vertices[3] = {
            { points[0][0],     points[0][1],     z_depth },
            { points[i + 1][0], points[i + 1][1], z_depth },
            { points[i + 2][0], points[i + 2][1], z_depth }
        };
        memcpy(cmd->world_positions, vertices, sizeof(vec3) * 3);

        // Populate all vertex attributes for the new pipeline
        for(int v = 0; v < 3; v++) {
            glm_vec2_zero(cmd->tex_coords[v]);
            glm_vec4_copy(norm_color, cmd->colors[v]);
            glm_vec3_copy(normal, cmd->normals[v]); // Add the normal
            cmd->light_levels[v] = 1.0f;            // Fully bright
        }

        // --- Commit the Command ---
        RGL.command_count++;
    }
}

/**
 * @brief Draws a filled, convex polygon in screen-space for UI rendering.
 *
 * This is a convenience wrapper for RGL_DrawPolygon. It assumes you want to draw
 * directly in pixel coordinates and draws on top of everything (Z=0).
 *
 * @param points Array of 2D points in screen-space (pixels).
 * @param point_count Number of points (must be >= 3).
 * @param color Solid color to fill the polygon.
 * @note For this to work as expected, you MUST set a 2D orthographic camera first, e.g.,
 *       `RGL_SetCamera2D((vec2){w/2, h/2}, 0, 1.0f);` before drawing your UI.
 */
SITAPI void RGL_DrawPolygonScreen(vec2* points, int point_count, Color color) {
    // This is now a simple, clean wrapper. It submits the screen-space points as
    // world-space points with a Z-depth of 0. The active orthographic camera
    // will then render them 1:1 to the screen.
    RGL_DrawPolygon(points, point_count, 0.0f, color);
}

/**
 * @brief Draws a sprite in 3D space that always perfectly faces the camera (Spherical Billboard).
 * This is the most common and robust type of billboard. It is ideal for particles, effects, and items.
 *
 * @brief Draws a sprite that always faces the camera, lit by the ground surface normal.
 * @param sprite The sprite to draw.
 * @param world_pos 3D world-space position of the sprite’s center.
 * @param size The width and height of the billboard in world units.
 * @param tint Color to tint the sprite.
 */
SITAPI void RGL_DrawBillboard(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint) {
    if (!RGL.is_batching) return;
    if (!_RGL_EnsureCommandCapacity(1)) return;

    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count++];
    cmd->texture = sprite.texture;
    cmd->z_depth = world_pos[2];
    cmd->is_triangle = false;

    // --- Calculate billboard vertices to face the camera ---
    mat4 view;
    RGL_GetViewMatrix(view);
    vec3 cam_right = { view[0][0], view[1][0], view[2][0] };
    vec3 cam_up    = { view[0][1], view[1][1], view[2][1] };

    vec3 half_right, half_up;
    glm_vec3_scale(cam_right, size[0] * 0.5f, half_right);
    glm_vec3_scale(cam_up,    size[1] * 0.5f, half_up);

    // Order: TL, BL, BR, TR for RGL_DrawSpritePro compatibility
    glm_vec3_sub(world_pos, half_right, cmd->world_positions[0]); glm_vec3_add(cmd->world_positions[0], half_up, cmd->world_positions[0]);
    glm_vec3_sub(world_pos, half_right, cmd->world_positions[1]); glm_vec3_sub(cmd->world_positions[1], half_up, cmd->world_positions[1]);
    glm_vec3_add(world_pos, half_right, cmd->world_positions[2]); glm_vec3_sub(cmd->world_positions[2], half_up, cmd->world_positions[2]);
    glm_vec3_add(world_pos, half_right, cmd->world_positions[3]); glm_vec3_add(cmd->world_positions[3], half_up, cmd->world_positions[3]);

    // --- NEW: Normal Calculation ---
    vec3 normal;
    RGLGroundInfo ground_info;
    if (RGL_GetGroundAt((vec2){world_pos[0], world_pos[2]}, &ground_info) && ground_info.is_hit) {
        glm_vec3_copy(ground_info.surface_normal, normal);
    } else {
        glm_vec3_sub(RGL.camera_position, world_pos, normal);
        glm_vec3_normalize(normal);
    }

    // --- Populate the rest of the command ---
    float u1 = sprite.source_rect.x / sprite.texture.width;
    float v1 = sprite.source_rect.y / sprite.texture.height;
    float u2 = u1 + sprite.source_rect.width / sprite.texture.width;
    float v2 = v1 + sprite.source_rect.height / sprite.texture.height;
    cmd->tex_coords[0][0]=u1; cmd->tex_coords[0][1]=v1; // TL
    cmd->tex_coords[1][0]=u1; cmd->tex_coords[1][1]=v2; // BL
    cmd->tex_coords[2][0]=u2; cmd->tex_coords[2][1]=v2; // BR
    cmd->tex_coords[3][0]=u2; cmd->tex_coords[3][1]=v1; // TR

    vec4 tint_v4;
    SituationConvertColorToVec4(tint, tint_v4);
    for(int i = 0; i < 4; i++) {
        glm_vec4_copy(tint_v4, cmd->colors[i]);
        glm_vec3_copy(normal, cmd->normals[i]);
        cmd->light_levels[i] = 1.0f;
    }
}

/**
 * @brief Draws a sprite that stays upright but pivots on the world Y-axis to face the camera (Cylindrical Billboard).
 * This is ideal for grounded objects like trees or characters that should not tilt.
 *
 * @param sprite The sprite to draw.
 * @param world_pos 3D world-space position of the sprite’s center.
 * @param size The width and height of the billboard in world units.
 * @param tint Color to tint the sprite.
 */
SITAPI void RGL_DrawBillboardCylindricalY(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint) {
    if (!RGL.is_initialized || !RGL.is_batching) return;
    if (RGL.command_count >= RGL.command_capacity) _RGL_FlushBatch();

    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];
    cmd->texture = sprite.texture;
    cmd->z_depth = world_pos[2];
    cmd->is_triangle = false;

    // --- Billboard Calculation with Gimbal Lock fix ---
    vec3 direction_to_cam;
    glm_vec3_sub(RGL.camera_position, world_pos, direction_to_cam);
    
    // We only care about the direction in the XZ plane for rotation
    direction_to_cam[1] = 0;
    
    // GIMBAL LOCK FIX: If the camera is directly above/below, the direction is zero.
    // In this case, we can just use the camera's X-axis as our "right" vector.
    if (glm_vec3_norm2(direction_to_cam) < 0.001f) {
        mat4 view;
        RGL_GetViewMatrix(view);
        direction_to_cam[0] = view[0][0]; // Camera's right vector X
        direction_to_cam[2] = view[2][0]; // Camera's right vector Z
    }
    glm_vec3_normalize(direction_to_cam);

    vec3 world_up = {0.0f, 1.0f, 0.0f};
    vec3 billboard_right;
    glm_vec3_cross(world_up, direction_to_cam, billboard_right);
    glm_vec3_normalize(billboard_right); // The billboard's final "right" axis

    vec3 scaled_right, scaled_up;
    glm_vec3_scale(billboard_right, size[0] * 0.5f, scaled_right);
    glm_vec3_scale(world_up,        size[1] * 0.5f, scaled_up);

    // Calculate the 4 world-space corners
    glm_vec3_sub(world_pos, scaled_right, cmd->world_positions[0]); glm_vec3_add(cmd->world_positions[0], scaled_up, cmd->world_positions[0]); // TL
    glm_vec3_sub(world_pos, scaled_right, cmd->world_positions[1]); glm_vec3_sub(cmd->world_positions[1], scaled_up, cmd->world_positions[1]); // BL
    glm_vec3_add(world_pos, scaled_right, cmd->world_positions[2]); glm_vec3_sub(cmd->world_positions[2], scaled_up, cmd->world_positions[2]); // BR
    glm_vec3_add(world_pos, scaled_right, cmd->world_positions[3]); glm_vec3_add(cmd->world_positions[3], scaled_up, cmd->world_positions[3]); // TR
    
    // Set texture coordinates and colors
    float u1 = sprite.source_rect.x / sprite.texture.width;
    float v1 = sprite.source_rect.y / sprite.texture.height;
    float u2 = u1 + sprite.source_rect.width / sprite.texture.width;
    float v2 = v1 + sprite.source_rect.height / sprite.texture.height;
    cmd->tex_coords[0][0]=u1; cmd->tex_coords[0][1]=v1; cmd->tex_coords[1][0]=u1; cmd->tex_coords[1][1]=v2;
    cmd->tex_coords[2][0]=u2; cmd->tex_coords[2][1]=v2; cmd->tex_coords[3][0]=u2; cmd->tex_coords[3][1]=v1;

    vec4 tint_v4;
    SituationConvertColorToVec4(tint, tint_v4);
    for(int i=0; i<4; i++) {
        glm_vec4_copy(tint_v4, cmd->colors[i]);
        cmd->light_levels[i] = 1.0f;
    }
    
    RGL.command_count++;
}

SITAPI void RGL_CastStencilShadowFromMesh(RGLMesh mesh, mat4 transform, const RGLShadowConfig* config) {
    // 1. --- SANITY CHECKS ---
    if (!RGL.is_batching || !config || mesh.id == 0 || !mesh.cpu_vertices || !mesh.cpu_indices) {
        return;
    }

    // 2. --- FIND THE LIGHT SOURCE ---
    int light_id = config->light_id;
    if (light_id <= 0 || light_id > RGL_MAX_LIGHTS || !RGL.lights[light_id - 1].is_active || RGL.lights[light_id - 1].type == RGL_LIGHT_TYPE_DIRECTIONAL) {
        return;
    }
    RGLLight* light = &RGL.lights[light_id - 1];

    _RGL_FlushBatch();

    // 3. --- SET UP STENCIL STATE ---
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_CULL_FACE);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glClear(GL_STENCIL_BUFFER_BIT);

    // 4. --- GENERATE SHADOW GEOMETRY (The Brute-Force Method) ---
    vec3* transformed_verts = (vec3*)malloc(mesh.vertex_count * sizeof(vec3));
    if (!transformed_verts) { SIT_Log(SIT_ERROR, "Failed to allocate memory for transformed shadow verts."); return; }
    
    for (int i = 0; i < mesh.vertex_count; i++) {
        glm_mat4_mulv3(transform, mesh.vertices[i], 1.0f, transformed_verts[i]);
    }

    // Allocate enough space for the sides of ALL triangles (worst-case scenario for non-closed meshes)
    vec3* volume_verts = (vec3*)malloc(mesh.index_count * 2 * sizeof(vec3));
    if (!volume_verts) { free(transformed_verts); SIT_Log(SIT_ERROR, "Failed to allocate memory for shadow volume."); return; }
    int volume_vert_count = 0;

    // Iterate through all triangles of the mesh
    for (int i = 0; i < mesh.index_count; i += 3) {
        vec3 v0 = transformed_verts[mesh.indices[i]];
        vec3 v1 = transformed_verts[mesh.indices[i+1]];
        vec3 v2 = transformed_verts[mesh.indices[i+2]];

        // Check if the triangle is front-facing with respect to the light
        vec3 normal;
        glm_vec3_cross(glm_vec3_sub(v1, v0, (vec3){0}), glm_vec3_sub(v2, v0, (vec3){0}), normal);
        
        if (glm_vec3_dot(normal, glm_vec3_sub(v0, light->position, (vec3){0})) > 0) {
            // If it is, extrude its three edges to form the side walls of the volume
            vec3 ev0, ev1, ev2;
            glm_vec3_add(v0, glm_vec3_scale(glm_vec3_normalize(glm_vec3_sub(v0, light->position, (vec3){0})), config->extrusion_length, (vec3){0}), ev0);
            glm_vec3_add(v1, glm_vec3_scale(glm_vec3_normalize(glm_vec3_sub(v1, light->position, (vec3){0})), config->extrusion_length, (vec3){0}), ev1);
            glm_vec3_add(v2, glm_vec3_scale(glm_vec3_normalize(glm_vec3_sub(v2, light->position, (vec3){0})), config->extrusion_length, (vec3){0}), ev2);
            
            // Add the three side quads (as 6 triangles)
            glm_vec3_copy(v0, volume_verts[volume_vert_count++]); glm_vec3_copy(v1, volume_verts[volume_vert_count++]); glm_vec3_copy(ev0, volume_verts[volume_vert_count++]);
            glm_vec3_copy(ev0, volume_verts[volume_vert_count++]); glm_vec3_copy(v1, volume_verts[volume_vert_count++]); glm_vec3_copy(ev1, volume_verts[volume_vert_count++]);

            glm_vec3_copy(v1, volume_verts[volume_vert_count++]); glm_vec3_copy(v2, volume_verts[volume_vert_count++]); glm_vec3_copy(ev1, volume_verts[volume_vert_count++]);
            glm_vec3_copy(ev1, volume_verts[volume_vert_count++]); glm_vec3_copy(v2, volume_verts[volume_vert_count++]); glm_vec3_copy(ev2, volume_verts[volume_vert_count++]);

            glm_vec3_copy(v2, volume_verts[volume_vert_count++]); glm_vec3_copy(v0, volume_verts[volume_vert_count++]); glm_vec3_copy(ev2, volume_verts[volume_vert_count++]);
            glm_vec3_copy(ev2, volume_verts[volume_vert_count++]); glm_vec3_copy(v0, volume_verts[volume_vert_count++]); glm_vec3_copy(ev0, volume_verts[volume_vert_count++]);
        }
    }
    
    free(transformed_verts);

    // 5. --- RENDER THE VOLUME ---
    if (volume_vert_count > 0) {
        glUseProgram(RGL.shadow_volume_shader.gl_program_id);
        glUniformMatrix4fv(RGL.loc_sv_view, 1, GL_FALSE, &RGL.current_view_matrix[0][0]);
        glUniformMatrix4fv(RGL.loc_sv_projection, 1, GL_FALSE, &RGL.current_projection_matrix[0][0]);

        glBindVertexArray(RGL.batch_vao);
        glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * volume_vert_count, volume_verts, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLES, 0, volume_vert_count);
    }
    free(volume_verts);

    // 6. --- DARKEN SCENE and CLEANUP (same as before) ---
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);

    glUseProgram(RGL.shadow_darken_shader.gl_program_id);
    glUniform4f(RGL.loc_sd_shadow_color, config->color.r/255.f, config->color.g/255.f, config->color.b/255.f, config->color.a/255.f);
    
    glBindVertexArray(RGL.fullscreen_quad_vao);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glUseProgram(RGL.main_shader.gl_program_id);
}

/**
 * @brief [INDOOR / QUALITY] Draws a perspectively correct shadow from a point/spot light using stencil volumes.
 *
 * This is the highest quality, most realistic shadow effect for indoor, point-lit environments.
 * The shadow will stretch, skew, and wrap correctly around all level geometry (floors, walls, etc.).
 * This is the most performance-intensive shadow function and should be used selectively on
 * important characters or dynamic objects that need to interact realistically with dynamic lights.
 *
 * @param sprite The sprite to use for the shadow's shape.
 * @param world_pos The 3D world position of the object casting the shadow.
 * @param size The width and height of the sprite object in world units.
 * @param config A struct specifying the light source ID, shadow color, and other properties.
 */
SITAPI void RGL_DrawSpriteWithShadow(RGLSprite sprite, vec3 world_pos, vec2 size, const RGLShadowConfig* config) {
    if (!RGL.is_batching || !config || sprite.texture.id == 0) return;

    // --- Build the mesh data for a camera-facing quad ---
    vec3 caster_verts[4];
    vec3 right, up;

    // Get camera-aligned right/up vectors
    right[0] = RGL.current_view_matrix[0][0]; right[1] = RGL.current_view_matrix[1][0]; right[2] = RGL.current_view_matrix[2][0];
    up[0]    = RGL.current_view_matrix[0][1]; up[1]    = RGL.current_view_matrix[1][1]; up[2]    = RGL.current_view_matrix[2][1];
    glm_vec3_normalize(right);
    glm_vec3_normalize(up);
    glm_vec3_scale(right, size[0] * 0.5f, right);
    glm_vec3_scale(up, size[1] * 0.5f, up);

    // Calculate the 4 corners of the quad
    vec3 temp;
    glm_vec3_sub(world_pos, right, temp); glm_vec3_sub(temp, up, caster_verts[0]); // bl
    glm_vec3_add(world_pos, right, temp); glm_vec3_sub(temp, up, caster_verts[1]); // br
    glm_vec3_add(world_pos, right, temp); glm_vec3_add(temp, up, caster_verts[2]); // tr
    glm_vec3_sub(world_pos, right, temp); glm_vec3_add(temp, up, caster_verts[3]); // tl

    // Define the quad's triangles (tl, bl, br) and (tl, br, tr)
    unsigned int indices[] = { 3, 0, 1, 3, 1, 2 };
    
    RGLMesh quad_mesh = {
        .id = -1, // Mark as temporary
        .cpu_vertices = caster_verts,
        .cpu_indices = indices,
        .vertex_count = 4,
        .index_count = 6
    };
    
    // The quad's vertices are already in world space, so we pass an identity transform.
    mat4 identity_transform;
    glm_mat4_identity(identity_transform);

    // --- Call the core mesh-based shadow caster ---
    RGL_CastStencilShadowFromMesh(quad_mesh, identity_transform, config);
}

/**
 * @brief [INDOOR / QUALITY / HELPER] A simplified helper for RGL_DrawSpriteWithShadow.
 *
 * This is a convenience wrapper around the more complex RGL_DrawSpriteWithShadow function.
 * It uses default shadow parameters (black, semi-transparent) and casts a perspectively
 * correct stencil volume shadow from the specified light source.
 *
 * @param sprite The sprite to use for the shadow's shape.
 * @param world_pos The 3D world position of the object casting the shadow.
 * @param size The width and height of the sprite object in world units.
 * @param light_id The ID of the single RGL_LIGHT_TYPE_POINT or RGL_LIGHT_TYPE_SPOT light to cast from.
 */
SITAPI void RGL_DrawSpriteWithSimpleShadow(RGLSprite sprite, vec3 world_pos, vec2 size, int light_id) {
    RGLShadowConfig config = {
        .color = {0, 0, 0, 128},
        .extrusion_length = 1000.0f, // Should be related to light radius
        .light_id = light_id
    };
    RGL_DrawSpriteWithShadow(sprite, world_pos, size, &config);
}

/**
 * @brief [OUTDOOR / FAST] Draws a simple, fast downward-projected "blob" shadow.
 *
 * Use this for outdoor scenes with a directional sun light. It is extremely high-performance.
 * The shadow is projected vertically onto the ground (Path/terrain) and will fade as the
 * caster's height increases. It does NOT depend on point lights and will NOT wrap onto walls.
 *
 * @param sprite The sprite to use as the shadow's shape.
 * @param world_pos The 3D world position of the object casting the shadow.
 * @param size The width and height of the shadow in world units.
 * @param shadow_tint The color and transparency of the shadow.
 */
SITAPI void RGL_DrawSpriteDownwardShadow(RGLSprite sprite, vec3 world_pos, vec2 size, Color shadow_tint) {
    if (!RGL.is_batching || sprite.texture.id == 0) return;

    RGLGroundInfo ground;
    vec2 pos_xz = { world_pos[0], world_pos[2] };
    if (!RGL_GetGroundAt(pos_xz, &ground) || !ground.is_hit) {
        return;
    }

    float height_off_ground = world_pos[1] - ground.ground_y;
    if (height_off_ground < 0) height_off_ground = 0; // Don't get darker if clipping through floor

    // Define a max height where the shadow is fully faded.
    const float max_shadow_height = 20.0f; // Tweak this value for your game's scale.
    float shadow_alpha_mod = 1.0f - RGL_Clamp(height_off_ground / max_shadow_height, 0.0f, 1.0f);
    
    // Smooth the falloff for a nicer look
    shadow_alpha_mod = shadow_alpha_mod * shadow_alpha_mod; 

    // If shadow is completely faded, just exit. This is an optimization.
    if (shadow_alpha_mod <= 0.01f) {
        return;
    }

    Color final_shadow_tint = shadow_tint;
    final_shadow_tint.a = (unsigned char)(shadow_tint.a * shadow_alpha_mod);


    // --- Step 2: Flush the batch to switch shaders and state ---
    // This is necessary because the shadow needs a different shader and blend mode.
    _RGL_FlushBatch();

    // --- Step 3: Set up Render State for Transparent Shadow ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer to avoid z-fighting with the ground it's on.
                           // The depth TEST is still on, so it won't draw through hills.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f); // Push shadow slightly away from the camera

    // --- Step 4: Use the dedicated shadow shader ---
    glUseProgram(RGL.shadow_shader.gl_program_id);
    glUniformMatrix4fv(RGL.loc_shadow_view, 1, GL_FALSE, &RGL.current_view_matrix[0][0]);
    glUniformMatrix4fv(RGL.loc_shadow_projection, 1, GL_FALSE, &RGL.current_projection_matrix[0][0]);
    glUniform1i(RGL.loc_shadow_texture, 0);
    glUniform4f(RGL.loc_shadow_tint, shadow_tint.r/255.f, shadow_tint.g/255.f, shadow_tint.b/255.f, shadow_tint.a/255.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite.texture.id);

    // --- Step 5: Define the shadow quad on the ground ---
    float w = size[0] * 0.5f;
    float h = size[1] * 0.5f; // This is size along the world Z-axis
    
    // We need to orient the quad along the ground's normal to prevent it from clipping through slopes.
    vec3 center = { world_pos[0], ground.ground_y + 0.02f, world_pos[2] }; // Center of the shadow, slightly offset
    vec3 up_vec = { ground.surface_normal[0], ground.surface_normal[1], ground.surface_normal[2] };
    
    // Create a 'right' vector that is perpendicular to the ground normal and the world forward vector (0,0,-1)
    vec3 forward_vec = {0, 0, -1};
    vec3 right_vec;
    glm_vec3_cross(forward_vec, up_vec, right_vec);
    glm_vec3_normalize(right_vec);
    
    // Now create a 'forward' vector for the quad that is perpendicular to the new right and up vectors
    vec3 quad_forward_vec;
    glm_vec3_cross(up_vec, right_vec, quad_forward_vec);

    // Scale them by the shadow size
    glm_vec3_scale(right_vec, w, right_vec);
    glm_vec3_scale(quad_forward_vec, h, quad_forward_vec);

    // Calculate the 4 corners of the quad
    vec3 p1, p2, p3, p4; // bl, br, tr, tl
    glm_vec3_sub(center, right_vec, p1); glm_vec3_sub(p1, quad_forward_vec, p1);
    glm_vec3_add(center, right_vec, p2); glm_vec3_sub(p2, quad_forward_vec, p2);
    glm_vec3_add(center, right_vec, p3); glm_vec3_add(p3, quad_forward_vec, p3);
    glm_vec3_sub(center, right_vec, p4); glm_vec3_add(p4, quad_forward_vec, p4);

    // --- Step 6: Manually submit the quad for immediate drawing ---
    Rectangle src = sprite.source_rect;
    float u1 = src.x / sprite.texture.width;
    float v1 = src.y / sprite.texture.height;
    float u2 = (src.x + src.width) / sprite.texture.width;
    float v2 = (src.y + src.height) / sprite.texture.height;

    float vertices[] = {
        // Position         // TexCoords
        p1[0], p1[1], p1[2],  u1, v2, // Bottom-left
        p2[0], p2[1], p2[2],  u2, v2, // Bottom-right
        p3[0], p3[1], p3[2],  u2, v1, // Top-right
        p4[0], p4[1], p4[2],  u1, v1  // Top-left
    };
    
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    // This part can be optimized by adding to a temporary batch instead of glBufferData every time.
    // But for simplicity, this immediate-mode style is fine.
    glBindVertexArray(RGL.batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    // We only need position and texcoord for the shadow shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    
    // Create and use an index buffer for this single draw
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &ibo); // Clean up

    // --- Step 7: Restore state ---
    _RGL_FlushBatch(); // Clear out any remaining state changes
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUseProgram(RGL.main_shader.gl_program_id);
    // Re-enable all attributes for the main shader
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3); glEnableVertexAttribArray(4);
}

SITAPI void RGL_GetViewMatrix(mat4 out_view) {
    if (!RGL.is_initialized) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized");
        glm_mat4_identity(out_view);
        return;
    }
    glm_mat4_copy(RGL.current_view_matrix, out_view);
}

SITAPI void RGL_GetProjectionMatrix(mat4 out_proj) {
    if (!RGL.is_initialized) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_INITIALIZED, "RGL not initialized");
        glm_mat4_identity(out_proj);
        return;
    }
    glm_mat4_copy(RGL.current_projection_matrix, out_proj);
}

// --- Set 3D transform matrix ---
SITAPI void RGL_SetTransform(mat4 transform) {
    glm_mat4_copy(transform, RGL.transform);
    RGL.use_transform = true;
}

// --- Reset to identity transform ---
SITAPI void RGL_ResetTransform(void) {
    glm_mat4_identity(RGL.transform);
    RGL.use_transform = false;
}

/**
 * @brief Extracts the six planes of the view frustum from a view-projection matrix.
 * @param vp_matrix The combined (projection * view) matrix.
 * @param out_frustum_planes An array of 6 vec4s to store the plane equations [A, B, C, D].
 */
static void _RGL_ExtractFrustumPlanes(const mat4 vp_matrix, vec4 out_frustum_planes[6]) {
    // Left Plane
    out_frustum_planes[0][0] = vp_matrix[0][3] + vp_matrix[0][0];
    out_frustum_planes[0][1] = vp_matrix[1][3] + vp_matrix[1][0];
    out_frustum_planes[0][2] = vp_matrix[2][3] + vp_matrix[2][0];
    out_frustum_planes[0][3] = vp_matrix[3][3] + vp_matrix[3][0];
    // Right Plane
    out_frustum_planes[1][0] = vp_matrix[0][3] - vp_matrix[0][0];
    out_frustum_planes[1][1] = vp_matrix[1][3] - vp_matrix[1][0];
    out_frustum_planes[1][2] = vp_matrix[2][3] - vp_matrix[2][0];
    out_frustum_planes[1][3] = vp_matrix[3][3] - vp_matrix[3][0];
    // Bottom Plane
    out_frustum_planes[2][0] = vp_matrix[0][3] + vp_matrix[0][1];
    out_frustum_planes[2][1] = vp_matrix[1][3] + vp_matrix[1][1];
    out_frustum_planes[2][2] = vp_matrix[2][3] + vp_matrix[2][1];
    out_frustum_planes[2][3] = vp_matrix[3][3] + vp_matrix[3][1];
    // Top Plane
    out_frustum_planes[3][0] = vp_matrix[0][3] - vp_matrix[0][1];
    out_frustum_planes[3][1] = vp_matrix[1][3] - vp_matrix[1][1];
    out_frustum_planes[3][2] = vp_matrix[2][3] - vp_matrix[2][1];
    out_frustum_planes[3][3] = vp_matrix[3][3] - vp_matrix[3][1];
    // Near Plane
    out_frustum_planes[4][0] = vp_matrix[0][3] + vp_matrix[0][2];
    out_frustum_planes[4][1] = vp_matrix[1][3] + vp_matrix[1][2];
    out_frustum_planes[4][2] = vp_matrix[2][3] + vp_matrix[2][2];
    out_frustum_planes[4][3] = vp_matrix[3][3] + vp_matrix[3][2];
    // Far Plane
    out_frustum_planes[5][0] = vp_matrix[0][3] - vp_matrix[0][2];
    out_frustum_planes[5][1] = vp_matrix[1][3] - vp_matrix[1][2];
    out_frustum_planes[5][2] = vp_matrix[2][3] - vp_matrix[2][2];
    out_frustum_planes[5][3] = vp_matrix[3][3] - vp_matrix[3][2];

    // Normalize the plane equations
    for (int i = 0; i < 6; i++) {
        float mag = glm_vec3_norm((vec3){out_frustum_planes[i][0], out_frustum_planes[i][1], out_frustum_planes[i][2]});
        if (mag > 0.0001f) {
            glm_vec4_scale(out_frustum_planes[i], 1.0f / mag, out_frustum_planes[i]);
        }
    }

}

/**
 * @brief Checks if a sphere intersects with the view frustum.
 * @param frustum_planes The 6 planes of the frustum.
 * @param sphere_center The world-space center of the sphere.
 * @param sphere_radius The radius of the sphere.
 * @return True if the sphere is inside or intersecting the frustum, false if it's completely outside.
 */
static bool _RGL_FrustumIntersectsSphere(const vec4 frustum_planes[6], vec3 sphere_center, float sphere_radius, float culling_bias) {
    for (int i = 0; i < 6; i++) {
        float dist = frustum_planes[i][0] * sphere_center[0] + 
                     frustum_planes[i][1] * sphere_center[1] + 
                     frustum_planes[i][2] * sphere_center[2] + 
                     frustum_planes[i][3];

        // Is this the near plane? (Index 4)
        float effective_radius = (i == 4) ? sphere_radius + culling_bias : sphere_radius;

        if (dist < -effective_radius) {
            return false;
        }
    }
    return true;
}

/**
 * @brief (INTERNAL) Queues 6 lit quads to form a cube.
 *
 * This is the definitive, lighting-aware implementation. It provides a unique,
 * correct normal vector for each of the cube's six faces, allowing it to be
 * properly lit by the dynamic lighting system. It also respects the global
_RGL_SetTransform_ matrix.
 *
 * @param position The center of the cube in world space.
 * @param size The total side length of the cube.
 * @param material The material properties, used for diffuse color and base ambient light level.
 */
static void _RGL_DrawCubeFaces(vec3 position, float size, RGLMaterial material) {
    // 1. --- Pre-flight and Capacity Check ---
    if (!_RGL_EnsureCommandCapacity(6)) return; // A cube requires 6 quad commands.

    // 2. --- Define Base Geometry and Normals ---
    float half_size = size / 2.0f;

    // Define the 8 vertices of a cube centered at the origin.
    vec3 local_vertices[8] = {
        {-half_size, -half_size, -half_size}, // 0: BLF (Bottom-Left-Front)
        { half_size, -half_size, -half_size}, // 1: BRF
        { half_size,  half_size, -half_size}, // 2: TRF
        {-half_size,  half_size, -half_size}, // 3: TLF
        {-half_size, -half_size,  half_size}, // 4: BLB (Bottom-Left-Back)
        { half_size, -half_size,  half_size}, // 5: BRB
        { half_size,  half_size,  half_size}, // 6: TRB
        {-half_size,  half_size,  half_size}  // 7: TLB
    };

    // Define the 6 unique normal vectors, one for each face.
    const vec3 normals[6] = {
        { 0,  0,  1}, // Back   (+Z)
        { 0,  0, -1}, // Front  (-Z)
        { 1,  0,  0}, // Right  (+X)
        {-1,  0,  0}, // Left   (-X)
        { 0,  1,  0}, // Top    (+Y)
        { 0, -1,  0}  // Bottom (-Y)
    };

    // Define the vertex indices for each face in TL, BL, BR, TR order for the batcher.
    const int faces[6][4] = {
        {7, 4, 5, 6}, // Back Face
        {3, 0, 1, 2}, // Front Face
        {2, 1, 5, 6}, // Right Face
        {7, 4, 0, 3}, // Left Face
        {7, 6, 2, 3}, // Top Face
        {0, 1, 5, 4}  // Bottom Face
    };

    // 3. --- Build Transformation Matrix ---
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    glm_translate(model_matrix, position); // Apply translation

    // Also create a rotation-only matrix for transforming normals
    mat4 rotation_matrix = GLM_MAT4_IDENTITY_INIT;

    // Apply the global RGL transform if it's active
    if (RGL.use_transform) {
        glm_mat4_mul(RGL.transform, model_matrix, model_matrix);
        // Extract only the rotation part of the global transform for the normals
        glm_mat4_copy(RGL.transform, rotation_matrix);
        rotation_matrix[3][0] = rotation_matrix[3][1] = rotation_matrix[3][2] = 0.0f;
    }

    // 4. --- Prepare Shared Data ---
    vec4 diffuse_v4;
    SituationConvertColorToVec4(material.diffuse, diffuse_v4);

    // 5. --- Loop and Queue Each Face ---
    for (int i = 0; i < 6; i++) {
        // Use the safe "populate-then-commit" pattern.
        RGLInternalDraw* cmd = &RGL.commands[RGL.command_count];

        // --- Populate the Command ---
        cmd->texture.id = 0; // Untextured
        cmd->is_triangle = false;
        
        // Transform the face's normal vector by the rotation matrix.
        vec3 final_normal;
        glm_mat4_mulv3(rotation_matrix, normals[i], 0.0f, final_normal);
        glm_vec3_normalize(final_normal);

        // Populate all 4 vertices for this face's quad command.
        for (int j = 0; j < 4; j++) {
            // Transform the local vertex position to its final world position.
            glm_mat4_mulv3(model_matrix, local_vertices[faces[i][j]], 1.0f, cmd->world_positions[j]);

            // Assign the calculated normal, color, and lighting info.
            glm_vec3_copy(final_normal, cmd->normals[j]);
            glm_vec4_copy(diffuse_v4, cmd->colors[j]);
            cmd->light_levels[j] = material.ambient; // Use ambient as the base light contribution.

            // Assign dummy UVs for pipeline compatibility.
            cmd->tex_coords[j][0] = (j == 2 || j == 3) ? 1.0f : 0.0f; // TR or BR
            cmd->tex_coords[j][1] = (j == 1 || j == 2) ? 1.0f : 0.0f; // BL or BR
        }
        
        // Calculate average Z for depth sorting.
        cmd->z_depth = (cmd->world_positions[0][2] + cmd->world_positions[1][2] + cmd->world_positions[2][2] + cmd->world_positions[3][2]) * 0.25f;

        // --- Commit the Command ---
        RGL.command_count++;
    }
}

// --- Draw a cube ---
SITAPI void RGL_DrawCube(vec3 position, float size, RGLMaterial material) {
    if (size <= 0.0f || material.ambient < 0.0f || material.ambient > 1.0f) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid cube parameters");
        return;
    }
    _RGL_DrawCubeFaces(position, size, material);
}

// --- Draw a 3D line ---
SITAPI void RGL_DrawLine3D(vec3 start, vec3 end, float thickness, Color color) {
    if (thickness <= 0.0f || color.a == 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid line parameters");
        return;
    }
    _RGL_DrawLineQuad(start, end, thickness, color);
}

/**
 * @brief Draws a lit, textured 3D quad defined by four corner points.
 * This is the primary low-level function for drawing custom 3D geometry. It automatically
 * calculates texture coordinates from the provided sprite.
 *
 * @param p1, p2, p3, p4 The four corner vertices of the quad.
 *                       The winding order should be consistent (e.g., counter-clockwise).
 * @param normal The surface normal vector for the entire quad, used for lighting.
 * @param sprite The sprite to texture the quad with. The sprite's source_rect is used
 *               to map the texture to the quad.
 * @param tint The color to tint the quad.
 * @param base_light The base ambient light multiplier for the surface [0.0 - 1.0].
 */
SITAPI void RGL_DrawQuad3D(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 normal, RGLSprite sprite, Color tint, float base_light) {
    if (!_RGL_EnsureCommandCapacity(1)) return;

    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count++];
    cmd->texture = sprite.texture;
    cmd->is_triangle = false;
    cmd->z_depth = (p1[2] + p2[2] + p3[2] + p4[2]) * 0.25f; // Average Z for sorting

    // --- Assign Vertices ---
    // The batcher expects a specific winding order for triangles (0,1,2 and 0,2,3).
    // To make this function intuitive, we accept any 4 points and internally arrange them.
    // Let's assume a common order like Bottom-Left, Bottom-Right, Top-Right, Top-Left.
    glm_vec3_copy(p4, cmd->world_positions[0]); // Top-Left
    glm_vec3_copy(p1, cmd->world_positions[1]); // Bottom-Left
    glm_vec3_copy(p2, cmd->world_positions[2]); // Bottom-Right
    glm_vec3_copy(p3, cmd->world_positions[3]); // Top-Right

    // --- Calculate and Assign UVs from Sprite ---
    float u1 = 0.0f, v1 = 0.0f, u2 = 1.0f, v2 = 1.0f;
    if (sprite.texture.id != 0 && sprite.texture.width > 0 && sprite.texture.height > 0) {
        u1 = sprite.source_rect.x / sprite.texture.width;
        v1 = sprite.source_rect.y / sprite.texture.height;
        u2 = (sprite.source_rect.x + sprite.source_rect.width) / sprite.texture.width;
        v2 = (sprite.source_rect.y + sprite.source_rect.height) / sprite.texture.height;
    }
    // Map UVs to the corresponding corners
    cmd->tex_coords[0][0] = u1; cmd->tex_coords[0][1] = v1; // Top-Left
    cmd->tex_coords[1][0] = u1; cmd->tex_coords[1][1] = v2; // Bottom-Left
    cmd->tex_coords[2][0] = u2; cmd->tex_coords[2][1] = v2; // Bottom-Right
    cmd->tex_coords[3][0] = u2; cmd->tex_coords[3][1] = v1; // Top-Right

    // --- Assign Color, Normals, and Light Level ---
    vec4 v4_tint;
    SituationConvertColorToVec4(tint, v4_tint);
    for (int i = 0; i < 4; i++) {
        glm_vec4_copy(v4_tint, cmd->colors[i]);
        glm_vec3_copy(normal, cmd->normals[i]);
        // Use the field name consistent with your final RGLInternalDraw struct
        cmd->light_levels[i] = base_light; 
    }
}

// --- Draw a 3D triangle ---
SITAPI void RGL_DrawTriangle3D(vec3 p1, vec3 p2, vec3 p3, vec3 normal, vec2 uv1, vec2 uv2, vec2 uv3, RGLSprite sprite, Color tint, float base_light) {
    if (!_RGL_EnsureCommandCapacity(1)) return;

    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count++];
    cmd->texture = sprite.texture;
    cmd->is_triangle = true;
    cmd->z_depth = (p1[2] + p2[2] + p3[2]) / 3.0f;

    glm_vec3_copy(p1, cmd->world_positions[0]);
    glm_vec3_copy(p2, cmd->world_positions[1]);
    glm_vec3_copy(p3, cmd->world_positions[2]);

    glm_vec2_copy(uv1, cmd->tex_coords[0]);
    glm_vec2_copy(uv2, cmd->tex_coords[1]);
    glm_vec2_copy(uv3, cmd->tex_coords[2]);

    vec4 v4_tint;
    SituationConvertColorToVec4(tint, v4_tint);
    for (int i = 0; i < 3; i++) {
        glm_vec4_copy(v4_tint, cmd->colors[i]);
        cmd->light_levels[i] = base_light;
        glm_vec3_copy(normal, cmd->normals[i]);
    }
}

// --- Add a vertex ---
SITAPI int RGL_AddVertex(const char* level_name, RGLVertex3D_pos vertex) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level not found");
        return -1;
    }
    RGLLevel* level = &RGL.levels[index];
    if (level->vertex_count >= level->vertex_capacity) {
        size_t new_capacity = level->vertex_capacity == 0 ? 16 : level->vertex_capacity * 2;
        RGLVertex3D_pos* new_vertices = realloc(level->vertices, new_capacity * sizeof(RGLVertex3D_pos));
        if (!new_vertices) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate vertices");
            return -1;
        }
        level->vertices = new_vertices;
        level->vertex_capacity = new_capacity;
    }
    level->vertices[level->vertex_count] = vertex;
    return (int)level->vertex_count++;
}

// --- Add a wall ---
SITAPI bool RGL_AddWall(const char* level_name, RGLWall wall) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level not found");
        return false;
    }
    RGLLevel* level = &RGL.levels[index];
    if (wall.start_vertex < 0 || wall.start_vertex >= (int)level->vertex_count ||
        wall.end_vertex < 0 || wall.end_vertex >= (int)level->vertex_count ||
        wall.bottom_y >= wall->top_y || wall.brightness < 0.0f || wall.brightness > 1.0f) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid wall parameters");
        return false;
    }
    if (level->wall_count >= level->wall_capacity) {
        size_t new_capacity = level->wall_capacity == 0 ? 16 : level->wall_capacity * 2;
        RGLWall* new_walls = realloc(level->walls, new_capacity * sizeof(RGLWall));
        if (!new_walls) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate walls");
            return false;
        }
        level->walls = new_walls;
        level->wall_capacity = new_capacity;
    }
    level->walls[level->wall_count++] = wall;
    return true;
}

// --- Triangulate a flat using ear clipping ---
static bool _RGL_TriangulateFlat(const RGLFlat* flat, const RGLVertex3D_pos* vertices, int* triangle_indices, size_t* triangle_count) {
    if (flat->vertex_count < 3) return false;

    // Allocate temporary vertex list for the algorithm
    size_t n = flat->vertex_count;
    int* indices = malloc(n * sizeof(int));
    if (!indices) return false;
    for (size_t i = 0; i < n; i++) indices[i] = flat->vertex_indices[i];

    size_t max_triangles = n - 2;
    size_t tri_idx = 0;

    // Ear clipping main loop
    while (n >= 3) {
        bool found_ear = false;
        for (size_t i = 0; i < n; i++) {
            size_t prev = (i == 0) ? n - 1 : i - 1;
            size_t next = (i + 1) % n;
            int v0 = indices[prev];
            int v1 = indices[i];
            int v2 = indices[next];

            // Get 2D points for calculation
            vec2 p0 = { vertices[v0].x, vertices[v0].z };
            vec2 p1 = { vertices[v1].x, vertices[v1].z };
            vec2 p2 = { vertices[v2].x, vertices[v2].z };

            // Check for convexity (using cross product)
            float area = (p1[0] - p0[0]) * (p2[1] - p0[1]) - (p1[1] - p0[1]) * (p2[0] - p0[0]);
            if (area <= 0) continue; // Not a convex corner, cannot be an ear

            // Check if any other vertex lies inside this potential ear triangle
            bool is_ear = true;
            for (size_t j = 0; j < n; j++) {
                if (j == prev || j == i || j == next) continue;
                vec2 p = { vertices[indices[j]].x, vertices[indices[j]].z };
                
                // Barycentric coordinate test
                float denom = area;
                float alpha = ((p2[1] - p0[1]) * (p[0] - p0[0]) - (p2[0] - p0[0]) * (p[1] - p0[1])) / denom;
                float beta = ((p0[1] - p1[1]) * (p[0] - p0[0]) - (p0[0] - p1[0]) * (p[1] - p0[1])) / denom;
                float gamma = 1.0f - alpha - beta;
                if (alpha > 0 && beta > 0 && gamma > 0) { // Strictly inside
                    is_ear = false;
                    break;
                }
            }

            if (is_ear) {
                // Found an ear! Add it to the output triangles.
                if ((tri_idx / 3) < max_triangles) {
                    triangle_indices[tri_idx++] = v0;
                    triangle_indices[tri_idx++] = v1;
                    triangle_indices[tri_idx++] = v2;
                }
                
                // "Clip" the ear by removing its vertex from our list
                for (size_t j = i; j < n - 1; j++) {
                    indices[j] = indices[j + 1];
                }
                n--;
                found_ear = true;
                break;
            }
        }
        if (!found_ear) {
            // No ear found, the polygon might be self-intersecting or invalid.
            free(indices);
            *triangle_count = 0;
            return false;
        }
    }

    *triangle_count = tri_idx / 3;
    free(indices);
    return true;
}

// --- Add a flat ---
SITAPI bool RGL_AddFlat(const char* level_name, RGLFlat flat) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level not found");
        return false;
    }
    RGLLevel* level = &RGL.levels[index];
    if (flat.vertex_count < 3 || flat.brightness < 0.0f || flat.brightness > 1.0f) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid flat parameters");
        return false;
    }
    for (size_t i = 0; i < flat.vertex_count; i++) {
        if (flat.vertex_indices[i] < 0 || flat.vertex_indices[i] >= (int)level->vertex_count) {
            _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid vertex index");
            return false;
        }
    }
    if (level->flat_count >= level->flat_capacity) {
        size_t new_capacity = level->flat_capacity == 0 ? 16 : level->flat_capacity * 2;
        RGLFlat* new_flats = realloc(level->flats, new_capacity * sizeof(RGLFlat));
        if (!new_flats) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate flats");
            return false;
        }
        level->flats = new_flats;
        level->flat_capacity = new_capacity;
    }
    // Allocate and copy vertex indices
    int* new_indices = malloc(flat.vertex_count * sizeof(int));
    if (!new_indices) {
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate vertex indices");
        return false;
    }
    memcpy(new_indices, flat.vertex_indices, flat.vertex_count * sizeof(int));
    RGLFlat new_flat = flat;
    new_flat.vertex_indices = new_indices;
    level->flats[level->flat_count++] = new_flat;
    return true;
}

// --- Add a thing ---
SITAPI bool RGL_AddThing(const char* level_name, RGLThing thing) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level not found");
        return false;
    }
    RGLLevel* level = &RGL.levels[index];
    if (thing.scale <= 0.0f || thing.brightness < 0.0f || thing.brightness > 1.0f) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid thing parameters");
        return false;
    }
    if (level->thing_count >= level->thing_capacity) {
        size_t new_capacity = level->thing_capacity == 0 ? 16 : level->thing_capacity * 2;
        RGLThing* new_things = realloc(level->things, new_capacity * sizeof(RGLThing));
        if (!new_things) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate things");
            return false;
        }
        level->things = new_things;
        level->thing_capacity = new_capacity;
    }
    level->things[level->thing_count++] = thing;
    return true;
}

// --- Find level index ---
static int _RGL_FindLevelIndex(const char* level_name) {
    if (!level_name) return -1;
    for (size_t i = 0; i < RGL.level_count; i++) {
        if (strncmp(RGL.levels[i].name, level_name, 32) == 0) {
            return (int)i;
        }
    }
    return -1;
}

// --- Create a new level ---
SITAPI bool RGL_CreateLevel(const char* level_name) {
    if (!level_name || strlen(level_name) >= 32) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid level name");
        return false;
    }
    if (_RGL_FindLevelIndex(level_name) != -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level already exists");
        return false;
    }
    if (RGL.level_count >= RGL.level_capacity) {
        size_t new_capacity = RGL.level_capacity == 0 ? 4 : RGL.level_capacity * 2;
        RGLLevel* new_levels = realloc(RGL.levels, new_capacity * sizeof(RGLLevel));
        if (!new_levels) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate levels");
            return false;
        }
        RGL.levels = new_levels;
        RGL.level_capacity = new_capacity;
    }
    RGLLevel* level = &RGL.levels[RGL.level_count++];
    strncpy(level->name, level_name, 32);
    glm_vec3_zero(level->position);
    glm_vec3_zero(level->rotation_eul_deg);
    level->vertices = NULL;
    level->vertex_count = 0;
    level->vertex_capacity = 0;
    level->walls = NULL;
    level->wall_count = 0;
    level->wall_capacity = 0;
    level->flats = NULL;
    level->flat_count = 0;
    level->flat_capacity = 0;
    level->things = NULL;
    level->thing_count = 0;
    level->thing_capacity = 0;
    return true;
}

/**
 * @brief Destroys a specific named level and frees all its associated memory.
 * This function safely deallocates all dynamically allocated data within the level,
 * including vertices, walls, things, and the per-flat vertex index arrays.
 *
 * @param level_name The name of the level to destroy.
 * @return True on success, false if no level with that name was found.
 */
SITAPI bool RGL_DestroyLevelByName(const char* level_name) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Cannot destroy level: not found.");
        return false;
    }

    RGLLevel* level = &RGL.levels[index];

    // --- Free all dynamically allocated memory within the level struct ---
    free(level->vertices);
    free(level->walls);
    free(level->things);
    
    // CRITICAL: Free the per-flat vertex index arrays
    for (size_t i = 0; i < level->flat_count; i++) {
        free(level->flats[i].vertex_indices);
    }
    free(level->flats);

    // --- Remove the level from the main array ---
    // Shift all subsequent elements down by one
    for (size_t i = index; i < RGL.level_count - 1; i++) {
        RGL.levels[i] = RGL.levels[i + 1];
    }
    RGL.level_count--;

    // Adjust the active level index if necessary
    if (RGL.active_level_index == index) {
        RGL.active_level_index = -1; // The active level was destroyed
    } else if (RGL.active_level_index > index) {
        RGL.active_level_index--; // A level before the active one was removed
    }

    return true;
}

// --- Set active level ---
SITAPI bool RGL_SetActiveLevel(const char* level_name) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Level not found");
        return false;
    }
    RGL.active_level_index = index;
    return true;
}

/**
 * @brief Creates a combined RGLMesh object containing CPU-side geometry (vertices and indices)
 * representing the walls and flats of a level, optimized for stencil shadow volume casting.
 *
 * This function processes the structured data of a level and generates a single, contiguous
 * set of vertex positions and indices in RAM (cpu_vertices, cpu_indices). This CPU-side
 * data is precisely what the RGL_CastStencilShadowFromMesh function requires to build
 * shadow volumes.
 *
 * Walls are processed by extracting their four 2D vertices from the level data and generating
 * positions and indices for two triangles forming a quad in the output buffers.
 *
 * Flats are processed by extracting their 2D vertex outlines (XZ plane). The robust
 * _RGL_TriangulateFlat function (an ear clipping algorithm) is then utilized to convert
 * these potentially non-convex 2D polygons into a list of triangles defined by indices
 * referencing the flat's vertices. The vertices for the flat are copied to the output
 * vertex buffer, and the triangulated indices are remapped to reference these copied
 * vertices in the combined buffer. This ensures that even complex flat shapes contribute
 * correct triangular geometry to the shadow mesh. Includes basic error handling for
 * triangulation failures (such flats will be skipped).
 *
 * @param level_name The name of the level to create the mesh from.
 * @return An RGLMesh object containing only CPU-side vertex positions and indices.
 *         Returns an invalid mesh (id=0, null pointers) on failure (e.g., level not found, memory allocation).
 * @note This function does NOT generate or store UV coordinates or normal vectors in the
 *       CPU data, as they are not required by RGL_CastStencilShadowFromMesh.
 * @note This function does NOT create or upload any GPU buffers (VAO, VBO, EBO). The
 *       returned mesh struct will have vao, vbo, and ebo fields set to 0.
 * @note The generated mesh is intended SOLELY for input to RGL_CastStencilShadowFromMesh.
 *       It cannot be drawn directly using RGL_DrawMesh() as it lacks necessary GPU data
 *       and vertex attributes (UVs, Normals, Color, BaseLight).
 * @note Requires the _RGL_TriangulateFlat function to be implemented correctly.
 * @note Things (billboards) are not included in the generated mesh.
 */
SITAPI RGLMesh RGL_CreateMeshFromLevel(const char* level_name) {
    RGLMesh new_mesh = {0}; // Start with a null mesh
    int level_idx = _RGL_FindLevelIndex(level_name);
    if (level_idx == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Level not found for mesh creation.");
        return new_mesh;
    }
    RGLLevel* level = &RGL.levels[level_idx];

    // --- Pass 1: Count total vertices and indices needed ---
    // This is crucial to allocate the correct amount of memory ONCE.
    // Note: We duplicate vertices here as needed for indexing. Welding identical
    // vertices would be more complex but save memory if vertex sharing was a goal.
    
    size_t total_vertex_count = 0;
    size_t total_index_count = 0;
    
    // Walls: Each wall is a quad (4 vertices). Stencil shadows use triangles (6 indices).
    total_vertex_count += level->wall_count * 4;
    total_index_count += level->wall_count * 6;
    
    // Flats: Each flat contributes its original vertices (N vertices). Stencil shadows
    // use triangulated geometry (N-2 triangles = (N-2)*3 indices).
    for (size_t i = 0; i < level->flat_count; i++) {
        RGLFlat* flat = &level->flats[i];
        if (flat->vertex_count >= 3) {
            total_vertex_count += flat->vertex_count;
            total_index_count += (flat->vertex_count - 2) * 3; // Max possible indices after triangulation
        }
    }

    // Return an empty mesh if there's no geometry to process
    if (total_vertex_count == 0) return new_mesh;

    // --- Pass 2: Allocate CPU-side memory for vertices and indices ---
    // These are the buffers required by RGL_CastStencilShadowFromMesh
    new_mesh.cpu_vertices = malloc(total_vertex_count * sizeof(vec3));
    new_mesh.cpu_indices = malloc(total_index_count * sizeof(unsigned int)); // Allocate for max possible indices
    // We do NOT need cpu_texcoords or cpu_normals for shadow volumes

    if (!new_mesh.cpu_vertices || !new_mesh.cpu_indices) {
        free(new_mesh.cpu_vertices);
        free(new_mesh.cpu_indices);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate CPU memory for level mesh (shadow volume).");
        return (RGLMesh){0}; // Return invalid mesh
    }
    
    // Store the allocated capacity, the actual count will be set later
    size_t allocated_vertex_capacity = total_vertex_count;
    size_t allocated_index_capacity = total_index_count;

    // Reset counts before populating
    new_mesh.vertex_count = 0;
    new_mesh.index_count = 0;
    
    // --- Pass 3: Populate the CPU-side mesh data buffers ---
    size_t current_vert_offset = 0;
    size_t current_index_offset = 0;

    // Populate from walls
    for (size_t i = 0; i < level->wall_count; i++) {
        RGLWall* wall = &level->walls[i];
        // Get 2D vertices from level, apply 3D wall height
        vec3 p1 = {level->vertices[wall->start_vertex].x, wall->bottom_y, level->vertices[wall->start_vertex].z};
        vec3 p2 = {level->vertices[wall->end_vertex].x,   wall->bottom_y, level->vertices[wall->end_vertex].z};
        vec3 p3 = {level->vertices[wall->end_vertex].x,   wall->top_y,    level->vertices[wall->end_vertex].z};
        vec3 p4 = {level->vertices[wall->start_vertex].x, wall->top_y,    level->vertices[wall->start_vertex].z};

        // Store vertex positions in the CPU buffer
        glm_vec3_copy(p1, new_mesh.cpu_vertices[current_vert_offset + 0]);
        glm_vec3_copy(p2, new_mesh.cpu_vertices[current_vert_offset + 1]);
        glm_vec3_copy(p3, new_mesh.cpu_vertices[current_vert_offset + 2]);
        glm_vec3_copy(p4, new_mesh.cpu_vertices[current_vert_offset + 3]);
        
        // Store indices for the two triangles forming the quad (0,1,2, 0,2,3)
        new_mesh.cpu_indices[current_index_offset + 0] = current_vert_offset + 0;
        new_mesh.cpu_indices[current_index_offset + 1] = current_vert_offset + 1;
        new_mesh.cpu_indices[current_index_offset + 2] = current_vert_offset + 2;
        new_mesh.cpu_indices[current_index_offset + 3] = current_vert_offset + 0;
        new_mesh.cpu_indices[current_index_offset + 4] = current_vert_offset + 2;
        new_mesh.cpu_indices[current_index_offset + 5] = current_vert_offset + 3;

        // Update offsets and counts
        current_vert_offset += 4;
        current_index_offset += 6;
        new_mesh.vertex_count += 4;
        new_mesh.index_count += 6;
    }
    
    // Populate from flats (this requires the triangulation helper for non-convex shapes)
    for (size_t i = 0; i < level->flat_count; i++) {
        RGLFlat* flat = &level->flats[i];
        if (flat->vertex_count < 3) continue;

        // Triangulate the flat's 2D vertices (XZ plane)
        size_t max_flat_indices = (flat->vertex_count - 2) * 3;
        int* flat_triangle_indices_local = malloc(max_flat_indices * sizeof(int)); // Indices relative to flat->vertex_indices
        if (!flat_triangle_indices_local) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate temp indices for flat triangulation during mesh creation.");
             // Continue to next flat, losing this one's geometry
             continue;
        }
        size_t flat_triangle_count = 0;

        // Call the triangulation function to get triangle indices for this flat
        if (_RGL_TriangulateFlat(flat, level->vertices, flat_triangle_indices_local, &flat_triangle_count)) {
            // Copy this flat's vertices into our combined mesh's CPU vertex buffer FIRST
            size_t flat_vert_start_index_in_new_mesh = current_vert_offset;
            for(size_t v_idx = 0; v_idx < flat->vertex_count; v_idx++) {
                int original_level_v_idx = flat->vertex_indices[v_idx]; // Index in the level's main vertices array
                RGLVertex3D_pos* v = &level->vertices[original_level_v_idx];
                // Store the vertex position with the correct Y coordinate from the flat
                glm_vec3_copy((vec3){v->x, flat->y, v->z}, new_mesh.cpu_vertices[current_vert_offset++]);
            }
            // Update vertex count
            new_mesh.vertex_count += flat->vertex_count;

            // Now copy the triangulated indices into the combined mesh's CPU index buffer
            for (size_t t_idx = 0; t_idx < flat_triangle_count * 3; t_idx++) {
                // The index `flat_triangle_indices_local[t_idx]` is 0-based relative to the *flat's vertex_indices array*.
                // If this local index is `k`, it means the k-th vertex in `flat->vertex_indices`.
                // This k-th vertex was copied into `new_mesh.cpu_vertices` at the absolute index `flat_vert_start_index_in_new_mesh + k`.
                int local_flat_vertex_index = flat_triangle_indices_local[t_idx];
                new_mesh.cpu_indices[current_index_offset++] = flat_vert_start_index_in_new_mesh + local_flat_vertex_index;
            }
            // Update index count
            new_mesh.index_count += flat_triangle_count * 3;

        } else {
             _SituationSetWarning("RGL_CreateMeshFromLevel: Triangulation failed for a flat. This flat's geometry will be missing from the shadow mesh.");
             // Triangulation failed - do not add this flat's geometry/indices.
        }

        // Cleanup the temporary local indices buffer for this flat
        free(flat_triangle_indices_local);
    }
    
    // --- Pass 4: Do NOT create or upload GPU buffers (VAO/VBO/EBO) ---
    // This mesh is intended only for CPU-side shadow volume generation,
    // which operates directly on cpu_vertices and cpu_indices.

    // --- Pass 5: Set the mesh ID and return ---
    // We don't use a global mesh registry for this specific function's output currently.
    // A simple non-zero ID indicates a valid, user-managed mesh.
    new_mesh.id = 1; // Indicate success and validity

    return new_mesh;
}

SITAPI RGLMesh RGL_LoadMeshFromFile(const char* filename) {
    RGLMesh mesh = {0}; // Always start with a null mesh

    // --- 1. Load File From Disk using the Platform Layer ---
    unsigned int file_size = 0;
    char* file_text = (char*)SituationLoadFileData(filename, &file_size);
    if (!file_text) {
        // situation.h will have already set the error message.
        return mesh;
    }

    // --- 2. Parse the OBJ data from the memory buffer ---
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = NULL;
    size_t num_shapes;
    tinyobj_material_t* materials = NULL;
    size_t num_materials;

    // We must use TINYOBJ_FLAG_TRIANGULATE to ensure we only get triangles.
    int result = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, file_text, file_size, TINYOBJ_FLAG_TRIANGULATE);
    
    // The raw text data is no longer needed.
    free(file_text);

    if (result != TINYOBJ_SUCCESS) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Failed to parse OBJ file data.");
        return mesh;
    }
    
    // We cannot proceed if the file has no geometry.
    if (attrib.num_vertices == 0 || attrib.num_faces == 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "OBJ file has no vertex or face data.");
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
        return mesh;
    }

    // --- 3. Process and Interleave Vertex Data ---
    // This is the most complex step. We convert the separate OBJ arrays (v, vt, vn)
    // into a single, interleaved buffer that OpenGL prefers.

    size_t total_indices = attrib.num_faces; // Since we triangulated, num_faces is the total number of indices.
    RGLVertex3D* vertex_data = malloc(sizeof(RGLVertex3D) * total_indices);
    if (!final_positions || !final_texcoords || !final_normals || !final_indices) {
        free(final_positions); free(final_texcoords); free(final_normals); free(final_indices);
        tinyobj_attrib_free(&attrib); tinyobj_shapes_free(shapes, num_shapes); tinyobj_materials_free(materials, num_materials);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for mesh processing.");
        return mesh;
    }
    
    // Loop through each vertex of each face defined in the OBJ
    for (size_t i = 0; i < total_indices; i++) {
        tinyobj_vertex_index_t f = attrib.faces[i];

        // --- Position (v) ---
        // This is always present.
        final_positions[i][0] = attrib.vertices[3 * f.v_idx + 0];
        final_positions[i][1] = attrib.vertices[3 * f.v_idx + 1];
        final_positions[i][2] = attrib.vertices[3 * f.v_idx + 2];
        
        // --- Texture Coordinate (vt) ---
        // Check if texture coordinates are provided in the OBJ.
        if (attrib.num_texcoords > 0 && f.vt_idx >= 0) {
            final_texcoords[i][0] = attrib.texcoords[2 * f.vt_idx + 0];
            final_texcoords[i][1] = 1.0f - attrib.texcoords[2 * f.vt_idx + 1]; // Flip V for OpenGL
        } else {
            glm_vec2_zero(final_texcoords[i]); // Default to (0,0) if not present.
        }

        // --- Normal (vn) ---
        // Check if normals are provided in the OBJ.
        if (attrib.num_normals > 0 && f.vn_idx >= 0) {
            final_normals[i][0] = attrib.normals[3 * f.vn_idx + 0];
            final_normals[i][1] = attrib.normals[3 * f.vn_idx + 1];
            final_normals[i][2] = attrib.normals[3 * f.vn_idx + 2];
        } else {
            // If no normals, we should calculate them. For now, we'll just zero them.
            // A proper implementation would calculate face normals here.
            glm_vec3_zero(final_normals[i]);
        }
        
        // Our vertex array is a simple, un-indexed list for now.
        final_indices[i] = i;
    }

    mesh.vertex_count = total_indices;
    mesh.index_count = total_indices;

    // --- 4. Upload Data to the GPU ---
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo); // We'll use one VBO for all attributes packed together.
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    
    // --- VBO ---
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    // Calculate total buffer size
    size_t pos_size = mesh.vertex_count * sizeof(vec3);
    size_t tex_size = mesh.vertex_count * sizeof(vec2);
    size_t norm_size = mesh.vertex_count * sizeof(vec3);
    glBufferData(GL_ARRAY_BUFFER, pos_size + tex_size + norm_size, NULL, GL_STATIC_DRAW);

    // Upload data in chunks
    glBufferSubData(GL_ARRAY_BUFFER, 0, pos_size, final_positions);
    glBufferSubData(GL_ARRAY_BUFFER, pos_size, tex_size, final_texcoords);
    glBufferSubData(GL_ARRAY_BUFFER, pos_size + tex_size, norm_size, final_normals);

    // --- EBO ---
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.index_count * sizeof(unsigned int), final_indices, GL_STATIC_DRAW);

    // --- Set Vertex Attribute Pointers for the separate data in the VBO ---
    // aPos (matches layout 0 in the main shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
    // aNormal (matches layout 1 in the main shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)(pos_size + tex_size));
    glEnableVertexAttribArray(1);
    // aTexCoord (matches layout 2 in the main shader)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)pos_size);
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    mesh.gpu_mesh = SituationCreateMesh(vertex_data, total_indices, sizeof(RGLVertex3D), NULL, 0);
    // --- 5. Store CPU-side copies for physics and shadows ---
    mesh.cpu_vertices = final_positions; // Transfer ownership of the pointer
    mesh.cpu_texcoords = final_texcoords;
    mesh.cpu_normals = final_normals;
    mesh.cpu_indices = final_indices;    // Transfer ownership of the pointer
    // The other temporary buffers can be freed.
    free(final_texcoords);
    free(final_normals);
    
    // --- 6. Final Cleanup of OBJ Parser Data ---
    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);

    // TODO: Add the mesh to a managed list in RGLState and assign it a real ID.
    mesh.id = 1; // Placeholder ID
    
    return mesh;
}

// Helper function to write a line to the file, for cleanliness
static bool _RGL_WriteObjLine(FILE* fp, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(fp, format, args);
    va_end(args);
    return result >= 0;
}

/**
 * @brief Saves a mesh's geometry to a file in the Wavefront OBJ format.
 *
 * This function writes the CPU-side vertex data of an RGLMesh object to a text file.
 * The resulting .obj file will contain vertex positions (v), texture coordinates (vt),
 * normals (vn), and faces (f). This allows for exporting procedurally generated or
 * loaded geometry for use in external 3D modeling applications.
 *
 * @param mesh The RGLMesh object to save. The mesh must have valid CPU data.
 * @param filename The path and name of the file to save (e.g., "exported_model.obj").
 * @return True if the mesh was saved successfully, false on failure (e.g., file access error, invalid mesh).
 */
SITAPI bool RGL_SaveMeshToFile(RGLMesh mesh, const char* filename) {
    // --- 1. Sanity Checks ---
    if (mesh.id == 0 || !mesh.cpu_vertices || !mesh.cpu_indices || !mesh.cpu_texcoords || !mesh.cpu_normals) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Cannot save mesh: mesh is invalid or missing CPU data (v, vt, vn, or f).");
        return false;
    }
    if (!filename) { /* ... */ return false; }

    // --- 2. Open File ---
    FILE* obj_file = fopen(filename, "w");
    if (!obj_file) { /* ... */ return false; }

    bool success = true;

    // --- 3. Write Header ---
    success &= _RGL_WriteObjLine(obj_file, "# KaOS Engine - RGL Mesh Export\n");
    success &= _RGL_WriteObjLine(obj_file, "# Vertices: %d\n", mesh.vertex_count);
    success &= _RGL_WriteObjLine(obj_file, "# Faces: %d\n\n", mesh.index_count / 3);

    // --- 4. Write ALL Vertex Data ---
    // Write vertex positions (v)
    for (int i = 0; i < mesh.vertex_count; i++) {
        success &= _RGL_WriteObjLine(obj_file, "v %.6f %.6f %.6f\n", mesh.cpu_vertices[i][0], mesh.cpu_vertices[i][1], mesh.cpu_vertices[i][2]);
    }
    success &= _RGL_WriteObjLine(obj_file, "\n");

    // Write texture coordinates (vt)
    for (int i = 0; i < mesh.vertex_count; i++) {
        // OBJ format V is bottom-to-top, but we store it top-to-bottom for OpenGL. We must flip it back on export.
        success &= _RGL_WriteObjLine(obj_file, "vt %.6f %.6f\n", mesh.cpu_texcoords[i][0], 1.0f - mesh.cpu_texcoords[i][1]);
    }
    success &= _RGL_WriteObjLine(obj_file, "\n");

    // Write vertex normals (vn)
    for (int i = 0; i < mesh.vertex_count; i++) {
        success &= _RGL_WriteObjLine(obj_file, "vn %.6f %.6f %.6f\n", mesh.cpu_normals[i][0], mesh.cpu_normals[i][1], mesh.cpu_normals[i][2]);
    }
    success &= _RGL_WriteObjLine(obj_file, "\n");

    // --- 5. Write Face Data ---
    // Now we write the full "f v/vt/vn" format.
    if (success) {
        success &= _RGL_WriteObjLine(obj_file, "o rgl_mesh\n"); // Object name
        success &= _RGL_WriteObjLine(obj_file, "s 1\n"); // Smoothing group
        for (int i = 0; i < mesh.index_count; i += 3) {
            // OBJ is 1-indexed, so we add 1 to each index.
            unsigned int i1 = mesh.cpu_indices[i] + 1;
            unsigned int i2 = mesh.cpu_indices[i+1] + 1;
            unsigned int i3 = mesh.cpu_indices[i+2] + 1;
            success &= _RGL_WriteObjLine(obj_file, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                                         i1, i1, i1,  // Vertex 1: pos/uv/norm
                                         i2, i2, i2,  // Vertex 2: pos/uv/norm
                                         i3, i3, i3); // Vertex 3: pos/uv/norm
        }
    }

    // --- 6. Cleanup ---
    fclose(obj_file);
    if (!success) { /* ... (existing error handling) ... */ return false; }

    return true;
}


SITAPI void RGL_DestroyMesh(RGLMesh* mesh) {
    if (!mesh || mesh->id == 0) return;

    // The ONLY thing RGL needs to do is ask situation.h to destroy the GPU mesh.
    SituationDestroyMesh(&mesh->gpu_mesh);
    
    // RGL is still responsible for the CPU-side data it allocated.
    free(mesh->cpu_vertices);
    free(mesh->cpu_texcoords);
    free(mesh->cpu_normals);
    free(mesh->cpu_indices);
    
    // Zero out the struct to invalidate it.
    memset(mesh, 0, sizeof(RGLMesh));
}

// --- Standard Primitives ---

SITAPI RGLMesh RGL_GenMeshPlane(float width, float length, int res_x, int res_z) {
    par_shapes_mesh* shape = par_shapes_create_plane(res_x, res_z);
    // par_shapes creates a plane from -1 to +1. We scale it to the desired size.
    par_shapes_scale(shape, width / 2.0f, length / 2.0f, 1.0f);
    // It creates an XY plane, but typically in 3D engines, a ground plane is XZ.
    par_shapes_rotate(shape, -M_PI / 2.0f, (float[]){1, 0, 0});
    // It lacks normals by default, so we compute them.
    par_shapes_compute_normals(shape);
    return _RGL_CreateMeshFromParShape(shape);
}

SITAPI RGLMesh RGL_GenMeshCube(float width, float height, float depth) {
    par_shapes_mesh* shape = par_shapes_create_cube();
    // Scale the -1 to +1 cube to the desired dimensions.
    par_shapes_scale(shape, width / 2.0f, height / 2.0f, depth / 2.0f);
    par_shapes_compute_normals(shape); // Ensure normals are present
    return _RGL_CreateMeshFromParShape(shape);
}

SITAPI RGLMesh RGL_GenMeshSphere(float radius, int slices, int stacks) {
    par_shapes_mesh* shape = par_shapes_create_parametric_sphere(slices, stacks);
    par_shapes_scale(shape, radius, radius, radius);
    // Normals are inherent to a sphere, par_shapes generates them.
    return _RGL_CreateMeshFromParShape(shape);
}

SITAPI RGLMesh RGL_GenMeshCylinder(float radius, float height, int slices) {
    par_shapes_mesh* shape = par_shapes_create_cylinder(slices, 8); // 8 stacks is a reasonable default
    // par_shapes creates a cylinder of radius 1 and height 2 (-1 to +1).
    par_shapes_scale(shape, radius, height / 2.0f, radius);
    par_shapes_compute_normals(shape);
    return _RGL_CreateMeshFromParShape(shape);
}

// NOTE: This implementation is more advanced. It re-calculates vertex positions
// to provide a more intuitive API than what par_shapes offers directly.
SITAPI RGLMesh RGL_GenMeshTorus(float major_radius, float tube_radius, int major_segments, int tube_segments) {
    par_shapes_mesh* shape = par_shapes_create_torus(major_segments, tube_segments, 1.0f); // Create a 'base' torus

    // Manually adjust vertices to match the intuitive radii, preventing distortion.
    for (int i = 0; i < shape->npoints; ++i) {
        float* p = shape->points + i * 3;
        // p[0] and p[2] form a circle of radius 1. Get the direction.
        float dir_x = p[0];
        float dir_z = p[2];
        float len = sqrtf(dir_x * dir_x + dir_z * dir_z);
        if (len > 0) {
            dir_x /= len;
            dir_z /= len;
        }

        // p[1] is the height along the tube.
        // The distance from the center of the tube is `p[1]`.
        // The distance from the center of the torus is `len`.
        
        // Find the center of the tube cross-section on the XZ plane
        float center_x = dir_x * major_radius;
        float center_z = dir_z * major_radius;

        // The point's position relative to the tube's center is its normal
        // scaled by the tube radius.
        float norm_x = shape->normals[i * 3 + 0];
        float norm_y = shape->normals[i * 3 + 1];
        float norm_z = shape->normals[i * 3 + 2];

        p[0] = center_x + norm_x * tube_radius;
        p[1] = norm_y * tube_radius; // The Y position is just from the tube's own circle.
        p[2] = center_z + norm_z * tube_radius;
    }

    // Normals are already correct from par_shapes, so we just return the modified mesh.
    return _RGL_CreateMeshFromParShape(shape);
}


// --- Compound Shapes ---

SITAPI RGLMesh RGL_GenMeshCapsule(float radius, float height, int slices, int stacks) {
    if (height < 0) height = 0;
    
    // Create the cylinder part
    par_shapes_mesh* cyl = par_shapes_create_cylinder(slices, 1);
    par_shapes_scale(cyl, radius, height / 2.0f, radius);

    // Create the sphere for the caps
    par_shapes_mesh* sphere = par_shapes_create_parametric_sphere(slices, stacks);
    par_shapes_scale(sphere, radius, radius, radius);

    // Split the sphere into top and bottom halves
    par_shapes_mesh* top_cap = par_shapes_clone(sphere, 0, sphere->ntriangles * 3);
    par_shapes_mesh* bot_cap = par_shapes_clone(sphere, 0, sphere->ntriangles * 3);
    par_shapes_remove_triangles(top_cap, 0, top_cap->ntriangles / 2); // Keep top half
    par_shapes_remove_triangles(bot_cap, bot_cap->ntriangles / 2, bot_cap->ntriangles / 2); // Keep bottom half

    // Position the caps at the ends of the cylinder
    par_shapes_translate(top_cap, 0, height / 2.0f, 0);
    par_shapes_translate(bot_cap, 0, -height / 2.0f, 0);

    // Merge everything together
    par_shapes_merge_and_free(cyl, top_cap);
    par_shapes_merge_and_free(cyl, bot_cap);

    par_shapes_free_mesh(sphere); // Free the original sphere
    
    // Weld vertices at the seams for smooth normals
    par_shapes_weld(cyl, 0.001f, NULL);
    par_shapes_compute_normals(cyl);

    return _RGL_CreateMeshFromParShape(cyl);
}

// --- Polyhedra ---

SITAPI RGLMesh RGL_GenMeshIcosahedron(float radius) {
    par_shapes_mesh* shape = par_shapes_create_icosahedron();
    par_shapes_scale(shape, radius, radius, radius);
    par_shapes_compute_normals(shape);
    return _RGL_CreateMeshFromParShape(shape);
}

SITAPI RGLMesh RGL_GenMeshDodecahedron(float radius) {
    par_shapes_mesh* shape = par_shapes_create_dodecahedron();
    par_shapes_scale(shape, radius, radius, radius);
    par_shapes_compute_normals(shape);
    return _RGL_CreateMeshFromParShape(shape);
}

// --- Procedural & Organic ---

SITAPI RGLMesh RGL_GenMeshKnot(float major_radius, float tube_radius, int major_segments, int tube_segments) {
    par_shapes_mesh* shape = par_shapes_create_trefoil_knot(major_segments, tube_segments, tube_radius);
    par_shapes_scale(shape, major_radius, major_radius, major_radius);
    // Normals are generated by par_shapes for this.
    return _RGL_CreateMeshFromParShape(shape);
}

SITAPI RGLMesh RGL_GenMeshRock(float radius, int subdivisions, int seed) {
    par_shapes_mesh* shape = par_shapes_create_rock(seed, subdivisions);
    par_shapes_scale(shape, radius, radius, radius);
    par_shapes_compute_normals(shape);
    return _RGL_CreateMeshFromParShape(shape);
}

/**
 * @brief Draws the active level, including walls, flats, and things, with full dynamic lighting.
 *
 * This is the definitive, feature-complete version that:
 * - Uses PushMatrix/PopMatrix to safely apply a world transform to the level without
 *   affecting other draw calls in the frame.
 * - Applies a world transform (position, rotation) to the entire level.
 * - Uses robust ear-clipping triangulation for non-convex floors and ceilings.
 * - Calculates correct surface normals for all geometry to enable dynamic lighting.
 * - Updates the world position of lights attached to "things" within the level.
 *
 * @param level_name The name of the level to draw. If NULL, draws the active level.
 */
SITAPI void RGL_DrawLevel(void) {
    // 1. --- PRE-FLIGHT CHECKS ---
    if (RGL.active_level_index < 0 || RGL.active_level_index >= (int)RGL.level_count) return;
    RGLLevel* level = &RGL.levels[RGL.active_level_index];

    // 2. --- SAVE THE CURRENT TRANSFORMATION STATE (CRITICAL) ---
    // This is the PushMatrix from your original file. It saves the current camera
    // and transform state so we can safely modify it for the level and then restore it.
    RGL_PushMatrix();

    // 3. --- CALCULATE AND APPLY THE LEVEL'S TRANSFORM (from your original file) ---
    // This transform will be applied to all subsequent RGL_Draw... calls until we pop.
    mat4 level_transform;
    glm_mat4_identity(level_transform);
    glm_translate(level_transform, level->position);
    if (level->rotation_eul_deg[1] != 0.0f) glm_rotate_y(level_transform, glm_rad(level->rotation_eul_deg[1]), level_transform);
    if (level->rotation_eul_deg[0] != 0.0f) glm_rotate_x(level_transform, glm_rad(level->rotation_eul_deg[0]), level_transform);
    if (level->rotation_eul_deg[2] != 0.0f) glm_rotate_z(level_transform, glm_rad(level->rotation_eul_deg[2]), level_transform);

    // Set this transform for all of the level's geometry.
    // The `RGL_DrawSpritePro` and other drawing functions will automatically multiply their
    // vertices by this matrix if `RGL.use_transform` is true.
    RGL_SetTransform(level_transform);

    // 4. --- DRAW WALLS (with lighting from the patch) ---
    for (size_t i = 0; i < level->wall_count; i++) {
        const RGLWall* wall = &level->walls[i];
        RGLVertex3D_pos v_start = level->vertices[wall->start_vertex];
        RGLVertex3D_pos v_end = level->vertices[wall->end_vertex];

        vec3 p1 = {v_start.x, wall->bottom_y, v_start.z};
        vec3 p2 = {v_end.x,   wall->bottom_y, v_end.z};
        vec3 p3 = {v_end.x,   wall->top_y,    v_end.z};
        vec3 p4 = {v_start.x, wall->top_y,    v_start.z};

        vec3 edge = {v_end.x - v_start.x, 0.0f, v_end.z - v_start.z};
        vec3 normal;
        glm_vec3_cross(edge, (vec3){0,1,0}, normal); // normal = edge x up
        glm_vec3_normalize(normal);

        // NOTE: The new RGL_DrawQuad3D needs the normal.
        RGL_DrawQuad3D(p1, p2, p3, p4, normal, wall->texture, WHITE, wall->brightness);
    }

    // 5. --- DRAW FLATS (with lighting and robust triangulation) ---
    for (size_t i = 0; i < level->flat_count; i++) {
        const RGLFlat* flat = &level->flats[i]; // Corrected access to level->flats
        if (flat->vertex_count < 3) continue;

        // Determine normal based on vertex winding order (This part is fine)
        vec3 v0_2d = {level->vertices[flat->vertex_indices[0]].x, level->vertices[flat->vertex_indices[0]].z};
        vec3 v1_2d = {level->vertices[flat->vertex_indices[1]].x, level->vertices[flat->vertex_indices[1]].z};
        vec3 v2_2d = {level->vertices[flat->vertex_indices[2]].x, level->vertices[flat->vertex_indices[2]].z};
        vec2 edge1_2d, edge2_2d;
        glm_vec2_sub(v1_2d, v0_2d, edge1_2d);
        glm_vec2_sub(v2_2d, v0_2d, edge2_2d);
        // Use 2D cross product for winding check (z component of 3D cross product)
        float cross_z = edge1_2d[0] * edge2_2d[1] - edge1_2d[1] * edge2_2d[0];
        vec3 normal = {0.0f, (cross_z > 0.0f) ? 1.0f : -1.0f, 0.0f}; // Up or Down normal

        // Use the robust ear-clipping triangulation
        size_t max_output_indices = (flat->vertex_count - 2) * 3; // Max possible indices
        int* triangle_indices = malloc(max_output_indices * sizeof(int));
        if (!triangle_indices) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate triangle indices for flat drawing.");
            continue; // Skip this flat
        }

        size_t triangle_count = 0;
        // *** PROBLEM FIXED HERE: CALL THE TRIANGULATION FUNCTION ***
        if (_RGL_TriangulateFlat(flat, level->vertices, triangle_indices, &triangle_count)) {
            // Now iterate through the INDICES returned by the triangulation
            for (size_t t_idx = 0; t_idx < triangle_count * 3; t_idx += 3) {
                // Get the ORIGINAL vertex indices for this triangle
                int original_v_idx1 = triangle_indices[t_idx];
                int original_v_idx2 = triangle_indices[t_idx + 1];
                int original_v_idx3 = triangle_indices[t_idx + 2];

                // Get the actual 3D positions from the level's vertex list, applying the flat's Y
                vec3 pos[3] = {
                    {level->vertices[original_v_idx1].x, flat->y, level->vertices[original_v_idx1].z},
                    {level->vertices[original_v_idx2].x, flat->y, level->vertices[original_v_idx2].z},
                    {level->vertices[original_v_idx3].x, flat->y, level->vertices[original_v_idx3].z}
                };
            
                // Calculate UVs based on XZ coordinates and flat's scale
                vec2 uvs[3] = {
                    {pos[0][0] * flat->u_scale, pos[0][2] * flat->v_scale},
                    {pos[1][0] * flat->u_scale, pos[1][2] * flat->v_scale},
                    {pos[2][0] * flat->u_scale, pos[2][2] * flat->v_scale}
                };

                // Draw the triangle
                RGL_DrawTriangle3D(pos[0], pos[1], pos[2], normal, uvs[0], uvs[1], uvs[2], flat->texture, WHITE, flat->brightness);
            }
        } else {
            // Triangulation failed (e.g., self-intersecting polygon)
            _SituationSetWarning("RGL_DrawLevel: Triangulation failed for a flat.");
        }

        // --- Cleanup the temporary indices buffer ---
        free(triangle_indices);
    }

    // 6. --- DRAW THINGS (with attached light logic from the patch) ---
    for (size_t i = 0; i < level->thing_count; i++) {
        const RGLThing* thing = &level->things[i];
        vec3 thing_pos = { thing->x, thing->y, thing->z }; // Local position

        // Update the position of any light attached to this thing
        if (thing->attached_light_id > 0) {
            // Transform the thing's local position into world space using the level's transform
            vec3 world_thing_pos;
            glm_mat4_mulv3(level_transform, thing_pos, 1.0f, world_thing_pos);
            RGL_SetLightPosition(thing->attached_light_id, world_thing_pos);
        }

        // Draw the billboard. It will be correctly transformed by RGL_SetTransform.
        RGL_DrawBillboard(thing->texture, thing_pos, (vec2){thing->scale, thing->scale}, WHITE);
    }

    // 7. --- RESTORE THE PREVIOUS TRANSFORMATION STATE (CRITICAL) ---
    // Reset the transform flag so it's not applied to non-level objects.
    RGL_ResetTransform();
    // Restore the exact view/projection matrices we had before this function was called.
    RGL_PopMatrix();
}

// --- Debug visualization ---
SITAPI void RGL_DrawLevelDebug(void) {
    if (RGL.active_level_index < 0 || RGL.active_level_index >= (int)RGL.level_count) return;
    RGLLevel* level = &RGL.levels[RGL.active_level_index];

    // Draw wireframe walls
    for (size_t i = 0; i < level->wall_count; i++) {
        const RGLWall* wall = &level->walls[i];
        vec3 v_start = { level->vertices[wall->start_vertex].x, wall->bottom_y, level->vertices[wall->start_vertex].z };
        vec3 v_end = { level->vertices[wall->end_vertex].x, wall->bottom_y, level->vertices[wall->end_vertex].z };
        RGL_DrawLine3D(v_start, v_end, 0.1f, (Color){255, 0, 0, 255});
        v_start[1] = wall->top_y;
        v_end[1] = wall->top_y;
        RGL_DrawLine3D(v_start, v_end, 0.1f, (Color){255, 0, 0, 255});
    }

    // Draw flat outlines
    for (size_t i = 0; i < level->flat_count; i++) {
        const RGLFlat* flat = &level->flats[i];
        for (size_t j = 0; j < flat->vertex_count; j++) {
            int v1_idx = flat->vertex_indices[j];
            int v2_idx = flat->vertex_indices[(j + 1) % flat->vertex_count];
            vec3 v1 = { level->vertices[v1_idx].x, flat->y, level->vertices[v1_idx].z };
            vec3 v2 = { level->vertices[v2_idx].x, flat->y, level->vertices[v2_idx].z };
            RGL_DrawLine3D(v1, v2, 0.1f, (Color){0, 255, 0, 255});
        }
    }

    // Draw thing markers
    for (size_t i = 0; i < level->thing_count; i++) {
        const RGLThing* thing = &level->things[i];
        vec3 min_b = { thing->x - 0.1f, thing->y - 0.1f, thing->z - 0.1f };
        vec3 max_b = { thing->x + 0.1f, thing->y + 0.1f, thing->z + 0.1f };
        RGL_DrawWireframeBounds(min_b, max_b, (Color){0, 0, 255, 255});
    }
}

/**
 * @brief Converts a 3D position from a level's local space to the global world space.
 * @param level_name The name of the level whose coordinate system is being used.
 * @param local_pos The 3D coordinate within the level's local space (as if its origin were at {0,0,0}).
 * @return The corresponding 3D coordinate in the global world space. Returns local_pos if level is not found.
 */
SITAPI vec3 RGL_LevelToWorld(const char* level_name, vec3 local_pos) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Level not found for LevelToWorld conversion.");
        return local_pos; // Return original position on failure
    }
    RGLLevel* level = &RGL.levels[index];

    // Build the level's transformation matrix
    mat4 transform;
    glm_mat4_identity(transform);
    glm_translate(transform, level->position);
    // Apply rotations in Yaw, Pitch, Roll order for intuitive results
    if (level->rotation_eul_deg[1] != 0.0f) glm_rotate_y(transform, glm_rad(level->rotation_eul_deg[1]), transform);
    if (level->rotation_eul_deg[0] != 0.0f) glm_rotate_x(transform, glm_rad(level->rotation_eul_deg[0]), transform);
    if (level->rotation_eul_deg[2] != 0.0f) glm_rotate_z(transform, glm_rad(level->rotation_eul_deg[2]), transform);
    
    // Apply the transformation
    vec3 world_pos;
    glm_mat4_mulv3(transform, local_pos, 1.0f, world_pos);
    return world_pos;
}

/**
 * @brief Converts a 3D position from the global world space to a level's local space.
 * @param level_name The name of the target level's coordinate system.
 * @param world_pos The 3D coordinate in the global world space.
 * @return The corresponding 3D coordinate in the level's local space. Returns world_pos if level is not found.
 */
SITAPI vec3 RGL_WorldToLevel(const char* level_name, vec3 world_pos) {
    int index = _RGL_FindLevelIndex(level_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Level not found for WorldToLevel conversion.");
        return world_pos; // Return original position on failure
    }
    RGLLevel* level = &RGL.levels[index];

    // Build the level's transformation matrix
    mat4 transform;
    glm_mat4_identity(transform);
    glm_translate(transform, level->position);
    if (level->rotation_eul_deg[1] != 0.0f) glm_rotate_y(transform, glm_rad(level->rotation_eul_deg[1]), transform);
    if (level->rotation_eul_deg[0] != 0.0f) glm_rotate_x(transform, glm_rad(level->rotation_eul_deg[0]), transform);
    if (level->rotation_eul_deg[2] != 0.0f) glm_rotate_z(transform, glm_rad(level->rotation_eul_deg[2]), transform);

    // To go from world to local, we use the *inverse* of the transformation matrix
    mat4 inv_transform;
    glm_mat4_inv(transform, inv_transform);

    // Apply the inverse transformation
    vec3 local_pos;
    glm_mat4_mulv3(inv_transform, world_pos, 1.0f, local_pos);
    return local_pos;
}

/**
 * @brief A utility function to easily place and orient a level relative to a point on a Path.
 * This automates the common workflow of placing a dungeon entrance along a path.
 *
 * @param level_name The name of the level to position.
 * @param path_name The name of the Path to use as a reference.
 * @param path_z The Z-position on the Path where the level's origin should be placed.
 * @param offset An {X, Y, Z} offset relative to the Path point. For example, {10, 0, 0} would place
 *               the level 10 units to the right of the Path's centerline.
 * @param yaw_offset_degrees An additional yaw rotation (in degrees) to apply to the level, relative to the Path's direction.
 *                           For example, 90.0 would make the level face perpendicular to the Path.
 * @return True on success, false if the level or Path is not found.
 */
SITAPI bool RGL_PlaceLevelOnPath(const char* level_name, const char* path_name, float path_z, vec3 offset, float yaw_offset_degrees) {
    // Find the level
    int level_idx = _RGL_FindLevelIndex(level_name);
    if (level_idx == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Level not found for placement.");
        return false;
    }
    
    // Temporarily set the active Path to get its properties
    int original_active_Path = RGL.active_Path_index;
    if (!RGL_SetActivePath(path_name)) {
        RGL.active_Path_index = original_active_Path; // Restore original
        return false; // RGL_SetActivePath already set the error
    }

    // Get the properties of the Path at the specified Z
    RGLPathPoint props;
    if (!RGL_GetPathPropertiesAt(path_z, &props)) {
        RGL.active_Path_index = original_active_Path; // Restore original
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Could not get Path properties at specified Z-position.");
        return false;
    }
    
    RGLLevel* level = &RGL.levels[level_idx];

    // --- Calculate Position ---
    // Start with the Path's centerline position
    vec3 base_pos = { props.world_x_offset, props.world_y_offset, props.world_z };
    
    // Apply the user's offset
    glm_vec3_add(base_pos, offset, level->position);

    // --- Calculate Rotation ---
    // To orient the level with the Path, we need the Path's direction (tangent).
    // We can approximate this by looking at a point slightly ahead on the path.
    RGLPathPoint props_ahead;
    if (RGL_GetPathPropertiesAt(path_z + 1.0f, &props_ahead)) {
        vec2 tangent = { props_ahead.world_x_offset - props.world_x_offset, 1.0f };
        glm_vec2_normalize(tangent);
        float Path_yaw_rads = atan2f(tangent[0], tangent[1]); // atan2(x, z) gives yaw
        
        // Combine Path yaw with user's offset yaw
        level->rotation_eul_deg[1] = glm_deg(Path_yaw_rads) + yaw_offset_degrees;
    } else {
        // Fallback if we're at the end of the path
        level->rotation_eul_deg[1] = yaw_offset_degrees;
    }

    // Restore the original active Path
    RGL.active_Path_index = original_active_Path;
    
    return true;
}

/**
 * @brief [Generic Core] Draws the active path using its currently assigned style.
 *
 * @section how_it_works How It Works (Dynamic Dispatch)
 *   This function is the heart of the flexible path rendering system. It does NOT contain
 *   any specific drawing logic for roads, rivers, etc. Instead, it performs a "dynamic dispatch"
 *   based on the style assigned to the active path.
 *
 *   1.  It retrieves the `RGLPathData` for the currently active path.
 *   2.  It looks at the `path->style` pointer. This pointer was set by `RGL_CreatePath` or
 *       `RGL_SetPathStyle`.
 *   3.  It then accesses the `draw_path_func` member of that style struct. This member is a
 *       **function pointer**—it holds the memory address of the actual drawing function
 *       (e.g., the address of `_RGL_DrawPathScene_Road`).
 *   4.  Finally, it calls the function at that memory address, passing the current player
 *       position and other necessary data.
 *
 *   This mechanism allows `RGL_DrawPath` to remain simple and generic, while users can
 *   provide entirely new visual styles for paths without ever modifying the library's code.
 *
 * @param player_z The current Z-position of the camera or player.
 * @param draw_distance The number of segments to draw into the distance.
 */
SITAPI void RGL_DrawPath(float player_z, int draw_distance) {
    RGLPathData* path = _RGL_GetActivePathData();
    if (!path) return;

    // 1. Look up the style "preset" that is currently "plugged in" to this path.
    const RGLPathStyle* style = path->style;
    if (!style || !style->draw_path_func) {
        // Safeguard: If no style is set, use the default road style to prevent a crash.
        style = RGL_GetDefaultRoadStyle();
    }

    // 2. Call the function pointer stored inside the style preset.
    // This is the dynamic dispatch. The code "jumps" to the address stored in
    // 'draw_path_func' and executes it. By default, this address points to
    // our internal _RGL_DrawPathScene_Road function.
    style->draw_path_func(player_z, draw_distance, style->user_data);
}


/**
 * @brief A high-level drawing function that renders the active Path and all active levels.
 * This is a convenience wrapper that calls RGL_DrawPathAsRoad() and then iterates through
 * all loaded levels, drawing each one. The game designer can use this for a simple,
 * one-call render loop, or call the underlying functions manually for more control.
 *
 * @param camera_z The camera's current Z position, used for Path rendering.
 * @param Path_draw_distance The number of Path segments to draw.
 */
SITAPI void RGL_DrawWorld(float camera_z, int Path_draw_distance) {
    // 1. Draw the active Path system, if one is set
    if (RGL.active_Path_index != -1) {
        RGL_DrawPathAsRoad(camera_z, Path_draw_distance);
    }
    
    // 2. Draw all loaded levels.
    // The game logic should handle which levels are "active" for gameplay,
    // but for rendering, we can simply draw them all. Culling will hide
    // levels that are not in the camera's view.
    for (size_t i = 0; i < RGL.level_count; i++) {
        // Temporarily set the active level to draw it
        int original_active_level = RGL.active_level_index;
        RGL.active_level_index = i;
        RGL_DrawLevel();
        RGL.active_level_index = original_active_level;
    }
}

/**
 * @brief Draws a horizontally scrolling panoramic background.
 *
 * This function should be called at the beginning of a frame, after RGL_Begin() but
 * before any other drawing. It's ideal for creating infinite skies or distant landscapes.
 * It draws immediately and does not participate in the main render batch.
 *
 * @param texture The panoramic texture to draw. Should be horizontally tileable.
 * @param scroll_offset_x The horizontal scroll position, from 0.0 (left edge) to 1.0 (right edge).
 * @param y_offset_pct The vertical offset of the texture, as a percentage of its height.
 * @param height_scale The vertical scaling of the texture. 1.0 is normal size.
 * @param tint A color to tint the entire panorama.
 */
SITAPI void RGL_DrawPanoramaBackground(RGLTexture texture, float scroll_offset_x, float y_offset_pct, float height_scale, Color tint) {
    if (!RGL.is_initialized || !RGL.is_batching) return;

    _RGL_FlushBatch();

    glDisable(GL_DEPTH_TEST);
    glUseProgram(RGL.main_shader);
    glUniform1i(RGL.loc_texture_sampler, 0);
    glUniform1i(RGL.loc_use_texture, texture.id != 0);
    
    mat4 ortho_proj, identity_view;
    int width, height;
    SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &width, &height);
    glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, ortho_proj);
    glm_mat4_identity(identity_view);
    
    glUniformMatrix4fv(RGL.loc_projection, 1, GL_FALSE, (const GLfloat*)ortho_proj);
    glUniformMatrix4fv(RGL.loc_view, 1, GL_FALSE, (const GLfloat*)identity_view);

    vec4 norm_color;
    SituationConvertColorToVec4(tint, norm_color);
    float u_width = (float)width / (float)texture.width;

    // BUGFIX: Vertex format is 10 floats.
    float vertices[] = {
        // pos.xyz, uv.xy, color.rgba, light
        // Triangle 1
        0.0f,      0.0f,       0.0f,  scroll_offset_x,           y_offset_pct,                      norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f,
        0.0f,      (float)height, 0.0f,  scroll_offset_x,           y_offset_pct + height_scale,       norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f,
        (float)width, 0.0f,       0.0f,  scroll_offset_x + u_width, y_offset_pct,                      norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f,
        // Triangle 2
        0.0f,      (float)height, 0.0f,  scroll_offset_x,           y_offset_pct + height_scale,       norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f,
        (float)width, (float)height, 0.0f,  scroll_offset_x + u_width, y_offset_pct + height_scale,       norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f,
        (float)width, 0.0f,       0.0f,  scroll_offset_x + u_width, y_offset_pct,                      norm_color[0], norm_color[1], norm_color[2], norm_color[3], 1.0f
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    
    glBindVertexArray(RGL.batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

/**
 * @brief Draws a single pixel at a specified 2D position.
 * @note This is implemented as a 1x1 rectangle and is batched accordingly.
 * @param position The (x, y) coordinate of the pixel.
 * @param color The color of the pixel.
 */
SITAPI void RGL_DrawPixel(vec2 position, Color color) {
    if (!RGL.is_initialized || !RGL.is_batching) return;

    // A pixel is an untextured 1x1 rectangle with no rotation.
    RGL_DrawTexturePro((RGLSprite){0}, (Rectangle){position[0], position[1], 1.0f, 1.0f}, (vec2){0.0f, 0.0f}, 0.0f, color);
}

/**
 * @brief Draws a color-filled circle.
 * @note The circle is rendered as a high-segment polygon.
 * @param center The center position (x, y) of the circle.
 * @param radius The radius of the circle in pixels.
 * @param color The fill color of the circle.
 */
SITAPI void RGL_DrawCircle(vec2 center, float radius, Color color) {
    if (!RGL.is_batching) return;
    if (radius <= 0) return;

    vec2 points[RGL_SHAPE_SEGMENTS];
    for (int i = 0; i < RGL_SHAPE_SEGMENTS; i++) {
        float angle = 2.0f * M_PI * (float)i / (float)RGL_SHAPE_SEGMENTS;
        points[i][0] = center[0] + cosf(angle) * radius;
        points[i][1] = center[1] + sinf(angle) * radius;
    }
    RGL_DrawPolygonScreen(points, RGL_SHAPE_SEGMENTS, color);
}

/**
 * @brief Draws the outline of a circle.
 * @param center The center position (x, y) of the circle.
 * @param radius The outer radius of the circle.
 * @param thickness The thickness of the outline in pixels.
 * @param color The color of the outline.
 */
SITAPI void RGL_DrawCircleOutline(vec2 center, float radius, float thickness, Color color) {
    if (!RGL.is_batching) return;
    if (radius <= 0 || thickness <= 0) return;

    float inner_radius = radius - thickness;
    if (inner_radius < 0) inner_radius = 0;

    // We create a strip of quads.
    for (int i = 0; i <= RGL_SHAPE_SEGMENTS; i++) {
        float angle1 = 2.0f * M_PI * (float)i / (float)RGL_SHAPE_SEGMENTS;
        float angle2 = 2.0f * M_PI * (float)(i + 1) / (float)RGL_SHAPE_SEGMENTS;
        
        vec2 quad_points[4];
        // Outer point 1
        quad_points[0][0] = center[0] + cosf(angle1) * radius;
        quad_points[0][1] = center[1] + sinf(angle1) * radius;
        // Inner point 1
        quad_points[1][0] = center[0] + cosf(angle1) * inner_radius;
        quad_points[1][1] = center[1] + sinf(angle1) * inner_radius;
        // Inner point 2
        quad_points[2][0] = center[0] + cosf(angle2) * inner_radius;
        quad_points[2][1] = center[1] + sinf(angle2) * inner_radius;
        // Outer point 2
        quad_points[3][0] = center[0] + cosf(angle2) * radius;
        quad_points[3][1] = center[1] + sinf(angle2) * radius;

        RGL_DrawPolygonScreen(quad_points, 4, color);
    }
}

/**
 * @brief Draws a color-filled circle using a YPQ color.
 * @note Ideal for achieving CRT/TV-like aesthetics. The YPQ color is converted to RGB before rendering.
 * @param center The center position (x, y) of the circle.
 * @param radius The radius of the circle in pixels.
 * @param color The fill color, specified in the YPQ color space.
 * @see RGL_DrawCircle, ColorYPQA
 */
SITAPI void RGL_DrawCircleYPQ(vec2 center, float radius, ColorYPQA color) {
    if (!RGL.is_batching) return;
    RGL_DrawCircle(center, radius, SituationColorFromYPQ(color));
}

/**
 * @brief Draws a color-filled ellipse.
 * @param center The center position (x, y) of the ellipse.
 * @param radii A vec2 containing the horizontal (x) and vertical (y) radii of the ellipse.
 * @param color The fill color of the ellipse.
 */
SITAPI void RGL_DrawEllipse(vec2 center, vec2 radii, Color color) {
    if (!RGL.is_batching) return;
    if (radii[0] <= 0 || radii[1] <= 0) return;

    vec2 points[RGL_SHAPE_SEGMENTS];
    for (int i = 0; i < RGL_SHAPE_SEGMENTS; i++) {
        float angle = 2.0f * M_PI * (float)i / (float)RGL_SHAPE_SEGMENTS;
        points[i][0] = center[0] + cosf(angle) * radii[0];
        points[i][1] = center[1] + sinf(angle) * radii[1];
    }
    RGL_DrawPolygonScreen(points, RGL_SHAPE_SEGMENTS, color);
}

/**
 * @brief Draws a color-filled arc or pie-slice shape.
 * @param center The center point of the arc's circle.
 * @param radius The radius of the arc.
 * @param start_angle The starting angle of the arc in degrees (0 is right, 90 is down).
 * @param end_angle The ending angle of the arc in degrees.
 * @param color The fill color of the arc.
 */
SITAPI void RGL_DrawArc(vec2 center, float radius, float start_angle, float end_angle, Color color) {
    if (!RGL.is_batching) return;
    if (radius <= 0) return;

    // Use a triangle fan
    int num_segments = (int)(RGL_SHAPE_SEGMENTS * fabsf(end_angle - start_angle) / 360.0f);
    if (num_segments < 2) num_segments = 2;

    vec2 points[num_segments + 1];
    points[0][0] = center[0];
    points[0][1] = center[1];

    for (int i = 0; i < num_segments; i++) {
        float current_angle_rad = (start_angle + (end_angle - start_angle) * (float)i / (float)(num_segments - 1)) * (M_PI / 180.0f);
        points[i + 1][0] = center[0] + cosf(current_angle_rad) * radius;
        points[i + 1][1] = center[1] + sinf(current_angle_rad) * radius;
    }
    RGL_DrawPolygonScreen(points, num_segments + 1, color);
}

/**
 * @brief Draws a color-filled ring shape.
 * @param center The center of the ring.
 * @param inner_radius The radius of the inner empty circle.
 * @param outer_radius The radius of the outer edge of the ring.
 * @param color The fill color of the ring.
 * @see RGL_DrawCircleOutline
 */
SITAPI void RGL_DrawRing(vec2 center, float inner_radius, float outer_radius, Color color) {
    if (!RGL.is_batching) return;
    // This is just a convenience wrapper around DrawCircleOutline
    RGL_DrawCircleOutline(center, outer_radius, outer_radius - inner_radius, color);
}

// --- Lines & Paths ---

/**
 * @brief Draws a simple line between two points.
 * @note This is a convenience wrapper for RGL_DrawLineEx with a thickness of 1.0f.
 * @param start The starting position (x, y) of the line.
 * @param end The ending position (x, y) of the line.
 * @param color The color of the line.
 */
SITAPI void RGL_DrawLine(vec2 start, vec2 end, Color color) {
    if (!RGL.is_batching) return;
    RGL_DrawLineEx(start, end, 1.0f, color);
}

/**
 * @brief Draws a cubic Bezier curve.
 * @param start The starting point of the curve.
 * @param end The ending point of the curve.
 * @param control1 The first control point.
 * @param control2 The second control point.
 * @param thickness The thickness of the curve's line.
 * @param color The color of the curve.
 */
SITAPI void RGL_DrawLineBezier(vec2 start, vec2 end, vec2 control1, vec2 control2, float thickness, Color color) {
    if (!RGL.is_batching) return;
    
    vec2 points[RGL_SHAPE_SEGMENTS + 1];
    for (int i = 0; i <= RGL_SHAPE_SEGMENTS; i++) {
        float t = (float)i / (float)RGL_SHAPE_SEGMENTS;
        float u = 1.0f - t;
        float tt = t*t;
        float uu = u*u;
        float uuu = uu * u;
        float ttt = tt * t;

        float b0 = uuu;
        float b1 = 3.0f * uu * t;
        float b2 = 3.0f * u * tt;
        float b3 = ttt;

        points[i][0] = (b0 * start[0]) + (b1 * control1[0]) + (b2 * control2[0]) + (b3 * end[0]);
        points[i][1] = (b0 * start[1]) + (b1 * control1[1]) + (b2 * control2[1]) + (b3 * end[1]);
    }
    RGL_DrawPolyline(points, RGL_SHAPE_SEGMENTS + 1, thickness, color, false);
}

/**
 * @brief Draws a series of connected lines from a set of points.
 * @param points An array of vec2 points.
 * @param point_count The number of points in the array.
 * @param thickness The thickness of the lines.
 * @param color The color of the lines.
 * @param closed If true, a line will be drawn from the last point to the first.
 */
SITAPI void RGL_DrawPolyline(vec2* points, int point_count, float thickness, Color color, bool closed) {
    if (!RGL.is_batching || point_count < 2) return;
    
    for (int i = 0; i < point_count - 1; i++) {
        RGL_DrawLineEx(points[i], points[i+1], thickness, color);
    }
    if (closed && point_count > 2) {
        RGL_DrawLineEx(points[point_count-1], points[0], thickness, color);
    }
}

/**
 * @brief Draws a grid of lines.
 * @param spacing A vec2 defining the width and height of each grid cell.
 * @param offset A vec2 defining the (x, y) offset of the grid.
 * @param color The color of the grid lines.
 */
SITAPI void RGL_DrawGrid(vec2 spacing, vec2 offset, Color color) {
    if (!RGL.is_batching) return;
    Rectangle screen = RGL_GetScreenRect();

    // Vertical lines
    for (float x = fmodf(offset[0], spacing[0]); x < screen.width; x += spacing[0]) {
        RGL_DrawLine((vec2){x, 0}, (vec2){x, screen.height}, color);
    }
    // Horizontal lines
    for (float y = fmodf(offset[1], spacing[1]); y < screen.height; y += spacing[1]) {
        RGL_DrawLine((vec2){0, y}, (vec2){screen.width, y}, color);
    }
}

/**
 * @brief Draws a ruler with tick marks, useful for calibration and measurement.
 * @param start The starting point of the ruler's main line.
 * @param end The ending point of the ruler's main line.
 * @param tick_spacing The distance between each tick mark along the ruler.
 * @param tick_length The length of each tick mark, perpendicular to the main line.
 * @param color The color of the ruler and its ticks.
 */
SITAPI void RGL_DrawRuler(vec2 start, vec2 end, float tick_spacing, float tick_length, Color color) {
    if (!RGL.is_batching) return;
    
    RGL_DrawLineEx(start, end, 1.0f, color);

    vec2 delta = { end[0] - start[0], end[1] - start[1] };
    float length = sqrtf(delta[0]*delta[0] + delta[1]*delta[1]);
    if (length < 0.001f) return;

    vec2 dir = { delta[0]/length, delta[1]/length };
    vec2 perp = { -dir[1], dir[0] };
    
    int num_ticks = length / tick_spacing;
    for (int i = 0; i <= num_ticks; i++) {
        vec2 tick_start = { start[0] + dir[0] * i * tick_spacing, start[1] + dir[1] * i * tick_spacing };
        vec2 tick_end = { tick_start[0] + perp[0] * tick_length, tick_start[1] + perp[1] * tick_length };
        RGL_DrawLine(tick_start, tick_end, color);
    }
}

// --- Rectangles & Rounded Shapes ---

/**
 * @brief Draws the outline of a rectangle.
 * @param rect The rectangle to draw the outline for.
 * @param thickness The thickness of the outline in pixels.
 * @param color The color of the outline.
 */
SITAPI void RGL_DrawRectangleOutline(Rectangle rect, float thickness, Color color) {
    if (!RGL.is_batching) return;
    vec2 tl = {rect.x, rect.y};
    vec2 tr = {rect.x + rect.width, rect.y};
    vec2 bl = {rect.x, rect.y + rect.height};
    vec2 br = {rect.x + rect.width, rect.y + rect.height};

    RGL_DrawLineEx(tl, tr, thickness, color);
    RGL_DrawLineEx(tr, br, thickness, color);
    RGL_DrawLineEx(br, bl, thickness, color);
    RGL_DrawLineEx(bl, tl, thickness, color);
}

/**
 * @brief Draws a color-filled rectangle with rounded corners.
 * @param rect The outer bounds of the rectangle.
 * @param roundness The radius of the corners. Will be clamped to half the rectangle's smaller dimension.
 * @param color The fill color of the rectangle.
 */
SITAPI void RGL_DrawRectangleRounded(Rectangle rect, float roundness, Color color) {
    if (!RGL.is_batching) return;
    if (roundness <= 0) {
        RGL_DrawRectangle(rect, 0.0f, color);
        return;
    }

    float r = roundness;
    if (r > rect.width/2.0f) r = rect.width/2.0f;
    if (r > rect.height/2.0f) r = rect.height/2.0f;

    // Center rectangle
    RGL_DrawRectangle((Rectangle){rect.x + r, rect.y, rect.width - 2*r, rect.height}, 0.0f, color);
    // Left and right rectangles
    RGL_DrawRectangle((Rectangle){rect.x, rect.y + r, r, rect.height - 2*r}, 0.0f, color);
    RGL_DrawRectangle((Rectangle){rect.x + rect.width - r, rect.y + r, r, rect.height - 2*r}, 0.0f, color);

    // Draw corner arcs
    RGL_DrawArc((vec2){rect.x + r, rect.y + r}, r, 180, 270, color); // Top-left
    RGL_DrawArc((vec2){rect.x + rect.width - r, rect.y + r}, r, 270, 360, color); // Top-right
    RGL_DrawArc((vec2){rect.x + r, rect.y + rect.height - r}, r, 90, 180, color); // Bottom-left
    RGL_DrawArc((vec2){rect.x + rect.width - r, rect.y + rect.height - r}, r, 0, 90, color); // Bottom-right
}

/**
 * @brief Draws the outline of a rectangle with rounded corners.
 * @param rect The outer bounds of the rectangle.
 * @param roundness The radius of the corners.
 * @param thickness The thickness of the outline.
 * @param color The color of the outline.
 */
SITAPI void RGL_DrawRectangleRoundedOutline(Rectangle rect, float roundness, float thickness, Color color) {
    // This can be complex. A simple approximation is to draw rounded Paths.
    if (!RGL.is_batching) return;
    float r = roundness;
    if (r > rect.width/2.0f) r = rect.width/2.0f;
    if (r > rect.height/2.0f) r = rect.height/2.0f;

    // Draw straight parts
    RGL_DrawLineEx((vec2){rect.x + r, rect.y}, (vec2){rect.x + rect.width - r, rect.y}, thickness, color);
    RGL_DrawLineEx((vec2){rect.x + r, rect.y + rect.height}, (vec2){rect.x + rect.width - r, rect.y + rect.height}, thickness, color);
    RGL_DrawLineEx((vec2){rect.x, rect.y + r}, (vec2){rect.x, rect.y + rect.height - r}, thickness, color);
    RGL_DrawLineEx((vec2){rect.x + rect.width, rect.y + r}, (vec2){rect.x + rect.width, rect.y + rect.height - r}, thickness, color);

    // Draw corner arcs (as polylines)
    int num_segments = RGL_SHAPE_SEGMENTS / 4;
    vec2 points[num_segments + 1];
    
    // Top-left
    for(int i=0; i <= num_segments; i++) {
        float angle = 180.0f + 90.0f * (float)i / (float)num_segments;
        points[i][0] = (rect.x + r) + cosf(angle * M_PI/180.0f) * r;
        points[i][1] = (rect.y + r) + sinf(angle * M_PI/180.0f) * r;
    }
    RGL_DrawPolyline(points, num_segments+1, thickness, color, false);
    
    // Top-right
    for(int i=0; i <= num_segments; i++) {
        float angle = 270.0f + 90.0f * (float)i / (float)num_segments;
        points[i][0] = (rect.x + rect.width - r) + cosf(angle * M_PI/180.0f) * r;
        points[i][1] = (rect.y + r) + sinf(angle * M_PI/180.0f) * r;
    }
    RGL_DrawPolyline(points, num_segments+1, thickness, color, false);

    // Bottom-left
    for(int i=0; i <= num_segments; i++) {
        float angle = 90.0f + 90.0f * (float)i / (float)num_segments;
        points[i][0] = (rect.x + r) + cosf(angle * M_PI/180.0f) * r;
        points[i][1] = (rect.y + rect.height - r) + sinf(angle * M_PI/180.0f) * r;
    }
    RGL_DrawPolyline(points, num_segments+1, thickness, color, false);

    // Bottom-right
    for(int i=0; i <= num_segments; i++) {
        float angle = 0.0f + 90.0f * (float)i / (float)num_segments;
        points[i][0] = (rect.x + rect.width - r) + cosf(angle * M_PI/180.0f) * r;
        points[i][1] = (rect.y + rect.height - r) + sinf(angle * M_PI/180.0f) * r;
    }
    RGL_DrawPolyline(points, num_segments+1, thickness, color, false);
}

/**
 * @brief Draws a rectangle with a smooth color gradient across its vertices.
 * @param rect The rectangle's position and size.
 * @param top_left The color of the top-left corner.
 * @param top_right The color of the top-right corner.
 * @param bottom_left The color of the bottom-left corner.
 * @param bottom_right The color of the bottom-right corner.
 */
SITAPI void RGL_DrawRectangleGradient(Rectangle rect, Color top_left, Color top_right, Color bottom_left, Color bottom_right) {
    if (!RGL.is_batching) return;
    
    Color colors[4] = { top_left, top_right, bottom_right, bottom_left };
    float light_levels[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // For a simple 2D rectangle, we can use DrawQuadPro with a zero rotation and Z position.
    // The texture will be null (id=0), and the batcher will handle it.
    RGL_DrawQuadPro((RGLTexture){0}, (Rectangle){0,0,1,1}, 
                    (vec3){rect.x, rect.y, 0.0f}, (vec2){rect.width, rect.height}, 
                    (vec2){0.0f, 0.0f}, (vec3){0,0,0}, (vec2){0,0}, 
                    colors, light_levels);
}

/**
 * @brief Draws a color-filled rectangle using a YPQ color.
 * @note Ideal for achieving CRT/TV-like aesthetics.
 * @param rect The rectangle's position and size.
 * @param color The fill color, specified in the YPQ color space.
 * @see RGL_DrawRectangle, ColorYPQA
 */
SITAPI void RGL_DrawRectangleYPQ(Rectangle rect, ColorYPQA color) {
    if (!RGL.is_batching) return;
    RGL_DrawRectangle(rect, 0.0f, SituationColorFromYPQ(color));
}

// --- Pattern Fills ---

/**
 * @brief Fills a rectangular area with a checkerboard pattern.
 * @note Useful for resolution and scaling tests.
 * @param rect The area to fill.
 * @param tile_size A vec2 defining the width and height of each checkerboard square.
 * @param color1 The first color of the pattern.
 * @param color2 The second color of the pattern.
 */
SITAPI void RGL_DrawCheckerboard(Rectangle rect, vec2 tile_size, Color color1, Color color2) {
    if (!RGL.is_batching) return;
    if (tile_size[0] <= 0 || tile_size[1] <= 0) return;

    int cols = (int)ceilf(rect.width / tile_size[0]);
    int rows = (int)ceilf(rect.height / tile_size[1]);

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Rectangle tile_rect = { rect.x + x * tile_size[0], rect.y + y * tile_size[1], tile_size[0], tile_size[1] };
            // Clamp tile to bounds of the main rectangle
            if (tile_rect.x + tile_rect.width > rect.x + rect.width) tile_rect.width = (rect.x + rect.width) - tile_rect.x;
            if (tile_rect.y + tile_rect.height > rect.y + rect.height) tile_rect.height = (rect.y + rect.height) - tile_rect.y;
            if (tile_rect.width > 0 && tile_rect.height > 0) {
                 RGL_DrawRectangle(tile_rect, 0.0f, ((x + y) % 2 == 0) ? color1 : color2);
            }
        }
    }
}

/**
 * @brief Fills a rectangular area with stripes.
 * @note Useful for convergence and moire pattern tests.
 * @param rect The area to fill.
 * @param stripe_width The width (or height) of each stripe.
 * @param vertical If true, stripes are vertical; otherwise, they are horizontal.
 * @param color1 The first color of the pattern.
 * @param color2 The second color of the pattern.
 */
SITAPI void RGL_DrawStripes(Rectangle rect, float stripe_width, bool vertical, Color color1, Color color2) {
    if (!RGL.is_batching) return;
    if (stripe_width <= 0) return;

    if (vertical) {
        int num_stripes = (int)ceilf(rect.width / stripe_width);
        for (int i = 0; i < num_stripes; i++) {
            Rectangle stripe_rect = { rect.x + i * stripe_width, rect.y, stripe_width, rect.height };
            if (stripe_rect.x + stripe_rect.width > rect.x + rect.width)    stripe_rect.width = (rect.x + rect.width) - stripe_rect.x;
            if (stripe_rect.width > 0)                                      RGL_DrawRectangle(stripe_rect, 0.0f, (i % 2 == 0) ? color1 : color2);
        }
    } else { // Horizontal
        int num_stripes = (int)ceilf(rect.height / stripe_width);
        for (int i = 0; i < num_stripes; i++) {
            Rectangle stripe_rect = { rect.x, rect.y + i * stripe_width, rect.width, stripe_width };
            if (stripe_rect.y + stripe_rect.height > rect.y + rect.height)  stripe_rect.height = (rect.y + rect.height) - stripe_rect.y;
            if (stripe_rect.height > 0)                                     RGL_DrawRectangle(stripe_rect, 0.0f, (i % 2 == 0) ? color1 : color2);
        }
    }
}

// --- Calibration Aids ---

/**
 * @brief Draws an outline representing the TV-safe area for a given screen size.
 * @param screen The full rectangle of the screen or viewport.
 * @param overscan_pct The percentage of the screen to consider as overscan margin (e.g., 0.05 for 5%).
 * @param color The color of the safe area outline.
 */
SITAPI void RGL_DrawSafeArea(Rectangle screen, float overscan_pct, Color color) {
    if (!RGL.is_batching) return;
    float margin_x = screen.width * overscan_pct;
    float margin_y = screen.height * overscan_pct;
    Rectangle safe_area = { screen.x + margin_x, screen.y + margin_y, screen.width - 2 * margin_x, screen.height - 2 * margin_y };
    RGL_DrawRectangleOutline(safe_area, 1.0f, color);
}

/**
 * @brief Draws a crosshair marker.
 * @param center The center point of the crosshair.
 * @param size The total width and height of the crosshair.
 * @param thickness The thickness of the crosshair lines.
 * @param color The color of the crosshair.
 */
SITAPI void RGL_DrawCrosshair(vec2 center, float size, float thickness, Color color) {
    if (!RGL.is_batching) return;
    float half_size = size / 2.0f;
    RGL_DrawLineEx((vec2){center[0] - half_size, center[1]}, (vec2){center[0] + half_size, center[1]}, thickness, color);
    RGL_DrawLineEx((vec2){center[0], center[1] - half_size}, (vec2){center[0], center[1] + half_size}, thickness, color);
}

/**
 * @brief Draws a line with an arrowhead at the end.
 * @param start The starting point of the arrow's shaft.
 * @param end The ending point of the arrow's shaft (where the head will be).
 * @param head_size The length of the arrowhead's sides.
 * @param thickness The thickness of the arrow's lines.
 * @param color The color of the arrow.
 */
SITAPI void RGL_DrawArrow(vec2 start, vec2 end, float head_size, float thickness, Color color) {
    if (!RGL.is_batching) return;
    
    RGL_DrawLineEx(start, end, thickness, color);
    vec2 delta = { end[0] - start[0], end[1] - start[1] };
    float length = sqrtf(delta[0]*delta[0] + delta[1]*delta[1]);
    if (length < 0.001f) return;
    vec2 dir = { delta[0]/length, delta[1]/length };
    
    // Arrow head points
    vec2 p1 = { end[0] - dir[0] * head_size + dir[1] * head_size, end[1] - dir[1] * head_size - dir[0] * head_size };
    vec2 p2 = { end[0] - dir[0] * head_size - dir[1] * head_size, end[1] - dir[1] * head_size + dir[0] * head_size };
    RGL_DrawLineEx(end, p1, thickness, color);
    RGL_DrawLineEx(end, p2, thickness, color);
}

// --- Utility Drawing ---

/**
 * @brief Draws a filled rectangle with a text label centered inside.
 * @note Useful for creating labeled buttons or test patterns.
 * @param rect The rectangle to draw.
 * @param label The text to display inside the rectangle.
 * @param font The bitmap font to use for the label.
 * @param rect_color The fill color of the rectangle.
 * @param text_color The color of the text label.
 */
SITAPI void RGL_DrawLabeledRectangle(Rectangle rect, const char* label, RGLBitmapFont font, Color rect_color, Color text_color) {
    if (!RGL.is_batching) return;

    RGL_DrawRectangle(rect, 0.0f, rect_color);
    RGL_DrawRectangleOutline(rect, 1.0f, RGL_ColorBrightness(rect_color, -0.5f));

    vec2 text_size = RGL_MeasureText(label, font);
    vec2 text_pos = {
        rect.x + (rect.width - text_size[0]) / 2.0f,
        rect.y + (rect.height - text_size[1]) / 2.0f
    };
    RGL_DrawText(label, text_pos, font, text_color);
}

SITAPI void RGL_RegisterSceneryStyle(RGLSceneryType type, const RGLSceneryStyle* style) {
    if (type < RGL_MAX_SCENERY_TYPES) {
        // We cast away const here internally, which is safe because we own the global array.
        g_scenery_styles[type] = (RGLSceneryStyle*)style;
    }
}

/**
 * @brief Finds all scenery objects on the active Path within a given Z-axis range.
 * @param start_z The starting Z-coordinate of the search range.
 * @param end_z The ending Z-coordinate of the search range.
 * @param out_scenery An array of pointers to RGLScenery structs to store the results.
 * @param max_scenery The maximum number of pointers the `out_scenery` array can hold.
 * @return The number of scenery objects found and written to the output array.
 */
SITAPI int RGL_FindSceneryInRange(float start_z, float end_z, RGLScenery* out_scenery[], int max_scenery) {
    // 1. --- Input Validation (Sanity Checks) ---
    if (!out_scenery || max_scenery <= 0 || start_z >= end_z) {
        return 0;
    }

    // 2. --- Get Active Path Context ---
    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points == 0) {
        return 0;
    }

    // 3. --- Efficiently Find Starting Point ---
    // Uses the O(log n) binary search helper for excellent performance.
    int start_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, start_z);
    if (start_idx == -1) {
        return 0; // The entire path is before the start_z.
    }

    // 4. --- Linear Scan Through Relevant Segment ---
    int found_count = 0;
    for (size_t i = start_idx; i < Path->num_points; i++) {
        RGLPathPoint* p = &Path->points[i];
        
        // Stop scanning once we've passed the end of the range.
        if (p->world_z > end_z) {
            break;
        }

        // Check all three scenery slots for this path point.
        if (p->scenery_left.type != RGL_SCENERY_NONE) {
            out_scenery[found_count++] = &p->scenery_left;
            if (found_count >= max_scenery) return found_count; // Buffer full, exit early.
        }
        if (p->scenery_right.type != RGL_SCENERY_NONE) {
            out_scenery[found_count++] = &p->scenery_right;
            if (found_count >= max_scenery) return found_count;
        }
        if (p->scenery_overhead.type != RGL_SCENERY_NONE) {
            out_scenery[found_count++] = &p->scenery_overhead;
            if (found_count >= max_scenery) return found_count;
        }
    }
    
    return found_count;
}

/**
 * @brief Finds all scenery objects on the active Path within a 3D spherical radius.
 * @param world_pos The 3D world-space center of the search sphere.
 * @param radius The radius of the search sphere.
 * @param out_scenery An array of pointers to RGLScenery structs to store the results.
 * @param max_scenery The maximum number of pointers the `out_scenery` array can hold.
 * @return The number of scenery objects found and written to the output array.
 */
SITAPI int RGL_FindSceneryInRadius(vec3 world_pos, float radius, RGLScenery* out_objects[], int max_objects) {
    if (!out_objects || max_objects <= 0 || radius <= 0) return 0;

    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points == 0) return 0;

    int found_count = 0;
    float radius_sq = radius * radius;
    float min_z = world_pos[2] - radius;
    float max_z = world_pos[2] + radius;

    int start_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, min_z);
    if (start_idx == -1) start_idx = 0;

    for (int i = start_idx; i < Path->num_points; i++) {
        RGLPathPoint* p = &Path->points[i];
        if (p->world_z > max_z) break;

        RGLScenery* scenery_slots[] = { &p->scenery_left, &p->scenery_right, &p->scenery_overhead };

        for (int j = 0; j < 3; j++) {
            RGLScenery* s = scenery_slots[j];
            if (s->type == RGL_SCENERY_NONE) continue;

            // CORRECTED, SUPERIOR LOGIC:
            vec3 scenery_pos;
            scenery_pos[0] = p->world_x_offset + s->x_offset * (p->primary_ribbon_width * 0.5f);
            scenery_pos[1] = p->world_y_offset + s->y_offset;
            scenery_pos[2] = p->world_z;

            if (glm_vec3_distance2(world_pos, scenery_pos) < radius_sq) {
                out_objects[found_count++] = s;
                if (found_count >= max_objects) {
                    return found_count;
                }
            }
        }
    }
    
    return found_count;
}

/**
 * @brief Creates a new, empty Path ribbon.
 * @param path_name A unique name for the new Path (e.g., "MainStreet", "RacePath").
 * @return True on success, false if the name is invalid or already exists.
 */
SITAPI bool RGL_CreatePath(const char* path_name) {
    if (!RGL.is_initialized || !path_name || strlen(path_name) == 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Path name cannot be null or empty.");
        return false;
    }
    if (_RGL_FindPathIndex(path_name) != -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "A Path with that name already exists.");
        return false;
    }

    if (RGL.Path_count >= RGL.Path_capacity) {
        size_t new_capacity = (RGL.Path_capacity == 0) ? 4 : RGL.Path_capacity * 2;
        RGLNamedPath* new_Paths = realloc(RGL.Paths, sizeof(RGLNamedPath) * new_capacity);
        if (!new_Paths) {
            _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to reallocate Paths array.");
            return false;
        }
        RGL.Paths = new_Paths;
        RGL.Path_capacity = new_capacity;
    }

    RGLNamedPath* new_path_entry = &RGL.Paths[RGL.Path_count];
    strncpy(new_path_entry->name, path_name, 31);
    new_path_entry->name[31] = '\0';
    memset(&new_path_entry->data, 0, sizeof(RGLPathData));
    new_path_entry->data.loop_to_z = -1.0f;
    new_path_entry->data.style = RGL_GetDefaultRoadStyle(); // Assign default style on creation

    RGL.Path_count++;

    if (RGL.Path_count == 1) {
        RGL.active_Path_index = 0;
    }
    return true;
}

/**
 * @brief Sets the Z-position for a named path to loop back to.
 * @param path_name The name of the Path to modify.
 * @param z_pos The Z-position to loop back to (e.g., 0.0f). Set to -1.0f to disable looping.
 * @return True on success, false if the Path is not found.
 */
SITAPI bool RGL_SetPathLooping(const char* path_name, float z_pos) {
    int index = _RGL_FindPathIndex(path_name);
    if (index == -1) return false;
    RGL.Paths[index].data.loop_to_z = z_pos;
    return true;
}

/**
 * @brief Destroys a specific named Path and frees its associated memory.
 * If the Path being destroyed is currently active, the active Path index will be
 * reset, and no Path will be active until RGL_SetActivePath is called again.
 *
 * @param path_name The name of the Path to destroy.
 * @return True on success, false if no Path with that name was found.
 */
SITAPI bool RGL_DestroyPathByName(const char* path_name) {
    int index = _RGL_FindPathIndex(path_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Cannot destroy Path: not found.");
        return false;
    }

    free(RGL.Paths[index].data.points);

    if (RGL.active_Path_index == index) {
        RGL.active_Path_index = -1;
    } 
    else if (RGL.active_Path_index > index) {
        RGL.active_Path_index--;
    }

    for (size_t i = index; i < RGL.Path_count - 1; i++) {
        RGL.Paths[i] = RGL.Paths[i + 1];
    }

    RGL.Path_count--;
    return true;
}

/**
 * @brief Sets the currently active Path for all drawing and query functions.
 * @param path_name The name of the Path to make active.
 * @return True on success, false if no Path with that name is found.
 */
SITAPI bool RGL_SetActivePath(const char* path_name) {
    int index = _RGL_FindPathIndex(path_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "No Path with the specified name was found.");
        return false;
    }
    RGL.active_Path_index = index;
    if (RGL.Paths) {
        RGL.Paths[index].data.last_segment_index_cache = 0;
    }
    return true;
}

/**
 * @brief [Helper] Returns a pointer to the default, built-in style for rendering paths as roads.
 */
SITAPI const RGLPathStyle* RGL_GetDefaultRoadStyle(void) {
    return &RGL_DEFAULT_ROAD_STYLE;
}

/**
 * @brief Assigns a custom visual style to a named path.
 * After calling this, any subsequent calls to RGL_DrawPath (or wrappers like RGL_DrawPathAsRoad)
 * for the active path will use the new style.
 *
 * @param path_name The name of the path to modify.
 * @param style A pointer to the RGLPathStyle struct that defines the new look.
 * @return True on success, false if the path name is not found.
 */
SITAPI bool RGL_SetPathStyle(const char* path_name, const RGLPathStyle* style) {
    int index = _RGL_FindPathIndex(path_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Cannot set style: Path not found.");
        return false;
    }
    // This is where we "plug in" a new style to a path's data.
    RGL.Paths[index].data.style = (style != NULL) ? style : RGL_GetDefaultRoadStyle();
    return true;
}

/**
 * @brief Adds a control point to the end of a specified named Path.
 * @param path_name The name of the Path to add the point to.
 * @param point The RGLPathPoint to add. Must have a world_z greater than the previous point.
 */
SITAPI void RGL_AddPathPoint(const char* path_name, RGLPathPoint point) {
    int index = _RGL_FindPathIndex(path_name);
    if (index == -1) {
        _SituationSetErrorFromCode(SITUATION_ERROR_NOT_FOUND, "Cannot add point: Path not found.");
        return;
    }
    RGLPathData* Path = &RGL.Paths[index].data;

    if (Path->num_points >= Path->capacity) {
        size_t new_capacity = (Path->capacity == 0) ? 256 : Path->capacity * 2;
        RGLPathPoint* new_points = realloc(Path->points, sizeof(RGLPathPoint) * new_capacity);
        if (!new_points) {
             _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to reallocate Path points buffer.");
            return;
        }
        Path->points = new_points;
        Path->capacity = new_capacity;
    }
    Path->points[Path->num_points++] = point;
}

/**
 * @brief Gets the interpolated properties of the currently active path at a specific Z position.
 *
 * This is the core query function for connecting gameplay logic to the path. It supports
 * multiple named paths, seamless path looping, and uses an internal cache to achieve O(1)
 * performance for the vast majority of sequential game loop calls.
 *
 * @param z_pos The world Z position to query. This position will be automatically wrapped for looping paths.
 * @param out_point A pointer to an RGLPathPoint struct to store the interpolated result.
 * @return True if a valid active path exists and properties could be calculated, false otherwise.
 */
SITAPI bool RGL_GetPathPropertiesAt(float z_pos, RGLPathPoint* out_point) {
    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points < 4) {
        return false;
    }

    if (Path->loop_to_z >= 0.0f) {
        float last_z = Path->points[Path->num_points - 1].world_z;
        if (z_pos > last_z) {
            float path_length = last_z - Path->loop_to_z;
            if (path_length > 0.001f) {
                 z_pos = fmodf(z_pos - Path->loop_to_z, path_length) + Path->loop_to_z;
            }
        }
    }

    // --- Segment Search on the active Path ---
    // Start our search from the active Path's "sticky bookmark".
    int p1_idx = Path->last_segment_index_cache;

    // Search forward (most common case).
    while (p1_idx < Path->num_points - 1 && Path->points[p1_idx + 1].world_z <= z_pos) {
        p1_idx++;
    }

    // Search backward (less common case).
    while (p1_idx > 0 && Path->points[p1_idx].world_z > z_pos) {
        p1_idx--;
    }

    // Update the active Path's "sticky bookmark" for the next frame.
    Path->last_segment_index_cache = p1_idx;

    // --- Interpolation (now operates on the active Path's points) ---
    int p0_idx = p1_idx - 1;
    int p2_idx = p1_idx + 1;
    int p3_idx = p1_idx + 2;

    p0_idx = (p0_idx < 0) ? 0 : p0_idx;
    p1_idx = (p1_idx < 0) ? 0 : p1_idx;
    p2_idx = (p2_idx >= Path->num_points) ? Path->num_points - 1 : p2_idx;
    p3_idx = (p3_idx >= Path->num_points) ? Path->num_points - 1 : p3_idx;

    // All point lookups now use the 'Path->points' array.
    RGLPathPoint p0 = Path->points[p0_idx];
    RGLPathPoint p1 = Path->points[p1_idx];
    RGLPathPoint p2 = Path->points[p2_idx];
    RGLPathPoint p3 = Path->points[p3_idx];

    float segment_length_z = p2.world_z - p1.world_z;
    float t = (segment_length_z > 0.0001f) ? (z_pos - p1.world_z) / segment_length_z : 0.0f;
    t = fmaxf(0.0f, fminf(1.0f, t));

    out_point->world_z = z_pos;
    out_point->world_x_offset       = _catmull_rom(p0.world_x_offset, p1.world_x_offset, p2.world_x_offset, p3.world_x_offset, t);
    out_point->world_y_offset       = _catmull_rom(p0.world_y_offset, p1.world_y_offset, p2.world_y_offset, p3.world_y_offset, t);
    out_point->path_roll_degrees    = _lerp(p1.path_roll_degrees, p2.path_roll_degrees, t);
    out_point->primary_ribbon_width        = _lerp(p1.primary_ribbon_width, p2.primary_ribbon_width, t);
    out_point->split_offset         = _lerp(p1.split_offset, p2.split_offset, t);
    out_point->split_width          = _lerp(p1.split_width, p2.split_width, t);
    out_point->rumble_width         = _lerp(p1.rumble_width, p2.rumble_width, t);
    out_point->split_offset         = _lerp(p1.split_offset, p2.split_offset, t);
    out_point->split_width          = _lerp(p1.split_width, p2.split_width, t);
    
    out_point->split_surface_texture   = p1.split_surface_texture;
    out_point->split_surface_color     = p1.split_surface_color;
    out_point->split_lanes          = p1.split_lanes;
    out_point->primary_lanes        = p1.primary_lanes;
    out_point->surface_texture         = p1.surface_texture;
    out_point->color_surface           = p1.color_surface;
    out_point->color_rumble         = p1.color_rumble;
    out_point->color_lines          = p1.color_lines;
    out_point->scenery_left         = p1.scenery_left;
    out_point->scenery_right        = p1.scenery_right;
    out_point->scenery_overhead     = p1.scenery_overhead;
    out_point->user_tag             = p1.user_tag;

    return true;
}

/**
 * @brief Performs a raycast-like query to find ground properties at a world XZ coordinate.
 *
 * @param world_xz The X and Z world coordinates to check.
 * @param out_info A pointer to an RGLGroundInfo struct to store the results.
 * @return True if any ground was found (Path, rumble, or off-Path), false if the Z is off the path.
 */
// NOTE: All other drawing functions (DrawBillboard, DrawPolygon, etc.) should be
// updated with the same `_RGL_EnsureCommandCapacity` logic as `RGL_DrawSpritePro`.
SITAPI bool RGL_GetGroundAt(vec2 world_xz, RGLGroundInfo* out_info) {
    if (!out_info) return false;

    RGLPathPoint props;
    if (!RGL_GetPathPropertiesAt(world_xz[1], &props)) {
        out_info->is_hit = false;
        return false;
    }

    out_info->is_hit = true;
    out_info->ground_y = props.world_y_offset;
    out_info->type = RGL_GROUND_TYPE_OFF_PATH;

    float Path_center_x = props.world_x_offset;
    float primary_half_width = props.primary_ribbon_width * 0.5f;
    float primary_rumble_half_width = primary_half_width + props.rumble_width;
    float dx_primary = world_xz[0] - Path_center_x;
    
    // Check primary Path
    if (fabsf(dx_primary) < primary_rumble_half_width) {
        float lateral_offset = (primary_half_width > 0.01f) ? dx_primary / primary_half_width : 0.0f;
        _RGL_CalculateBankedSurface(&props, lateral_offset, out_info->surface_normal);
        
        // Adjust ground height based on banking (outer edge of a banked turn is higher)
        float banking_height_offset = sinf(glm_rad(props.path_roll_degrees)) * dx_primary;
        out_info->ground_y += banking_height_offset;

        out_info->type = (fabsf(dx_primary) < primary_half_width) ? RGL_GROUND_TYPE_PATH : RGL_GROUND_TYPE_SHOULDER;
        return true;
    }

    // Check split Path
    if (props.split_width > 0.01f) {
        float split_center_x = props.world_x_offset + props.split_offset;
        float split_half_width = props.split_width * 0.5f;
        float split_rumble_half_width = split_half_width + props.rumble_width;
        float dx_split = world_xz[0] - split_center_x;

        if (fabsf(dx_split) < split_rumble_half_width) {
            float lateral_offset = (split_half_width > 0.01f) ? dx_split / split_half_width : 0.0f;
            _RGL_CalculateBankedSurface(&props, lateral_offset, out_info->surface_normal);
            float banking_height_offset = sinf(glm_rad(props.path_roll_degrees)) * dx_split;
            out_info->ground_y += banking_height_offset;
            out_info->type = (fabsf(dx_split) < split_half_width) ? RGL_GROUND_TYPE_PATH : RGL_GROUND_TYPE_SHOULDER;
            return true;
        }
    }

    // Off-Path: use a flat normal
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, out_info->surface_normal);
    return true;
}

/**
 * @brief Updates dynamic scenery elements along the active Path, such as creating lights.
 * This function should be called once per frame to ensure scenery state is continuous.
 * It handles creating RGL light objects from scenery definitions as the player approaches them.
 *
 * @param player_z The player's current Z position along the path.
 * @param view_distance The forward distance to check for scenery.
 */
SITAPI void RGL_UpdatePathScenery(float player_z, float view_distance) {
    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points < 2) return;

    // Determine the range of path points to check
    int start_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, player_z);
    if (start_idx == -1) start_idx = 0;
    
    int end_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, player_z + view_distance);
    if (end_idx == -1) end_idx = Path->num_points - 1;

    // Lambda helper function to process a single scenery object
    auto void process_scenery(RGLScenery* scenery, RGLPathPoint* p) {
        if (scenery->type == RGL_SCENERY_LIGHT_SOURCE) {
            // If the light hasn't been created in the RGL system yet...
            if (scenery->data.light.light_id == 0) {
                // Calculate its absolute world position using the parent path point.
                // This is the CRITICAL step that was missing.
                vec3 pos = {
                    p->world_x_offset + scenery->x_offset,
                    p->world_y_offset + scenery->y_offset,
                    p->world_z
                };
                
                // Create the light and store its ID back in the scenery data.
                int id = RGL_CreatePointLight(pos, scenery->data.light.color,
                                              scenery->data.light.radius,
                                              scenery->data.light.intensity);
                scenery->data.light.light_id = id;
            }
            // Ensure the light is active (it might have been deactivated if we drove far away)
            if (scenery->data.light.light_id > 0) {
                 RGL_SetLightActive(scenery->data.light.light_id, true);
            }
        }
    };

    // Iterate through the visible segment of the path and process all scenery slots
    for (int i = start_idx; i <= end_idx; i++) {
        RGLPathPoint* p = &Path->points[i];
        process_scenery(&p->scenery_left, p);
        process_scenery(&p->scenery_right, p);
        process_scenery(&p->scenery_overhead, p);
    }
}

/**
 * @brief (INTERNAL) Queues a single quad for the Path system with a specified normal.
 * This is the new core drawing primitive for RGL_DrawPathAsRoad.
 */
static void _RGL_DrawPathAsRoadQuad(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 normal, RGLSprite sprite, Color color) {
    if (!_RGL_EnsureCommandCapacity(1)) return;

    RGLInternalDraw* cmd = &RGL.commands[RGL.command_count++];
    cmd->texture = sprite.texture;
    cmd->is_triangle = false;
    cmd->z_depth = (p1[2] + p2[2] + p3[2] + p4[2]) * 0.25f;

    // Vertices in TL, BL, BR, TR order for the batcher
    glm_vec3_copy(p4, cmd->world_positions[0]);
    glm_vec3_copy(p1, cmd->world_positions[1]);
    glm_vec3_copy(p2, cmd->world_positions[2]);
    glm_vec3_copy(p3, cmd->world_positions[3]);

    // Assign UVs
    cmd->tex_coords[0][0] = 0.0f; cmd->tex_coords[0][1] = 0.0f;
    cmd->tex_coords[1][0] = 0.0f; cmd->tex_coords[1][1] = 1.0f;
    cmd->tex_coords[2][0] = 1.0f; cmd->tex_coords[2][1] = 1.0f;
    cmd->tex_coords[3][0] = 1.0f; cmd->tex_coords[3][1] = 0.0f;

    // Assign color, base light, and the crucial normal vector
    vec4 v4_color;
    SituationConvertColorToVec4(color, v4_color);
    for (int i = 0; i < 4; i++) {
        glm_vec4_copy(v4_color, cmd->colors[i]);
        cmd->light_levels[i] = 1.0f; // Paths are fully lit by default
        glm_vec3_copy(normal, cmd->normals[i]);
    }
}

/**
 * @brief (INTERNAL) The default implementation for drawing a path segment as a classic road.
 * This function contains the logic that USED to be inside the monolithic RGL_DrawPathAsRoad.
 */
static void _RGL_DrawSegment_Road(const RGLPathPoint* p_near, const RGLPathPoint* p_far, const vec3* normal, void* user_data) {
    (void)user_data; // Unused in this default implementation

    float z_near = p_near->world_z;
    float z_far  = p_far->world_z;

    // --- RENDER THE PRIMARY PATH ---
    vec3 p1 = {p_near->world_x_offset - p_near->primary_ribbon_width * 0.5f, p_near->world_y_offset, z_near};
    vec3 p2 = {p_far->world_x_offset  - p_far->primary_ribbon_width  * 0.5f, p_far->world_y_offset,  z_far};
    vec3 p3 = {p_far->world_x_offset  + p_far->primary_ribbon_width  * 0.5f, p_far->world_y_offset,  z_far};
    vec3 p4 = {p_near->world_x_offset + p_near->primary_ribbon_width * 0.5f, p_near->world_y_offset, z_near};
    
    Color road_color = ((int)(z_near / 10.0f) % 2 == 0) ? p_near->color_surface : (Color){60,60,60,255};
    _RGL_DrawPathQuad(p1, p2, p3, p4, *normal, p_near->surface_texture, road_color);
    
    // Draw Rumble Strips
    if (p_near->rumble_width > 0.0f) {
        Color rumble_color = ((int)(z_near / 5.0f) % 2 == 0) ? p_near->color_rumble : WHITE;
        float rumble_w = p_near->rumble_width;
        
        vec3 r1 = {p1[0] - rumble_w, p1[1], p1[2]};
        vec3 r2 = {p2[0] - rumble_w, p2[1], p2[2]};
        _RGL_DrawPathQuad(r1, r2, p2, p1, *normal, (RGLSprite){0}, rumble_color); // Left
        
        vec3 r3 = {p3[0] + rumble_w, p3[1], p3[2]};
        vec3 r4 = {p4[0] + rumble_w, p4[1], p4[2]};
        _RGL_DrawPathQuad(p4, p3, r3, r4, *normal, (RGLSprite){0}, rumble_color); // Right
    }
    
    // Draw Lane Markings
    if (p_near->primary_lanes > 1 && ((int)(z_near / 4.0f) % 2 != 0)) {
        float lane_width = p_near->primary_ribbon_width / p_near->primary_lanes;
        float line_half_w = 0.15f;
        for(int j = 1; j < p_near->primary_lanes; ++j) {
            float x_offset = -p_near->primary_ribbon_width * 0.5f + j * lane_width;
            vec3 l1 = {p_near->world_x_offset + x_offset - line_half_w, p_near->world_y_offset, z_near};
            vec3 l2 = {p_far->world_x_offset  + x_offset - line_half_w, p_far->world_y_offset,  z_far};
            vec3 l3 = {p_far->world_x_offset  + x_offset + line_half_w, p_far->world_y_offset,  z_far};
            vec3 l4 = {p_near->world_x_offset + x_offset + line_half_w, p_near->world_y_offset, z_near};
            _RGL_DrawPathQuad(l1, l2, l3, l4, *normal, (RGLSprite){0}, p_near->color_lines);
        }
    }
    
    // --- RENDER THE SPLIT PATH ---
    if (p_near->split_width > 0.01f) {
        float split_x_near = p_near->world_x_offset + p_near->split_offset;
        float split_x_far  = p_far->world_x_offset  + p_far->split_offset;
        vec3 s1 = {split_x_near - p_near->split_width * 0.5f, p_near->world_y_offset, z_near};
        vec3 s2 = {split_x_far  - p_far->split_width  * 0.5f, p_far->world_y_offset,  z_far};
        vec3 s3 = {split_x_far  + p_far->split_width  * 0.5f, p_far->world_y_offset,  z_far};
        vec3 s4 = {split_x_near + p_near->split_width * 0.5f, p_near->world_y_offset, z_near};
        Color split_color = ((int)(z_near/10.f)%2 == 0) ? p_near->split_surface_color : (Color){50,50,50,255};
        _RGL_DrawPathQuad(s1, s2, s3, s4, *normal, p_near->split_surface_texture, split_color);
    }
}

/**
 * @brief [Convenience Wrapper] Draws the active path as a classic road.
 *
 * This is a high-level helper function for the most common use case.
 * It is equivalent to calling `RGL_DrawPath()` on a path that is using the default road style.
 * Note: If you have set a custom style on the active path using `RGL_SetPathStyle`,
 * calling this function will NOT override it. It will draw with your custom style.
 * To guarantee a road is drawn, you would first call `RGL_SetPathStyle(path_name, RGL_GetDefaultRoadStyle());`.
 */
SITAPI void RGL_DrawPathAsRoad(float player_z, int draw_distance) {
    // This function simply calls the generic dispatcher. The dispatcher will then
    // look up the path's assigned style and execute it.
    RGL_DrawPath(player_z, draw_distance);
}

/**
 * @brief (INTERNAL) Helper to draw one segment of the Path surface (Path + rumbles).
 */
static void _RGL_DrawPathAsRoadSurface(RGLScalerProjection p_near, RGLScalerProjection p_far, RGLPathPoint* prop_near, RGLPathPoint* prop_far, bool is_split) {
    vec2 points[4];
    
    // Determine which set of properties to use (primary or split Path)
    float width_near = is_split ? prop_near->split_width : prop_near->primary_ribbon_width;
    float width_far = is_split ? prop_far->split_width : prop_far->primary_ribbon_width;
    // A more advanced implementation might have separate colors for split Paths
    Color color_surface = prop_near->color_surface;
    Color color_rumble = prop_near->color_rumble;

    // 1. Draw wide polygon for rumble strips first (the background layer)
    float total_width_near = width_near + prop_near->rumble_width * 2.0f;
    float total_width_far = width_far + prop_far->rumble_width * 2.0f;
    if (RGL_GetPathSegmentPoints(p_near, p_far, total_width_near, total_width_far, points)) {
        // Alternate rumble color for a classic striped effect
        Color final_rumble_color = ((int)(prop_near->world_z / 5.0f) % 2 == 0) ? color_rumble : WHITE;
        RGL_DrawPolygonScreen(points, 4, final_rumble_color);
    }

    // 2. Draw Path surface polygon on top
    if (RGL_GetPathSegmentPoints(p_near, p_far, width_near, width_far, points)) {
        // Alternate Path color slightly for segment definition
        Color final_Path_color = ((int)(prop_near->world_z / 10.0f) % 2 == 0) ? color_surface : (Color){60, 60, 60, 255};
        RGL_DrawPolygonScreen(points, 4, final_Path_color);
    }
}

/**
 * @brief (INTERNAL) The default drawing implementation for rendering a path as a classic road.
 *
 * This function is the heart of the default visual style. It contains the complete,
 * advanced logic for rendering a multi-lane road with shoulders, dashed lines, and splits,
 * as well as rendering all associated scenery in the correct back-to-front order. It is
 * called by RGL_DrawPath() when a path's style is set to the default road style.
 *
 * @param player_z The current Z-position of the camera or player.
 * @param draw_distance The number of segments to draw into the distance.
 * @param user_data A generic pointer from the RGLPathStyle struct (unused in this default implementation).
 */
static void _RGL_DrawPathScene_Road(float player_z, int draw_distance, void* user_data) {
    // This function is the new home for the logic previously in RGL_DrawPathAsRoad.
    (void)user_data; // This default implementation doesn't use custom data.

    RGLPathData* path = _RGL_GetActivePathData();
    if (!path || path->num_points < 2) {
        return;
    }

    // --- 1. Looping Logic ---
    // Handle seamless looping by wrapping the player's Z-position if the path is configured to loop.
    if (path->loop_to_z >= 0.0f) {
        float last_z = path->points[path->num_points - 1].world_z;
        if (player_z > last_z) {
            float path_length = last_z - path->loop_to_z;
            if (path_length > 0.001f) {
                player_z = fmodf(player_z - path->loop_to_z, path_length) + path->loop_to_z;
            }
        }
    }

    const float segment_length = 5.0f;

    // --- 2. Main Drawing Loop (Far to Near) for Road Geometry ---
    // We iterate from the farthest visible segment towards the camera for correct alpha blending.
    for (int i = draw_distance; i > 0; i--) {
        RGLPathPoint prop_near, prop_far;
        float z_near = player_z + (i - 1) * segment_length;
        float z_far  = player_z + i * segment_length;

        // Interpolate path properties at the start and end of the segment.
        if (!RGL_GetPathPropertiesAt(z_near, &prop_near)) continue;
        if (!RGL_GetPathPropertiesAt(z_far, &prop_far)) continue;

        // Calculate the surface normal for lighting, based on the banking of the near point.
        vec3 normal;
        _RGL_CalculateBankedSurface(&prop_near, 0.0f, normal);

        // --- RENDER THE PRIMARY ROAD SURFACE ---
        vec3 p1 = {prop_near.world_x_offset - prop_near.primary_ribbon_width * 0.5f, prop_near.world_y_offset, z_near};
        vec3 p2 = {prop_far.world_x_offset  - prop_far.primary_ribbon_width  * 0.5f, prop_far.world_y_offset,  z_far};
        vec3 p3 = {prop_far.world_x_offset  + prop_far.primary_ribbon_width  * 0.5f, prop_far.world_y_offset,  z_far};
        vec3 p4 = {prop_near.world_x_offset + prop_near.primary_ribbon_width * 0.5f, prop_near.world_y_offset, z_near};
        
        // Alternate road color for a subtle segment definition effect.
        Color road_color = ((int)(z_near / 10.0f) % 2 == 0) ? prop_near.color_surface : (Color){60,60,60,255};
        _RGL_DrawPathQuad(p1, p2, p3, p4, normal, prop_near.surface_texture, road_color);
        
        // --- RENDER RUMBLE STRIPS / SHOULDERS ---
        if (prop_near.rumble_width > 0.0f) {
            Color rumble_color = ((int)(z_near / 5.0f) % 2 == 0) ? prop_near.color_rumble : WHITE;
            float rumble_w = prop_near.rumble_width;
            
            // Left shoulder
            vec3 r1 = {p1[0] - rumble_w, p1[1], p1[2]};
            vec3 r2 = {p2[0] - rumble_w, p2[1], p2[2]};
            _RGL_DrawPathQuad(r1, r2, p2, p1, normal, (RGLSprite){0}, rumble_color);
            
            // Right shoulder
            vec3 r3 = {p3[0] + rumble_w, p3[1], p3[2]};
            vec3 r4 = {p4[0] + rumble_w, p4[1], p4[2]};
            _RGL_DrawPathQuad(p4, p3, r3, r4, normal, (RGLSprite){0}, rumble_color);
        }
        
        // --- RENDER LANE MARKINGS ---
        if (prop_near.primary_lanes > 1 && ((int)(z_near / 4.0f) % 2 != 0)) { // Dashed lines effect
            float lane_width = prop_near.primary_ribbon_width / prop_near.primary_lanes;
            float line_half_w = 0.15f;
            for(int j = 1; j < prop_near.primary_lanes; ++j) {
                float x_offset = -prop_near.primary_ribbon_width * 0.5f + j * lane_width;
                vec3 l1 = {prop_near.world_x_offset + x_offset - line_half_w, prop_near.world_y_offset + 0.01f, z_near};
                vec3 l2 = {prop_far.world_x_offset  + x_offset - line_half_w, prop_far.world_y_offset  + 0.01f, z_far};
                vec3 l3 = {prop_far.world_x_offset  + x_offset + line_half_w, prop_far.world_y_offset  + 0.01f, z_far};
                vec3 l4 = {prop_near.world_x_offset + x_offset + line_half_w, prop_near.world_y_offset + 0.01f, z_near};
                _RGL_DrawPathQuad(l1, l2, l3, l4, normal, (RGLSprite){0}, prop_near.color_lines);
            }
        }
        
        // --- RENDER THE SPLIT ROAD ---
        if (prop_near.split_width > 0.01f) {
            float split_x_near = prop_near.world_x_offset + prop_near.split_offset;
            float split_x_far  = prop_far.world_x_offset  + prop_far.split_offset;
            vec3 s1 = {split_x_near - prop_near.split_width * 0.5f, prop_near.world_y_offset, z_near};
            vec3 s2 = {split_x_far  - prop_far.split_width  * 0.5f, prop_far.world_y_offset,  z_far};
            vec3 s3 = {split_x_far  + prop_far.split_width  * 0.5f, prop_far.world_y_offset,  z_far};
            vec3 s4 = {split_x_near + prop_near.split_width * 0.5f, prop_near.world_y_offset, z_near};
            Color split_color = ((int)(z_near/10.f)%2 == 0) ? prop_near.split_surface_color : (Color){50,50,50,255};
            _RGL_DrawPathQuad(s1, s2, s3, s4, normal, prop_near.split_surface_texture, split_color);
        }
    }

    // --- 3. Scenery Drawing Loop ---
    // This is kept separate from the road geometry loop to ensure all scenery
    // is drawn on top of the road surface, respecting depth.
    int start_idx = path->last_segment_index_cache;
    if (start_idx < 0) start_idx = 0;
    
    // Find the farthest visible path point index.
    int far_idx = _RGL_FindPathPointIndexAt(path->points, path->num_points, player_z + draw_distance * segment_length);
    if (far_idx == -1) far_idx = path->num_points - 1;

    // Iterate backwards from the farthest point to draw scenery back-to-front.
    for (int i = far_idx; i >= start_idx; i--) {
        RGLPathPoint* current_point = &path->points[i];
        if (current_point->world_z < player_z - 50.0f) break; // Simple culling for objects behind the camera

        if (current_point->scenery_left.type != RGL_SCENERY_NONE) _RGL_DrawPathScenery(current_point, ¤t_point->scenery_left);
        if (current_point->scenery_right.type != RGL_SCENERY_NONE) _RGL_DrawPathScenery(current_point, ¤t_point->scenery_right);
        if (current_point->scenery_overhead.type != RGL_SCENERY_NONE) _RGL_DrawPathScenery(current_point, ¤t_point->scenery_overhead);
    }
}

/**
 * @brief (INTERNAL) Draws a single scenery object by dispatching to its registered style.
 *
 * This function is the heart of the extensible scenery system. It replaces a rigid
 * switch statement with a flexible, dynamic dispatch mechanism. Instead of knowing how
 * to draw every type of scenery itself, its only job is to look up the correct drawing
 * function from a global registry and call it.
 *
 * @section how_it_works How It Works (The "Trickery" Explained)
 *   1.  **Calculate Position:** It first calculates the final 3D world-space position of the
 *       scenery object's anchor point, based on the path's geometry and the scenery's own offsets.
 *
 *   2.  **Look Up Style:** It takes the `scenery->type` (which is just an integer/enum) and uses it
 *       as an index into the global `g_scenery_styles` array. For example, if `scenery->type` is
 *       `RGL_SCENERY_SPRITE` (which might be the integer `1`), it looks at `g_scenery_styles[1]`.
 *
 *   3.  **Check for Registration:** It checks if the pointer at that index is `NULL`. If it is, it means
 *       no drawing style has been registered for this scenery type (e.g., for invisible triggers),
 *       and the function simply does nothing, which is the correct behavior.
 *
 *   4.  **Get the Function Pointer:** If a style *is* found, it retrieves the `draw_func` member from
 *       it. This `draw_func` is a function pointer—a variable that holds the memory address of the
 *       actual drawing function (like `_RGL_DrawScenery_Sprite`).
 *
 *   5.  **Execute the Callback:** Finally, it calls the function at that memory address, passing all
 *       the necessary information (the scenery data, its world position, etc.) to it.
 *
 * This allows the system to be extended by the user at runtime. The user can call
 * `RGL_RegisterSceneryStyle()` to "plug in" their own custom drawing functions for new or
 * existing scenery types, and this dispatcher will call them without ever needing to be modified.
 *
 * @param path_point The path point the scenery is attached to.
 * @param scenery The scenery object to be rendered.
 */
static void _RGL_DrawPathScenery(RGLPathPoint* path_point, RGLScenery* scenery) {
    // --- Step 1: Calculate the final 3D world position of the scenery's anchor. ---
    vec3 world_pos;
    world_pos[0] = path_point->world_x_offset + (scenery->x_offset * (path_point->primary_ribbon_width * 0.5f));
    world_pos[1] = path_point->world_y_offset + scenery->y_offset;
    world_pos[2] = path_point->world_z;

    // --- Step 2: Dynamic Dispatch via the Style Registry. ---
    RGLSceneryType type = scenery->type;

    // Check if the type is within the valid range of our registry array.
    if (type < RGL_MAX_SCENERY_TYPES) {
        
        // Look up the style from our global registry using the type as an index.
        const RGLSceneryStyle* style = g_scenery_styles[type];

        // --- Step 3: Check if a style was actually registered for this type. ---
        // If 'style' is NULL, it means this is a non-visual type like an event marker,
        // and we should do nothing.
        if (style != NULL) {
            
            // --- Step 4: Get the function pointer from the style object. ---
            // We also check if the function pointer itself is valid before trying to call it.
            if (style->draw_func != NULL) {
                
                // --- Step 5: Execute the callback function. ---
                // The program "jumps" to the memory address stored in 'style->draw_func'
                // and begins executing the code there (e.g., _RGL_DrawScenery_Sprite).
                style->draw_func(scenery, path_point, &world_pos, style->user_data);
            }
        }
    }
    // If the type is out of bounds or its style is NULL, the function simply ends,
    // correctly drawing nothing.
}

// --- PRIVATE HELPER: Find a named Path and return its index ---
static int _RGL_FindPathIndex(const char* path_name) {
    if (!RGL.is_initialized || !path_name) return -1;
    for (size_t i = 0; i < RGL.Path_count; i++) {
        if (strncmp(RGL.Paths[i].name, path_name, 31) == 0) {
            return i;
        }
    }
    return -1;
}

// --- PRIVATE HELPER: Get a pointer to the currently active Path's data ---
static RGLPathData* _RGL_GetActivePathData() {
    if (!RGL.is_initialized || RGL.active_Path_index < 0 || RGL.active_Path_index >= RGL.Path_count) {
        return NULL;
    }
    return &RGL.Paths[RGL.active_Path_index].data;
}

SITAPI bool RGL_GetDistanceToMarker(float player_z, const char* marker_name, float* out_distance) {
    if (!marker_name || !out_distance) return false;

    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path) return false;

    // Scan forward from the player's position
    for (size_t i = 0; i < Path->num_points; i++) {
        RGLPathPoint* p = &Path->points[i];
        if (p->world_z <= player_z) continue; // Only find markers ahead of the player

        RGLScenery* scenery_slots[] = { &p->scenery_left, &p->scenery_right, &p->scenery_overhead };
        for (int j = 0; j < 3; j++) {
            RGLScenery* s = scenery_slots[j];
            if (s->type == RGL_SCENERY_EVENT_MARKER && strncmp(s->data.event.name, marker_name, 31) == 0) {
                *out_distance = p->world_z - player_z;
                return true;
            }
        }
    }

    // Optional: If the Path loops, check from the beginning of the path as well
    if (Path->loop_to_z >= 0.0f) {
        float path_length = Path->points[Path->num_points - 1].world_z - Path->loop_to_z;
        for (size_t i = 0; i < Path->num_points; i++) {
            RGLPathPoint* p = &Path->points[i];
            // Stop if we've reached the player's original position on the next lap
            if (p->world_z >= player_z) break;

            RGLScenery* scenery_slots[] = { &p->scenery_left, &p->scenery_right, &p->scenery_overhead };
            for (int j = 0; j < 3; j++) {
                RGLScenery* s = scenery_slots[j];
                if (s->type == RGL_SCENERY_EVENT_MARKER && strncmp(s->data.event.name, marker_name, 31) == 0) {
                    *out_distance = (p->world_z + path_length) - player_z;
                    return true;
                }
            }
        }
    }
    
    *out_distance = 0.0f;
    return false;
}

/**
 * @brief Finds all event markers within a specified Z-range of the path.
 *
 * @param start_z The starting Z-coordinate of the search range.
 * @param end_z The ending Z-coordinate of the search range.
 * @param out_markers An array to store the found markers.
 * @param max_markers The maximum number of markers the `out_markers` array can hold.
 * @return The number of markers found and written to the output array.
 */
SITAPI int RGL_FindMarkersInRange(float start_z, float end_z, RGLMarkerInfo out_markers[], int max_markers) {
    if (!out_markers || max_markers <= 0 || start_z >= end_z) return 0;
    
    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path) return 0;

    int found_count = 0;
    // Use the refactored pure helper function
    int start_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, start_z);
    if (start_idx == -1) return 0;

    for (int i = start_idx; i < Path->num_points; i++) {
        RGLPathPoint* p = &Path->points[i];
        if (p->world_z > end_z) break;

        RGLScenery* scenery_slots[] = { &p->scenery_left, &p->scenery_right, &p->scenery_overhead };
        for (int j = 0; j < 3; j++) {
            if (scenery_slots[j]->type == RGL_SCENERY_EVENT_MARKER) {
                RGLMarkerInfo* result = &out_markers[found_count];
                strncpy(result->name, scenery_slots[j]->data.event.name, 31);
                result->name[31] = '\0';
                result->id = scenery_slots[j]->data.event.id;
                result->distance = p->world_z - start_z;
                
                result->world_pos[0] = p->world_x_offset + scenery_slots[j]->x_offset * (p->primary_ribbon_width * 0.5f);
                result->world_pos[1] = p->world_y_offset + scenery_slots[j]->y_offset;
                result->world_pos[2] = p->world_z;
                
                found_count++;
                if (found_count >= max_markers) return found_count;
            }
        }
    }
    return found_count;
}

/**
 * @brief (INTERNAL HELPER) Draws a simple, untextured 2D polygon immediately.
 * This function is for internal use by tools like the map renderer. It does not use
 * the main batching system and submits its data directly to the GPU.
 *
 * @param vao A temporary VAO to use for this draw call.
 * @param vbo A temporary VBO to use for this draw call.
 * @param points An array of 2D points in the coordinate space of the current view.
 * @param point_count The number of points in the array.
 * @param color The solid color to fill the polygon with.
 */
static void _RGL_DrawMapPolygon(GLuint vao, GLuint vbo, vec2* points, int point_count, Color color) {
    if (point_count < 3) return;

    vec4 norm_color;
    SituationConvertColorToVec4(color, norm_color);

    // BUGFIX: Vertex format is 10 floats.
    const int floats_per_vertex = 10;
    float vertices[point_count * floats_per_vertex];
    for(int i = 0; i < point_count; i++) {
        float* v_ptr = &vertices[i * floats_per_vertex];
        v_ptr[0] = points[i][0]; // pos.x
        v_ptr[1] = points[i][1]; // pos.y
        v_ptr[2] = 0.0f;         // pos.z
        v_ptr[3] = 0.0f; v_ptr[4] = 0.0f; // uv
        v_ptr[5] = norm_color[0]; // color.r
        v_ptr[6] = norm_color[1]; // color.g
        v_ptr[7] = norm_color[2]; // color.b
        v_ptr[8] = norm_color[3]; // color.a
        v_ptr[9] = 1.0f;          // light
    }
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * point_count * floats_per_vertex, vertices);
    
    glUniform1i(RGL.loc_use_texture, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, point_count);
}

/**
 * @brief Renders a simplified, top-down 2D map of the active path onto a texture.
 * This is ideal for creating in-game minimaps.
 *
 * @param target The RGLTexture to render to. Must have been created with RGL_CreateRenderTexture().
 * @param center_pos_xz The world-space XZ coordinate to center the map view on (e.g., the player's position).
 * @param world_width The total width of the world area to capture in the map view.
 * @param bg_color The background color of the map.
 */
SITAPI void RGL_DrawPathAsMap(RGLTexture target, vec2 center_pos_xz, float world_width, Color bg_color) {
    if (!RGL.is_initialized || target.fbo_id == 0) return;

    RGL_SetRenderTarget(target);
    
    vec4 norm_bg_color;
    SituationConvertColorToVec4(bg_color, norm_bg_color);
    glClearColor(norm_bg_color[0], norm_bg_color[1], norm_bg_color[2], norm_bg_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect_ratio = (float)target.height / (float)target.width;
    float world_height = world_width * aspect_ratio;
    Rectangle view_rect = { center_pos_xz[0] - world_width * 0.5f, center_pos_xz[1] - world_height * 0.5f, world_width, world_height };

    mat4 ortho_proj, top_down_view;
    glm_ortho(view_rect.x, view_rect.x + view_rect.width, view_rect.y + view_rect.height, view_rect.y, -1.0f, 1.0f, ortho_proj);
    glm_mat4_identity(top_down_view);

    glUseProgram(RGL.main_shader);
    glUniformMatrix4fv(RGL.loc_projection, 1, GL_FALSE, (const GLfloat*)ortho_proj);
    glUniformMatrix4fv(RGL.loc_view, 1, GL_FALSE, (const GLfloat*)top_down_view);
    glDisable(GL_DEPTH_TEST);

    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points < 2) {
        RGL_ResetRenderTarget();
        return;
    }
    
    GLuint map_vao, map_vbo;
    glGenVertexArrays(1, &map_vao);
    glGenBuffers(1, &map_vbo);
    glBindVertexArray(map_vao);
    glBindBuffer(GL_ARRAY_BUFFER, map_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), NULL, GL_STREAM_DRAW);
    
    // BUGFIX: Set up all vertex attributes correctly for the 10-float format.
    const int floats_per_vertex = 10;
    size_t stride = floats_per_vertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(5*sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(9*sizeof(float))); glEnableVertexAttribArray(3);

    Color Path_color = WHITE;
    Color tunnel_color = (Color){100, 100, 100, 255};
    Color scenery_color = (Color){0, 150, 0, 255};
    Color player_color = RED;

    float min_z = view_rect.y;
    float max_z = view_rect.y + view_rect.height;
    int start_idx = _RGL_FindPathPointIndexAt(Path->points, Path->num_points, min_z);
    if (start_idx == -1) start_idx = 0;

    for (int i = start_idx; i < Path->num_points - 1; i++) {
        RGLPathPoint *p_near = &Path->points[i];
        RGLPathPoint *p_far = &Path->points[i+1];
        if (p_near->world_z > max_z) break;

        vec2 Path_quad[4] = {
            { p_near->world_x_offset - p_near->primary_ribbon_width * 0.5f, p_near->world_z },
            { p_far->world_x_offset - p_far->primary_ribbon_width * 0.5f, p_far->world_z },
            { p_far->world_x_offset + p_far->primary_ribbon_width * 0.5f, p_far->world_z },
            { p_near->world_x_offset + p_near->primary_ribbon_width * 0.5f, p_near->world_z }
        };
        _RGL_DrawMapPolygon(map_vao, map_vbo, Path_quad, 4, Path_color);
        
        if (p_near->split_width > 0.01f) {
            vec2 split_quad[4] = {
                { p_near->world_x_offset + p_near->split_offset - p_near->split_width * 0.5f, p_near->world_z },
                { p_far->world_x_offset + p_far->split_offset - p_far->split_width * 0.5f, p_far->world_z },
                { p_far->world_x_offset + p_far->split_offset + p_far->split_width * 0.5f, p_far->world_z },
                { p_near->world_x_offset + p_near->split_offset + p_near->split_width * 0.5f, p_near->world_z }
            };
            _RGL_DrawMapPolygon(map_vao, map_vbo, split_quad, 4, Path_color);
        }
    }
 
    for (int i = start_idx; i < Path->num_points - 1; i++) {
        RGLPathPoint *p_near = &Path->points[i];
        RGLPathPoint *p_far = &Path->points[i+1];
        if (p_near->world_z > max_z) break;

        if (p_near->scenery_left.type == RGL_SCENERY_SPRITE) {
            float x = p_near->world_x_offset + p_near->scenery_left.x_offset * p_near->primary_ribbon_width * 0.5f;
            vec2 dot[4] = {{x-2, p_near->world_z-2}, {x+2, p_near->world_z-2}, {x+2, p_near->world_z+2}, {x-2, p_near->world_z+2}};
            _RGL_DrawMapPolygon(map_vao, map_vbo, dot, 4, scenery_color);
        }
        
        if (p_near->scenery_overhead.type == RGL_SCENERY_ARCH) {
            vec2 tunnel_quad[4] = {
                { p_near->world_x_offset - p_near->primary_ribbon_width * 0.5f, p_near->world_z },
                { p_far->world_x_offset - p_far->primary_ribbon_width * 0.5f, p_far->world_z },
                { p_far->world_x_offset + p_far->primary_ribbon_width * 0.5f, p_far->world_z },
                { p_near->world_x_offset + p_near->primary_ribbon_width * 0.5f, p_near->world_z }
            };
            _RGL_DrawMapPolygon(map_vao, map_vbo, tunnel_quad, 4, tunnel_color);
        }
    }

    float s = 4.0f;
    vec2 player_dot[4] = {
        {center_pos_xz[0]-s, center_pos_xz[1]-s}, {center_pos_xz[0]+s, center_pos_xz[1]-s},
        {center_pos_xz[0]+s, center_pos_xz[1]+s}, {center_pos_xz[0]-s, center_pos_xz[1]+s}
    };
    _RGL_DrawMapPolygon(map_vao, map_vbo, player_dot, 4, player_color);

    glDeleteBuffers(1, &map_vbo);
    glDeleteVertexArrays(1, &map_vao);
    RGL_ResetRenderTarget();
    
    int width, height;
    SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &width, &height);
    if(width > 0 && height > 0) glViewport(0, 0, width, height);
}

/**
 * @brief Queries the active path for a junction at the player's position and returns all available choices.
 *
 * This is the definitive function for handling all path network topology. It finds the nearest
 * RGL_SCENERY_JUNCTION_TRIGGER and populates the `out_info` struct with the type of junction
 * and all valid destination paths.
 *
 * @param player_z The current Z-position of the player on the active path.
 * @param search_radius The forward distance (in world units) to look for a junction trigger.
 * @param out_info A pointer to an RGLJunctionInfo struct to store the complete result.
 * @return True if a junction trigger was found within the search radius, false otherwise.
 */
SITAPI bool RGL_QueryJunction(float player_z, float search_radius, RGLJunctionInfo* out_info) {
    if (!out_info) {
        return false;
    }
    memset(out_info, 0, sizeof(RGLJunctionInfo));
    out_info->is_valid = false;

    RGLPathData* path = _RGL_GetActivePathData();
    if (!path || path->num_points == 0) {
        return false;
    }

    // Define the Z-range to search for a junction trigger
    const float start_z = player_z;
    const float end_z = player_z + search_radius;

    // Find the starting path point for our search
    int start_idx = _RGL_FindPathPointIndexAt(path->points, path->num_points, start_z);
    if (start_idx == -1) {
        return false; // Search range is completely off the path
    }

    // --- Direct Search Loop (from start_idx to the end of the path or search radius) ---
    for (int i = start_idx; i < path->num_points; i++) {
        RGLPathPoint* p = &path->points[i];

        // Stop if we've searched past the end of our radius
        if (p->world_z > end_z) {
            break;
        }

        // Create a lambda to check a scenery slot and populate our struct if it's a junction.
        auto check_and_populate = [&](RGLScenery* scenery) {
            if (scenery->type == RGL_SCENERY_JUNCTION_TRIGGER) {
                // We found a junction trigger! This is our definitive result.
                out_info->is_valid = true;
                out_info->type = scenery->data.junction.type;
                
                // Copy connection data for all three possible directions.
                memcpy(&out_info->choice_left, &scenery->data.junction.connect_left, sizeof(out_info->choice_left));
                memcpy(&out_info->choice_right, &scenery->data.junction.connect_right, sizeof(out_info->choice_right));
                memcpy(&out_info->choice_straight, &scenery->data.junction.connect_straight, sizeof(out_info->choice_straight));
                
                return true; // Signal that we found it and can stop searching.
            }
            return false;
        };

        // Check all three scenery slots at this path point.
        if (check_and_populate(&p->scenery_left)) return true;
        if (check_and_populate(&p->scenery_right)) return true;
        if (check_and_populate(&p->scenery_overhead)) return true;
    }

    // No junction trigger was found in the search range.
    return false;
}

/**
 * @brief (INTERNAL, PURE) Binary search to find the index of the first path point at or after a given Z position.
 * This is a critical performance helper for queries, running in O(log n) time.
 * @param points A pointer to the array of path points to search.
 * @param num_points The number of points in the array.
 * @param z_pos The world Z-coordinate to search for.
 * @return The index of the first point >= z_pos, or -1 if all points are before z_pos.
 */
static int _RGL_FindPathPointIndexAt(RGLPathPoint* points, size_t num_points, float z_pos) {
    if (!points || num_points == 0) return -1;

    int low = 0;
    int high = num_points - 1;
    int result = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (points[mid].world_z >= z_pos) {
            result = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return result;
}

/**
 * @brief Draws a wireframe bounding box in 3D space.
 * Useful for debugging collision bounds, object extents, and spatial queries.
 * @param min_bounds The minimum corner of the bounding box.
 * @param max_bounds The maximum corner of the bounding box.
 * @param color The color of the wireframe lines.
 */
SITAPI void RGL_DrawWireframeBounds(vec3 min_bounds, vec3 max_bounds, Color color) {
    if (!RGL.is_initialized || !RGL.is_batching) return;
    if (!_RGL_InitDebugRendering()) return;
    
    // Flush pending commands to ensure debug shapes draw after the main scene geometry.
    _RGL_FlushBatch();
    
    vec3 v[8] = {
        {min_bounds[0], min_bounds[1], min_bounds[2]}, {max_bounds[0], min_bounds[1], min_bounds[2]},
        {max_bounds[0], max_bounds[1], min_bounds[2]}, {min_bounds[0], max_bounds[1], min_bounds[2]},
        {min_bounds[0], min_bounds[1], max_bounds[2]}, {max_bounds[0], min_bounds[1], max_bounds[2]},
        {max_bounds[0], max_bounds[1], max_bounds[2]}, {min_bounds[0], max_bounds[1], max_bounds[2]}
    };
    int e[24] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
    
    float line_verts[72];
    for (int i = 0; i < 24; i++) memcpy(&line_verts[i*3], v[e[i]], sizeof(vec3));
    
    mat4 mvp;
    glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, mvp);
    
    glUseProgram(RGL.debug.wireframe_shader);
    glUniformMatrix4fv(RGL.debug.wireframe_mvp_loc, 1, GL_FALSE, (float*)mvp);
    vec4 norm_color; SituationConvertColorToVec4(color, norm_color);
    glUniform4fv(RGL.debug.wireframe_color_loc, 1, norm_color);
    
    glBindVertexArray(RGL.debug.wireframe_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.debug.wireframe_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_verts), line_verts);
    
    glDepthMask(GL_FALSE);
    glDrawArrays(GL_LINES, 0, 24);
    glDepthMask(GL_TRUE);
    
    glBindVertexArray(0);
    glUseProgram(0);
}

/**
 * @brief Draws a comprehensive, in-world debug visualization for the active path.
 * This function is an essential tool for level design and debugging, rendering the
 * path's control points, the smoothly interpolated spline curve, and other metadata
 * directly in the 3D scene.
 * 
 * @param player_z The current Z position of the camera or player, used as a center point for the visualization.
 * @param show_control_points If true, renders the raw RGLPathPoint locations as cubes.
 * @param show_splines If true, renders the calculated Path edges and other interpolated data.
 */
SITAPI void RGL_DrawPathDebugInfo(float player_z, bool show_control_points, bool show_splines) {
    if (!RGL.is_initialized || !RGL.is_batching) return;
    
    RGLPathData* Path = _RGL_GetActivePathData();
    if (!Path || Path->num_points < 2) return;
    
    // Ensure both the wireframe and text debug systems are ready.
    if (!_RGL_InitDebugRendering() || !_RGL_InitDebugTextSystem()) return;
    
    const float DEBUG_RANGE = 500.0f; // Only draw debug info within this Z-distance from the player.
    const float MIN_Z = player_z - 50.0f;  // Don't draw behind the camera.
    const float MAX_Z = player_z + DEBUG_RANGE;
    
    // Colors for different debug elements
    Color control_point_color = {255, 50, 50, 255};      // Red
    Color bank_vector_color   = {255, 165, 0, 255};      // Orange
    Color Path_bounds_color   = {100, 100, 255, 255};    // Blue
    Color split_bounds_color  = {255, 255, 100, 255};    // Yellow
    Color text_color          = {255, 255, 255, 200};    // White
    
    // --- 1. Draw Raw Control Points (User-defined data) ---
    if (show_control_points) {
        for (size_t i = 0; i < Path->num_points; i++) {
            RGLPathPoint* point = &Path->points[i];
            if (point->world_z < MIN_Z || point->world_z > MAX_Z) continue;
            
            vec3 point_pos = {point->world_x_offset, point->world_y_offset, point->world_z};
            
            // Draw a cube at the control point's location.
            vec3 cube_min, cube_max;
            float cube_size = 1.0f;
            glm_vec3_sub_s(point_pos, cube_size, cube_min);
            glm_vec3_add_s(point_pos, cube_size, cube_max);
            RGL_DrawWireframeBounds(cube_min, cube_max, control_point_color);

            // Draw a text label above the control point.
            char label[64];
            snprintf(label, sizeof(label), "Z:%.1f T:%d", point->world_z, point->user_tag);
            // We need to project the 3D world point to 2D screen space for the text.
            mat4 view_proj;
            vec4 screen_pos_h; // Homogeneous coordinates
            glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, view_proj);
            glm_mat4_mulv(view_proj, (vec4){point_pos[0], point_pos[1] + 3.0f, point_pos[2], 1.0f}, screen_pos_h);

            // Convert from homogeneous to normalized device coordinates (-1 to 1)
            if (screen_pos_h[3] > 0.0f) { // Only draw if it's in front of the camera
                vec3 screen_pos_ndc;
                glm_vec3_divs(screen_pos_h, screen_pos_h[3], screen_pos_ndc);

                // Convert from NDC to screen pixels
                int screen_w, screen_h;
                SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &screen_w, &screen_h);
                int screen_x = (int)((screen_pos_ndc[0] + 1.0f) * 0.5f * screen_w);
                int screen_y = (int)((1.0f - screen_pos_ndc[1]) * 0.5f * screen_h);
                _RGL_DrawDebugText(label, screen_x, screen_y, 12, text_color);
            }
            
            // Draw a line representing the banking normal vector.
            if (fabsf(point->path_roll_degrees) > 0.1f) {
                vec3 bank_normal, bank_end_pos;
                _RGL_CalculateBankedSurface(point, 0.0f, bank_normal);
                glm_vec3_scale_as(bank_normal, 10.0f, bank_end_pos);
                glm_vec3_add(point_pos, bank_end_pos, bank_end_pos);

                // Draw a small cube at the end of the normal vector.
                // A proper line drawing function would be better, but this works.
                glm_vec3_sub_s(bank_end_pos, 0.5f, cube_min);
                glm_vec3_add_s(bank_end_pos, 0.5f, cube_max);
                RGL_DrawWireframeBounds(cube_min, cube_max, bank_vector_color);
            }
        }
    }
    
    // --- 2. Draw Interpolated Spline Geometry ---
    if (show_splines) {
        const float SPLINE_STEP = 5.0f; // Draw a marker every 5 world units.
        
        for (float z = MIN_Z; z < MAX_Z; z += SPLINE_STEP) {
            float current_z = (fmodf(z, SPLINE_STEP) > 0) ? (floorf(z / SPLINE_STEP) * SPLINE_STEP) : z;
            if (current_z < MIN_Z) continue;

            RGLPathPoint p;
            if (!RGL_GetPathPropertiesAt(current_z, &p)) continue;
            
            // Calculate the 3D position of the left and right Path edges.
            vec3 Path_up = {0,1,0}, Path_right = {1,0,0}, Path_center = {p.world_x_offset, p.world_y_offset, p.world_z};
            _RGL_CalculateBankedSurface(&p, 0.0f, Path_up);
            glm_vec3_cross(Path_up, (vec3){0,0,1}, Path_right); // right = up x forward
            glm_vec3_normalize(Path_right);

            float half_width = p.primary_ribbon_width * 0.5f;
            vec3 offset_vec, left_edge, right_edge;

            glm_vec3_scale(Path_right, half_width, offset_vec);
            glm_vec3_sub(Path_center, offset_vec, left_edge);
            glm_vec3_add(Path_center, offset_vec, right_edge);
            
            // Draw small cubes at the Path edges.
            vec3 edge_min, edge_max;
            float edge_size = 0.5f;
            
            glm_vec3_sub_s(left_edge, edge_size, edge_min);
            glm_vec3_add_s(left_edge, edge_size, edge_max);
            RGL_DrawWireframeBounds(edge_min, edge_max, Path_bounds_color);
            
            glm_vec3_sub_s(right_edge, edge_size, edge_min);
            glm_vec3_add_s(right_edge, edge_size, edge_max);
            RGL_DrawWireframeBounds(edge_min, edge_max, Path_bounds_color);
            
            // Draw split Path boundaries if they exist.
            if (p.split_width > 0.01f) {
                vec3 split_center;
                vec3 split_offset_vec;
                glm_vec3_scale(Path_right, p.split_offset, split_offset_vec);
                glm_vec3_add(Path_center, split_offset_vec, split_center);
                
                float split_half_width = p.split_width * 0.5f;
                vec3 split_left, split_right;

                glm_vec3_scale(Path_right, split_half_width, offset_vec);
                glm_vec3_sub(split_center, offset_vec, split_left);
                glm_vec3_add(split_center, offset_vec, split_right);
                
                glm_vec3_sub_s(split_left, edge_size, edge_min);
                glm_vec3_add_s(split_left, edge_size, edge_max);
                RGL_DrawWireframeBounds(edge_min, edge_max, split_bounds_color);
                
                glm_vec3_sub_s(split_right, edge_size, edge_min);
                glm_vec3_add_s(split_right, edge_size, edge_max);
                RGL_DrawWireframeBounds(edge_min, edge_max, split_bounds_color);
            }
        }
    }
}


/**
 * @brief (INTERNAL) Initializes the self-contained debug text rendering system.
 * This function creates a bitmap font for debug purposes using the public RGL API
 * and an embedded font from font_data.h. It is called automatically on the first
 * debug text draw call.
 * @return True on success, false on failure.
 */
static bool _RGL_InitDebugTextSystem(void) {
    if (RGL.debug.font_initialized) return true;

    // This now works perfectly. The preprocessor will change FONT_DATA_8X8
    // to ibm_font_8x8 before compilation.
    RGL.debug.font = RGL_CreateCP437Font(FONT_DATA_8X8); 
    
    if (RGL.debug.font.atlas_texture.id == 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INITIALIZATION_FAILED, "Failed to create internal debug font.");
        return false;
    }
    
    RGL.debug.font_initialized = true;
    return true;
}

/**
 * @brief (INTERNAL) Frees the texture used by the debug text system.
 */
static void _RGL_ShutdownDebugTextSystem(void) {
    if (RGL.debug.font_initialized) {
        RGL_UnloadBitmapFont(RGL.debug.font);
        RGL.debug.font_initialized = false;
    }
}

/**
 * @brief (INTERNAL) Renders a line of debug text on the screen using the embedded bitmap font.
 * This is the key internal drawing function used by the performance overlay and other debug tools.
 *
 * @param text The string to render. Supports newline characters.
 * @param x The top-left screen-space X coordinate.
 * @param y The top-left screen-space Y coordinate.
 * @param size The height of each character in pixels. Width is scaled proportionally.
 * @param color The color to tint the text.
 */
static void _RGL_DrawDebugText(const char* text, int x, int y, int size, Color color) {
    if (!RGL.debug.font_initialized) {
        if (!_RGL_InitDebugTextSystem()) return;
    }
    
    // Get the debug font from the state
    RGLBitmapFont font = RGL.debug.font;

    int start_x = x;
    // Calculate scale based on desired size vs. native font height
    float scale = (float)size / (float)font.char_height;
    vec2 char_size = { font.char_width * scale, font.char_height * scale };

    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            y += char_size[1];
            x = start_x;
            continue;
        }
        int char_index = (unsigned char)*p;
        
        // Calculate source rectangle in the font atlas
        Rectangle src_rect = {
            (float)((char_index % font.chars_per_row) * font.char_width),
            (float)((char_index / font.chars_per_row) * font.char_height),
            (float)font.char_width,
            (float)font.char_height
        };
        
        RGLSprite glyph_sprite = { font.atlas_texture, src_rect };
        Color colors[4] = {color, color, color, color};

        // Use the master drawing function to draw the character at the correct scaled size
        RGL_DrawSpritePro(
            glyph_sprite,
            (vec3){(float)x, (float)y, 0.0f},
            char_size,
            (vec2){0.0f, 0.0f}, // Top-left origin
            (vec3){0.0f, 0.0f, 0.0f}, // No rotation
            (vec2){0.0f, 0.0f}, // No skew
            colors,
            NULL
        );
        x += char_size[0];
    }
}

/**
 * @brief Draws a real-time performance overlay with detailed rendering statistics.
 * This provides developers with crucial, at-a-glance information for profiling
 * and optimization. It uses a placeholder function for text rendering which must
 * be replaced by a proper font system.
 */
SITAPI void RGL_DrawPerformanceOverlay(void) {
    if (!RGL.is_initialized || !RGL.is_batching) return;
    
    // --- 1. Update Statistics ---
    // Use a static variable to path the time of the last stats update.
    static double last_stats_update_time = 0.0;
    static double time_since_last_update = 0.0;
    
    // Use the engine's master timer for frame time calculation.
    double current_time = SituationTimerGetTime();
    RGL.stats.last_frame_time_ms = (float)((current_time - last_stats_update_time) * 1000.0);
    time_since_last_update += (current_time - last_stats_update_time);
    last_stats_update_time = current_time;
    
    // --- 2. Switch to a 2D Camera for UI Rendering ---
    // This ensures the overlay is drawn flat on the screen, on top of the 3D scene.
    int screen_w, screen_h;
    SituationGetVirtualDisplaySize(RGL.active_virtual_display_id, &screen_w, &screen_h);
    
    // Flush any 3D commands and set up a 2D orthographic camera.
    _RGL_FlushBatch();
    RGL_SetCamera2D((vec2){screen_w / 2.0f, screen_h / 2.0f}, 0.0f, 1.0f);

    // --- 3. Define Overlay Layout ---
    const int FONT_SIZE = 14;
    const int PADDING = 10;
    const int LINE_HEIGHT = FONT_SIZE + 4;
    const int PANEL_WIDTH = 280;
    const int PANEL_HEIGHT = 160;
    const int START_X = PADDING;
    const int START_Y = PADDING;
    Color text_color = {220, 220, 220, 255}; // Light gray
    Color value_color = {100, 255, 100, 255}; // Bright green
    Color warn_color = {255, 255, 100, 255}; // Yellow
    Color bad_color = {255, 100, 100, 255}; // Red
    char buffer[128];

    // --- 4. Draw Background Panel ---
    RGL_DrawRectangle((Rectangle){(float)START_X, (float)START_Y, (float)PANEL_WIDTH, (float)PANEL_HEIGHT}, 0.0f, (Color){20, 20, 20, 200});

    // --- 5. Render Statistics Text ---
    int current_y = START_Y + PADDING;

    // --- CPU/GPU Timing ---
    float fps = (RGL.stats.last_frame_time_ms > 0.0f) ? 1000.0f / RGL.stats.last_frame_time_ms : 0.0f;
    snprintf(buffer, sizeof(buffer), "FPS: %.1f", fps);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, fps > 50 ? value_color : (fps > 30 ? warn_color : bad_color));
    
    snprintf(buffer, sizeof(buffer), "Frame: %.2f ms", RGL.stats.last_frame_time_ms);
    _RGL_DrawDebugText(buffer, START_X + PADDING + 120, current_y, FONT_SIZE, text_color);
    current_y += LINE_HEIGHT;

    // --- Batching & Draw Calls ---
    snprintf(buffer, sizeof(buffer), "Draw Calls: %llu", RGL.stats.total_draw_calls);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    
    snprintf(buffer, sizeof(buffer), "Flushes: %llu", RGL.stats.batch_flushes);
    _RGL_DrawDebugText(buffer, START_X + PADDING + 120, current_y, FONT_SIZE, RGL.stats.batch_flushes > 5 ? warn_color : text_color);
    current_y += LINE_HEIGHT;

    // --- Vertex & Primitive Data ---
    snprintf(buffer, sizeof(buffer), "Vertices: %llu", RGL.stats.total_vertices_drawn);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    current_y += LINE_HEIGHT;
    
    uint64_t triangles = RGL.stats.total_vertices_drawn / 3;
    snprintf(buffer, sizeof(buffer), "Triangles: %llu", triangles);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    current_y += LINE_HEIGHT;
    
    // --- Batch Efficiency ---
    float efficiency = (RGL.stats.total_draw_calls > 0) ? (float)RGL.stats.total_vertices_drawn / RGL.stats.total_draw_calls : 0.0f;
    snprintf(buffer, sizeof(buffer), "V/Call: %.1f", efficiency);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, efficiency > 100 ? value_color : (efficiency > 50 ? warn_color : bad_color));
    current_y += LINE_HEIGHT;

    // --- Memory Usage ---
    float cmd_mem_kb = (RGL.command_capacity * sizeof(RGLInternalDraw)) / 1024.0f;
    float vbo_mem_kb = (RGL.cpu_vertex_buffer_floats_capacity * sizeof(float)) / 1024.0f;
    snprintf(buffer, sizeof(buffer), "Buffer Mem: %.1f KB", cmd_mem_kb + vbo_mem_kb);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    current_y += LINE_HEIGHT;
    
    float usage_pct = (RGL.command_capacity > 0) ? ((float)RGL.command_count / RGL.command_capacity) * 100.0f : 0.0f;
    snprintf(buffer, sizeof(buffer), "Buffer Use: %.1f%% (%zu/%zu)", usage_pct, RGL.command_count, RGL.command_capacity);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, usage_pct > 85.0f ? warn_color : text_color);

    current_y += LINE_HEIGHT;
    snprintf(buffer, sizeof(buffer), "Downward Shad: %d", RGL.stats.downward_shadows_drawn);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    current_y += LINE_HEIGHT;
    snprintf(buffer, sizeof(buffer), "Stencil Shad: %d", RGL.stats.stencil_volumes_drawn);
    _RGL_DrawDebugText(buffer, START_X + PADDING, current_y, FONT_SIZE, text_color);
    
    // --- 6. Reset Per-Frame Stats and Increment Frame Counter ---
    // Only reset stats every frame to show the data for the *previous* complete frame.
    if (time_since_last_update >= 1.0/60.0) { // Update roughly every frame
        RGL.stats.total_draw_calls = 0;
        RGL.stats.total_vertices_drawn = 0;
        RGL.stats.batch_flushes = 0;
        time_since_last_update = 0.0;
    }
    RGL.stats.frames_rendered++;
}

SITAPI void RGL_DrawShadowVolumeDebug(vec3 world_pos, vec2 size, const RGLShadowConfig* config) {
    if (!RGL.is_batching || !config) return;
    
    // ... (copy the entire "Find the Light Source" block from RGL_DrawSpriteWithShadow) ...
    RGLLight* light = &RGL.lights[config->light_id -1]; // Simplified

    _RGL_FlushBatch();

    // --- State setup for DEBUG visualization ---
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Don't write to depth, so we can see through it
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

// -- Part A: Create the sprite quad facing the camera
    vec3 caster_verts[4];
    vec3 right, up;

    // CORRECT, NON-MUTATING WAY to get camera vectors:
    // The first column of the view matrix is the camera's right vector in world space.
    // The second column is the camera's up vector.
    right[0] = RGL.current_view_matrix[0][0]; right[1] = RGL.current_view_matrix[1][0]; right[2] = RGL.current_view_matrix[2][0];
    up[0]    = RGL.current_view_matrix[0][1]; up[1]    = RGL.current_view_matrix[1][1]; up[2]    = RGL.current_view_matrix[2][1];

    glm_vec3_normalize(right);
    glm_vec3_normalize(up);
    
    glm_vec3_scale(right, size[0] * 0.5f, right);
    glm_vec3_scale(up, size[1] * 0.5f, up);

    // Calculate the 4 corners of the camera-facing quad
    vec3 temp;
    glm_vec3_sub(world_pos, right, temp); glm_vec3_sub(temp, up, caster_verts[0]); // bottom-left
    glm_vec3_add(world_pos, right, temp); glm_vec3_sub(temp, up, caster_verts[1]); // bottom-right
    glm_vec3_add(world_pos, right, temp); glm_vec3_add(temp, up, caster_verts[2]); // top-right
    glm_vec3_sub(world_pos, right, temp); glm_vec3_add(temp, up, caster_verts[3]); // top-left

    // -- Part B: Extrude the quad's vertices away from the light source
    vec3 extruded_verts[4];
    for(int i = 0; i < 4; i++) {
        vec3 dir;
        glm_vec3_sub(caster_verts[i], light->position, dir);
        glm_vec3_normalize(dir);
        glm_vec3_scale(dir, config->extrusion_length, dir);
        // Note: We use the original caster_verts to calculate the direction, then add to it.
        glm_vec3_add(caster_verts[i], dir, extruded_verts[i]);
    }

    // -- Part C: Build the final, closed volume mesh (12 triangles)
    // The winding order (CW/CCW) is critical for the front/back stencil ops to work.
    vec3 volume_mesh[] = {
        // Sides of the volume (4 quads = 8 triangles)
        // TYPO FIXES ARE HERE: The second triangle of each quad was incorrect.
        
        // Bottom side quad
        caster_verts[0], extruded_verts[0], extruded_verts[1],
        caster_verts[0], extruded_verts[1], caster_verts[1],

        // Right side quad
        caster_verts[1], extruded_verts[1], extruded_verts[2],
        caster_verts[1], extruded_verts[2], caster_verts[2],

        // Top side quad
        caster_verts[2], extruded_verts[2], extruded_verts[3],
        caster_verts[2], extruded_verts[3], caster_verts[3],

        // Left side quad
        caster_verts[3], extruded_verts[3], extruded_verts[0],
        caster_verts[3], extruded_verts[0], caster_verts[0],
        
        // Back Cap of the volume (2 triangles, facing away from light)
        // This closes the volume to prevent light leaking. Winding is crucial.
        extruded_verts[0], extruded_verts[2], extruded_verts[1],
        extruded_verts[0], extruded_verts[3], extruded_verts[2]
    };
    
    // --- Render the Volume VISIBLY ---
    glUseProgram(RGL.shadow_darken_shader.gl_program_id); // Re-use this simple shader
    glUniform4f(RGL.loc_sd_shadow_color, 1.0f, 0.0f, 0.5f, 0.25f); // Pink, 25% transparent
    // We need to pass the matrices to this shader too, so we'd need to add MVP to it.
    // A better way is to use the wireframe shader.
    glUseProgram(RGL.debug.wireframe_shader);
    mat4 mvp;
    glm_mat4_mul(RGL.current_projection_matrix, RGL.current_view_matrix, mvp);
    glUniformMatrix4fv(RGL.debug.wireframe_mvp_loc, 1, GL_FALSE, (float*)mvp);
    vec4 norm_color; SituationConvertColorToVec4((Color){255,0,255,255}, norm_color);
    glUniform4fv(RGL.debug.wireframe_color_loc, 1, norm_color);

    glBindVertexArray(RGL.batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, RGL.batch_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(volume_mesh), volume_mesh, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // RENDER AS WIREFRAME
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore fill mode

    // --- Final Cleanup ---
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(RGL.main_shader.gl_program_id);
}

SITAPI RGLTexture RGL_LoadTextureWithParams(const char* filename, const RGLTextureParams* params) {
    if (!params) {
        // Handle case where NULL is passed
        return LTLoadTexture(filename, LT_WRAP_REPEAT, LT_FILTER_LINEAR_MIPMAP_LINEAR);
    }
    
    // Convert the old RGL params to the new LT params
    LTTextureParams lt_params = {
        .format = LT_FORMAT_RGBA8, // RGL only supported basic formats
        .wrap_s = params->wrap_s,
        .wrap_t = params->wrap_t,
        .filter_min = params->min_filter,
        .filter_mag = params->mag_filter,
        .generate_mipmaps = params->generate_mipmaps,
        .anisotropic_level = 0
    };
    return LTLoadTextureWithParams(filename, &LT_params);
}

SITAPI RGLTexture RGL_LoadTexture(const char* filename, GLenum wrap_mode, GLenum filter_mode) {
    // This function can now call the simpler LTLoadTexture directly
    return LTLoadTexture(filename, (LTWrapMode)wrap_mode, (LTFilterMode)filter_mode);
    // NOTE: This direct cast works if the enums have the same underlying values.
    // A safer way is to write a small conversion helper.
}

SITAPI void RGL_UnloadTexture(RGLTexture texture) {
    LTTexture lt_tex = texture; // Cast for clarity
    LTDestroyTexture(&LT_tex);
}

/**
 * @brief Linearly interpolates between two float values.
 * @param a The starting value.
 * @param b The ending value.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated value.
 */
SITAPI float RGL_Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * @brief Clamps a float value between a minimum and maximum.
 * @param value The value to clamp.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @return The clamped value.
 * @note This function is robust and works even if min > max.
 */
SITAPI float RGL_Clamp(float value, float min, float max) {
    if (min > max) {
        float temp = min;
        min = max;
        max = temp;
    }
    const float res = value < min ? min : value;
    return res > max ? max : res;
}

/**
 * @brief Normalizes a value from a given range to the [0, 1] range.
 * @param value The value to normalize.
 * @param start The start of the input range.
 * @param end The end of the input range.
 * @return The normalized value (0.0 to 1.0). Returns 0 if the range has no width.
 */
SITAPI float RGL_Normalize(float value, float start, float end) {
    float width = end - start;
    if (fabsf(width) < FLT_EPSILON) return 0.0f; // Avoid division by zero
    return (value - start) / width;
}

/**
 * @brief Remaps a value from one range to another.
 * @param value The input value.
 * @param input_start The start of the input range.
 * @param input_end The end of the input range.
 * @param output_start The start of the output range.
 * @param output_end The end of the output range.
 * @return The remapped value.
 */
SITAPI float RGL_Remap(float value, float input_start, float input_end, float output_start, float output_end) {
    float t = RGL_Normalize(value, input_start, input_end);
    return RGL_Lerp(output_start, output_end, t);
}

/**
 * @brief Linearly interpolates between two 2D vectors.
 * @param a The starting vector.
 * @param b The ending vector.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated vector.
 */
SITAPI vec2 RGL_Vector2Lerp(vec2 a, vec2 b, float t) {
    vec2 result;
    result[0] = RGL_Lerp(a[0], b[0], t);
    result[1] = RGL_Lerp(a[1], b[1], t);
    return result;
}

/**
 * @brief Rotates a 2D vector by a given angle.
 * @param v The vector to rotate.
 * @param angle_degrees The angle of rotation in degrees.
 * @return The rotated vector.
 */
SITAPI vec2 RGL_Vector2Rotate(vec2 v, float angle_degrees) {
    vec2 result;
    float angle_rads = angle_degrees * (GLM_PI / 180.0f);
    float cos_a = cosf(angle_rads);
    float sin_a = sinf(angle_rads);
    
    result[0] = v[0] * cos_a - v[1] * sin_a;
    result[1] = v[0] * sin_a + v[1] * cos_a;
    
    return result;
}

/**
 * @brief Calculates the angle of a 2D vector.
 * @param v The input vector.
 * @return The angle in degrees, from -180 to 180.
 *         (Angle of {1,0} is 0 degrees).
 */
SITAPI float RGL_Vector2Angle(vec2 v) {
    // atan2f handles all quadrants correctly and avoids division by zero
    float angle_rads = atan2f(v[1], v[0]);
    return angle_rads * (180.0f / GLM_PI);
}

/**
 * @brief Helper function to draw simple procedural characters
 */
static void _RGL_DrawSimpleChar(unsigned char* atlas_data, int atlas_width, int atlas_height, 
                               int char_x, int char_y, int char_width, int char_height, int char_code) {
    // Simple procedural character generation for demo purposes
    // In a real implementation, you'd use FreeType here
    
    auto set_pixel = [&](int x, int y, unsigned char value) {
        if (x >= 0 && x < atlas_width && y >= 0 && y < atlas_height) {
            int idx = (y * atlas_width + x) * 4;
            atlas_data[idx + 0] = value; // R
            atlas_data[idx + 1] = value; // G  
            atlas_data[idx + 2] = value; // B
            atlas_data[idx + 3] = value; // A
        }
    };
    
    // Draw a simple rectangle outline for most characters
    if (char_code >= 33 && char_code <= 126) {
        // Top and bottom lines
        for (int x = 1; x < char_width - 1; x++) {
            set_pixel(char_x + x, char_y + 1, 255);
            set_pixel(char_x + x, char_y + char_height - 2, 255);
        }
        // Left and right lines
        for (int y = 1; y < char_height - 1; y++) {
            set_pixel(char_x + 1, char_y + y, 255);
            set_pixel(char_x + char_width - 2, char_y + y, 255);
        }
        
        // Add some character-specific details
        switch (char_code) {
            case 'A':
                // Horizontal line in middle
                for (int x = 2; x < char_width - 2; x++) {
                    set_pixel(char_x + x, char_y + char_height / 2, 255);
                }
                break;
            case 'O':
                // Fill the rectangle to make it solid
                for (int y = 2; y < char_height - 2; y++) {
                    for (int x = 2; x < char_width - 2; x++) {
                        set_pixel(char_x + x, char_y + y, 255);
                    }
                }
                break;
            // Add more character-specific patterns as needed
        }
    }
}

/**
 * @brief Creates a bitmap font from a texture atlas
 */
SITAPI RGLBitmapFont RGL_LoadBitmapFont(const char* texture_filepath, int char_width, int char_height, int first_char) {
    RGLBitmapFont font = {0};
    
    font.atlas_texture = RGL_LoadTexture(texture_filepath, GL_CLAMP_TO_EDGE, GL_NEAREST); // Pixel-perfect
    if (font.atlas_texture.id == 0) return font;
    
    font.char_width = char_width;
    font.char_height = char_height;
    font.chars_per_row = font.atlas_texture.width / char_width;
    font.chars_per_col = font.atlas_texture.height / char_height;
    font.first_char = first_char;
    font.char_count = font.chars_per_row * font.chars_per_col;
    font.char_spacing = 0.0f;
    font.line_spacing = 0.0f;
    
    return font;
}

/**
 * @brief Creates a bitmap font by rendering a system font to a texture atlas
 * Perfect for creating custom fonts from TTF files or system fonts
 */
SITAPI RGLBitmapFont RGL_CreateBitmapFontFromSystemFont(const char* font_name, int font_size, int char_width, int char_height) {
    RGLBitmapFont font = {0};
    
    // This would require a font rendering library like FreeType or stb_truetype
    // For now, let's create a placeholder that generates a simple font
    
    int atlas_width = 16 * char_width;   // 16 chars per row
    int atlas_height = 16 * char_height; // 16 rows (256 chars)
    
    // Allocate RGBA data for the atlas
    unsigned char* atlas_data = calloc(atlas_width * atlas_height * 4, 1);
    
    // TODO: This is where you'd use FreeType to render each character
    // For now, create a simple procedural font
    for (int char_code = 32; char_code < 127; char_code++) {
        int char_x = ((char_code - 32) % 16) * char_width;
        int char_y = ((char_code - 32) / 16) * char_height;
        
        // Draw a simple representation of each character
        _RGL_DrawSimpleChar(atlas_data, atlas_width, atlas_height, char_x, char_y, char_width, char_height, char_code);
    }
    
    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas_width, atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    free(atlas_data);
    
    font.atlas_texture.id = texture_id;
    font.atlas_texture.width = atlas_width;
    font.atlas_texture.height = atlas_height;
    font.char_width = char_width;
    font.char_height = char_height;
    font.chars_per_row = 16;
    font.chars_per_col = 16;
    font.first_char = 32; // Start with space character
    font.char_count = 95;  // Printable ASCII characters
    font.char_spacing = 1.0f;
    font.line_spacing = 2.0f;
    
    return font;
}

/**
 * @brief Loads a TrueType font and pre-renders it into a GPU texture atlas for high-speed drawing.
 * @details This function performs the one-time, heavyweight task of parsing a .ttf file and rasterizing
 *          a set of characters (printable ASCII 32-126) into a single bitmap. This bitmap is then
 *          uploaded directly to the GPU as an OpenGL texture. All necessary character metrics (position,
 *          offsets, advance widths) are "baked" into the RGLTrueTypeFont struct for use by RGL_DrawTextTTF.
 * @param font_path The file path to the .ttf font file.
 * @param font_size The pixel height to render the characters at. This determines the quality of the font.
 * @return An `RGLTrueTypeFont` struct ready for use. On failure, the `atlas_texture.id` will be 0.
 * @note This function requires `stb_truetype.h` and performs direct file I/O and OpenGL calls.
 */
SITAPI RGLTrueTypeFont RGL_LoadTrueTypeFont(const char* font_path, float font_size) {
    RGLTrueTypeFont font = {0};

    // --- 1. Load the font file directly from disk ---
    FILE* font_file = fopen(font_path, "rb");
    if (!font_file) {
        // We can still use Situation's error reporting if rgl.h has access to it.
        _SituationSetErrorFromCode(SITUATION_ERROR_FILE_ACCESS, "Could not open font file");
        return font;
    }
    
    fseek(font_file, 0, SEEK_END);
    long file_size = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);
    
    unsigned char* font_data = (unsigned char*)malloc(file_size);
    if (!font_data) {
        fclose(font_file);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for font file");
        return font;
    }
    fread(font_data, 1, file_size, font_file);
    fclose(font_file);
    
    // --- 2. Prepare for baking ---
    const int atlas_width = 1024; // Use a larger atlas for better quality/more chars
    const int atlas_height = 1024;
    const int first_char = 32;    // Start at ASCII ' '

    // These buffers are temporary for the baking process.
    unsigned char* atlas_bitmap = (unsigned char*)calloc(atlas_width * atlas_height, sizeof(unsigned char));
    stbtt_bakedchar* baked_chars = (stbtt_bakedchar*)calloc(RGL_FONT_ATLAS_CHAR_COUNT, sizeof(stbtt_bakedchar));

    if (!atlas_bitmap || !baked_chars) {
        free(font_data);
        free(atlas_bitmap);
        free(baked_chars);
        _SituationSetErrorFromCode(SITUATION_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for font baking");
        return font;
    }

    // --- 3. The Core Operation: Bake the font into the bitmap using stb_truetype ---
    // This function rasterizes the characters and packs them into the atlas_bitmap.
    int result = stbtt_BakeFontBitmap(
        font_data, 0, font_size,
        atlas_bitmap, atlas_width, atlas_height,
        first_char, RGL_FONT_ATLAS_CHAR_COUNT,
        baked_chars
    );

    if (result <= 0) {
        // This means the atlas was too small to fit all characters at the requested size.
        free(font_data);
        free(atlas_bitmap);
        free(baked_chars);
        _SituationSetErrorFromCode(SITUATION_ERROR_GENERAL, "Failed to bake font to bitmap. Atlas may be too small or font size too large.");
        return font;
    }

    // --- 4. Upload the 1-channel bitmap directly to the GPU as an OpenGL texture ---
    // The original code correctly used GL_R8 (one red 8-bit channel) which is perfect for this.
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // Tell OpenGL how to unpack the pixel data (it's tightly packed).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas_width, atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_bitmap);
    
    // Set texture parameters. Linear filtering gives softer edges than Nearest.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Restore default pixel alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // The CPU-side bitmap is no longer needed after GPU upload.
    free(atlas_bitmap);

    // --- 5. Populate the RGLTrueTypeFont struct with the final data ---
    font.atlas_texture.id = texture_id;
    font.atlas_texture.width = atlas_width;
    font.atlas_texture.height = atlas_height;
    font.font_size = font_size;
    font.first_char = first_char;

    // Get proper line height metrics using stb_truetype's font info struct.
    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_data, 0);
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
    float scale = stbtt_ScaleForPixelHeight(&font_info, font_size);
    font.line_height = (ascent - descent + line_gap) * scale;
    
    // Copy the character metadata from stb's temporary format to our permanent arrays.
    for (int i = 0; i < RGL_FONT_ATLAS_CHAR_COUNT; ++i) {
        stbtt_bakedchar* bc = &baked_chars[i];
        
        font.char_rects[i] = (Rectangle){(float)bc->x0, (float)bc->y0, (float)(bc->x1 - bc->x0), (float)(bc->y1 - bc->y0)};
        font.char_offsets[i] = (vec2){bc->xoff, bc->yoff};
        font.char_advances[i] = bc->xadvance;
    }
    
    // Now that all data is extracted, we can free the raw font file data and the baked char data.
    free(font_data);
    free(baked_chars);
    
    return font;
}

/**
 * @brief Stamps text directly to a texture for caching/effects
 * Perfect for UI elements, signs, or pre-rendered text
 */
SITAPI RGLTexture RGL_StampTextToTexture(const char* text, RGLBitmapFont font, Color text_color, Color bg_color, int* out_width, int* out_height) {
    if (!text || !RGL.is_initialized) {
        *out_width = *out_height = 0;
        return (RGLTexture){0};
    }
    
    // Calculate text dimensions
    vec2 text_size = RGL_MeasureText(text, font);
    int tex_width = (int)text_size[0] + 4; // 2px padding each side
    int tex_height = (int)text_size[1] + 4;
    
    // Create render texture
    RGLTexture render_target = RGL_CreateRenderTexture(tex_width, tex_height);
    if (render_target.id == 0) {
        *out_width = *out_height = 0;
        return (RGLTexture){0};
    }
    
    // Render text to texture
    RGL_SetRenderTarget(render_target);
    
    // Set up orthographic camera for the texture size
    RGL_Begin(-1); // Use internal rendering
    RGL_SetCamera2D((vec2){tex_width/2.0f, tex_height/2.0f}, 0.0f, 1.0f);
    
    // Clear with background color
    if (bg_color.a > 0) {
        RGL_DrawRectangle((Rectangle){0, 0, tex_width, tex_height}, 0.0f, bg_color);
    }
    
    // Draw text
    RGL_DrawText(text, (vec2){2, 2}, font, text_color); // 2px padding
    
    RGL_End();
    RGL_ResetRenderTarget();
    
    *out_width = tex_width;
    *out_height = tex_height;
    return render_target;
}

/**
 * @brief Advanced text-to-texture with word wrapping and formatting
 */
SITAPI RGLTexture RGL_StampTextToTextureAdvanced(const char* text, RGLTrueTypeFont font, Color text_color, Color bg_color, float wrap_width, int* out_width, int* out_height) {
    if (!text || font.atlas_texture.id == 0) {
        *out_width = *out_height = 0;
        return (RGLTexture){0};
    }
    
    // Calculate wrapped text dimensions
    vec2 text_size = RGL_MeasureTextTTF(text, font);
    
    // If wrap_width is specified, calculate wrapped dimensions
    if (wrap_width > 0 && text_size[0] > wrap_width) {
        int line_count = RGL_GetTextLineCount(text, (RGLBitmapFont){0}, wrap_width); // Simplified
        text_size[0] = wrap_width;
        text_size[1] = line_count * font.line_height;
    }
    
    int tex_width = (int)text_size[0] + 8;  // 4px padding each side
    int tex_height = (int)text_size[1] + 8;
    
    // Create render texture
    RGLTexture render_target = RGL_CreateRenderTexture(tex_width, tex_height);
    if (render_target.id == 0) {
        *out_width = *out_height = 0;
        return (RGLTexture){0};
    }
    
    // Render to texture
    RGL_SetRenderTarget(render_target);
    RGL_Begin(-1);
    RGL_SetCamera2D((vec2){tex_width/2.0f, tex_height/2.0f}, 0.0f, 1.0f);
    
    // Clear background
    if (bg_color.a > 0) {
        RGL_DrawRectangle((Rectangle){0, 0, tex_width, tex_height}, 0.0f, bg_color);
    }
    
    // Draw text with wrapping
    RGL_DrawTextTTF(text, (vec2){4, 4}, font, text_color);
    
    RGL_End();
    RGL_ResetRenderTarget();
    
    *out_width = tex_width;
    *out_height = tex_height;
    return render_target;
}

/**
 * @brief Creates a terminal bitmap font from raw pixel data in memory
 * 
 * @param font_data Pointer to const pixel data (1 byte per pixel, 0=transparent, 255=opaque)
 * @param char_width Width of each character in pixels
 * @param char_height Height of each character in pixels  
 * @param char_count Number of characters in the font data
 * @param chars_per_row How many characters are arranged horizontally in the source data
 * @param first_char ASCII code of the first character (usually 0 or 32)
 * @return RGLBitmapFont ready to use for rendering
 */
 SITAPI RGLBitmapFont RGL_CreateTerminalFont(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char) {
    RGLBitmapFont font = {0};
    
    if (!font_data || char_width <= 0 || char_height <= 0 || char_count <= 0 || chars_per_row <= 0) {  _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid parameters for terminal font creation"); return font; }
    
    // Calculate source dimensions
    int chars_per_col = (char_count + chars_per_row - 1) / chars_per_row; // Ceiling division
    int source_width = chars_per_row * char_width;
    int source_height = chars_per_col * char_height;
    
    // Create OpenGL texture atlas
    // We'll arrange characters in a 16x16 grid for optimal GPU access
    const int ATLAS_CHARS_PER_ROW = 16;
    const int ATLAS_CHARS_PER_COL = 16;
    int atlas_width = ATLAS_CHARS_PER_ROW * char_width;
    int atlas_height = ATLAS_CHARS_PER_COL * char_height;
    
    // Allocate atlas data (RGBA format for maximum compatibility)
    unsigned char* atlas_data = calloc(atlas_width * atlas_height * 4, 1);
    
    // Copy characters from source data to atlas
    for (int char_idx = 0; char_idx < char_count && char_idx < 256; char_idx++) {
        // Calculate source position
        int src_char_x = (char_idx % chars_per_row) * char_width;
        int src_char_y = (char_idx / chars_per_row) * char_height;
        
        // Calculate atlas position
        int atlas_char_x = (char_idx % ATLAS_CHARS_PER_ROW) * char_width;
        int atlas_char_y = (char_idx / ATLAS_CHARS_PER_ROW) * char_height;
        
        // Copy character pixel by pixel
        for (int y = 0; y < char_height; y++) {
            for (int x = 0; x < char_width; x++) {
                // Source pixel
                int src_pixel_idx = (src_char_y + y) * source_width + (src_char_x + x);
                unsigned char src_pixel = font_data[src_pixel_idx];
                
                // Atlas pixel (RGBA)
                int atlas_pixel_idx = ((atlas_char_y + y) * atlas_width + (atlas_char_x + x)) * 4;
                atlas_data[atlas_pixel_idx + 0] = src_pixel; // R
                atlas_data[atlas_pixel_idx + 1] = src_pixel; // G
                atlas_data[atlas_pixel_idx + 2] = src_pixel; // B
                atlas_data[atlas_pixel_idx + 3] = src_pixel; // A
            }
        }
    }
    
    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas_width, atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
    
    // Set texture parameters for pixel-perfect rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Clean up temporary data
    free(atlas_data);
    
    // Fill out font structure
    font.atlas_texture.id = texture_id;
    font.atlas_texture.width = atlas_width;
    font.atlas_texture.height = atlas_height;
    font.atlas_texture.wrap_mode = GL_CLAMP_TO_EDGE;
    font.atlas_texture.filter_mode = GL_NEAREST;
    font.char_width = char_width;
    font.char_height = char_height;
    font.chars_per_row = ATLAS_CHARS_PER_ROW;
    font.chars_per_col = ATLAS_CHARS_PER_COL;
    font.first_char = first_char;
    font.char_count = char_count;
    font.char_spacing = 0.0f; // Terminal fonts typically have no extra spacing
    font.line_spacing = 0.0f;
    
    return font;
}

/**
 * @brief Convenience function for CP437 terminal fonts (common 8x16 format)
 */
SITAPI RGLBitmapFont RGL_CreateCP437Font(const unsigned char* font_data_8x16) {
    // Standard CP437 font: 256 characters, 8x16 pixels each, arranged in 16x16 grid
    return RGL_CreateTerminalFont(font_data_8x16, 8, 16, 256, 16, 0);
}

/**
 * @brief Convenience function for standard ASCII terminal fonts
 */
SITAPI RGLBitmapFont RGL_CreateASCIIFont(const unsigned char* font_data, int char_width, int char_height) {
    // Standard ASCII: 95 printable characters starting from space (32)
    return RGL_CreateTerminalFont(font_data, char_width, char_height, 95, 16, 32);
}

/**
 * @brief Creates a terminal font with custom character mapping
 */
SITAPI RGLBitmapFont RGL_CreateTerminalFontEx(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char, float char_spacing, float line_spacing) {
    RGLBitmapFont font = RGL_CreateTerminalFont(font_data, char_width, char_height, char_count, chars_per_row, first_char);
    
    // Apply custom spacing
    font.char_spacing = char_spacing;
    font.line_spacing = line_spacing;
    
    return font;
}

/**
 * @brief Creates a bitmap font from packed data with flexible configuration
 */
SITAPI RGLBitmapFont RGL_CreatePackedBitmapFont(const void* packed_data, const RGLPackedFontConfig* config) {
    if (!config) {
        RGLBitmapFont empty = {0};
        return empty;
    }
    
    // Create a copy of the config with outline disabled
    RGLPackedFontConfig no_outline_config = *config;
    outline_config.enable_outline = false;
    
    // Call the main outline-capable function
    return RGL_CreateOutlinedPackedBitmapFont(packed_data, &outline_config);
}

/**
 * @brief Creates a bitmap font from packed data with outline support
 */
SITAPI RGLBitmapFont RGL_CreateOutlinedPackedBitmapFont(const void* packed_data, const RGLPackedFontConfigEx* config) {
    RGLBitmapFont font = {0};
    
    if (!packed_data || !config || config->char_width <= 0 || config->char_height <= 0 || config->char_count <= 0) {
        _SituationSetErrorFromCode(SITUATION_ERROR_INVALID_PARAM, "Invalid parameters for outlined packed bitmap font creation");
        return font;
    }
    
    // Auto-configure atlas layout if not specified
    int atlas_chars_per_row = config->atlas_chars_per_row > 0 ? config->atlas_chars_per_row : 16;
    int atlas_chars_per_col = config->atlas_chars_per_col > 0 ? config->atlas_chars_per_col : 
                              (config->char_count + atlas_chars_per_row - 1) / atlas_chars_per_row;
    
    // Calculate dimensions
    int display_width = config->char_width + config->left_padding + config->right_padding;
    int display_height = config->display_height > 0 ? config->display_height : 
                         config->char_height + config->top_padding + config->bottom_padding;
    
    int atlas_width = atlas_chars_per_row * display_width;
    int atlas_height = atlas_chars_per_col * display_height;
    
    // Allocate temporary single-channel data for outline processing
    unsigned char* temp_data = calloc(atlas_width * atlas_height, 1);
    unsigned char* atlas_data = calloc(atlas_width * atlas_height * 4, 1);
    
    if (!temp_data || !atlas_data) {
        free(temp_data);
        free(atlas_data);
        _SituationSetErrorFromCode(SITUATION_ERROR_OUT_OF_MEMORY, "Failed to allocate atlas data");
        return font;
    }
    
    // Determine data type size
    int bytes_per_entry = (config->bits_per_row + 7) / 8;
    const unsigned char* data_bytes = (const unsigned char*)packed_data;
    
    // First pass: Extract font data to temp buffer
    for (int char_idx = 0; char_idx < config->char_count; char_idx++) {
        // Calculate atlas position
        int atlas_char_x = (char_idx % atlas_chars_per_row) * display_width;
        int atlas_char_y = (char_idx / atlas_chars_per_row) * display_height;
        
        // Calculate source character position
        int src_char_x = (char_idx % config->chars_per_row);
        int src_char_y = (char_idx / config->chars_per_row);
        int src_char_base_idx = (src_char_y * config->chars_per_row + src_char_x) * config->char_height;
        
        // Extract character to temp buffer
        for (int display_row = 0; display_row < display_height; display_row++) {
            for (int display_col = 0; display_col < display_width; display_col++) {
                unsigned char pixel = 0;
                
                // Check if we're in the actual font data area
                bool in_data_area = (display_row >= config->top_padding && 
                                   display_row < display_height - config->bottom_padding &&
                                   display_col >= config->left_padding && 
                                   display_col < display_width - config->right_padding);
                
                if (in_data_area) {
                    int font_row = display_row - config->top_padding;
                    int font_col = display_col - config->left_padding;
                    
                    // Get the packed row data
                    int row_data_idx = (src_char_base_idx + font_row) * bytes_per_entry;
                    
                    // Extract the row value based on data size
                    uint32_t row_data = 0;
                    for (int b = 0; b < bytes_per_entry; b++) {
                        row_data |= ((uint32_t)data_bytes[row_data_idx + b]) << (b * 8);
                    }
                    
                    // Apply bit offset to align data
                    row_data >>= config->data_bit_offset;
                    
                    // Extract the specific bit for this pixel
                    int bit_pos;
                    if (config->bit_order_msb_first) {
                        bit_pos = (config->data_bits - 1) - font_col;
                    } else {
                        bit_pos = font_col;
                    }
                    
                    if (bit_pos >= 0 && bit_pos < config->data_bits) {
                        pixel = (row_data & (1U << bit_pos)) ? 255 : 0;
                    }
                }
                
                // Store in temp buffer
                int temp_idx = (atlas_char_y + display_row) * atlas_width + (atlas_char_x + display_col);
                temp_data[temp_idx] = pixel;
            }
        }
    }
    
    // Second pass: Generate outlined RGBA atlas with tile boundary protection
    for (int y = 0; y < atlas_height; y++) {
        for (int x = 0; x < atlas_width; x++) {
            int atlas_idx = (y * atlas_width + x) * 4;
            int temp_idx = y * atlas_width + x;
            
            // Determine which character tile we're in
            int tile_x = x / display_width;
            int tile_y = y / display_height;
            int tile_local_x = x % display_width;
            int tile_local_y = y % display_height;
            
            // Calculate tile boundaries
            int tile_left = tile_x * display_width;
            int tile_right = tile_left + display_width - 1;
            int tile_top = tile_y * display_height;
            int tile_bottom = tile_top + display_height - 1;
            
            unsigned char pixel = temp_data[temp_idx];
            bool is_outline = false;
            
            // If outline is enabled and this pixel is transparent, check for nearby font pixels
            if (config->enable_outline && pixel == 0) {
                int thickness = config->outline_thickness;
                
                // Check surrounding pixels for font data (within tile boundaries)
                for (int dy = -thickness; dy <= thickness && !is_outline; dy++) {
                    for (int dx = -thickness; dx <= thickness && !is_outline; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int check_x = x + dx;
                        int check_y = y + dy;
                        
                        // Ensure we stay within the same tile
                        if (check_x < tile_left || check_x > tile_right || 
                            check_y < tile_top || check_y > tile_bottom) {
                            continue;
                        }
                        
                        // Ensure we stay within atlas bounds
                        if (check_x < 0 || check_x >= atlas_width || 
                            check_y < 0 || check_y >= atlas_height) {
                            continue;
                        }
                        
                        // Check if nearby pixel is font data
                        int check_idx = check_y * atlas_width + check_x;
                        if (temp_data[check_idx] > 0) {
                            // Calculate distance for smoother outlines
                            float dist = sqrtf(dx*dx + dy*dy);
                            if (dist <= thickness) {
                                is_outline = true;
                            }
                        }
                    }
                }
            }
            
            // Set final pixel color
            if (pixel > 0) {
                // Font pixel
                atlas_data[atlas_idx + 0] = config->font_r;
                atlas_data[atlas_idx + 1] = config->font_g;
                atlas_data[atlas_idx + 2] = config->font_b;
                atlas_data[atlas_idx + 3] = config->font_a;
            } else if (is_outline) {
                // Outline pixel
                atlas_data[atlas_idx + 0] = config->outline_r;
                atlas_data[atlas_idx + 1] = config->outline_g;
                atlas_data[atlas_idx + 2] = config->outline_b;
                atlas_data[atlas_idx + 3] = config->outline_a;
            } else {
                // Transparent pixel
                atlas_data[atlas_idx + 0] = 0;
                atlas_data[atlas_idx + 1] = 0;
                atlas_data[atlas_idx + 2] = 0;
                atlas_data[atlas_idx + 3] = 0;
            }
        }
    }
    
    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas_width, atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Cleanup
    free(temp_data);
    free(atlas_data);
    
    // Fill out font structure
    font.atlas_texture.id = texture_id;
    font.atlas_texture.width = atlas_width;
    font.atlas_texture.height = atlas_height;
    font.atlas_texture.wrap_mode = GL_CLAMP_TO_EDGE;
    font.atlas_texture.filter_mode = GL_NEAREST;
    font.char_width = display_width;
    font.char_height = display_height;
    font.chars_per_row = atlas_chars_per_row;
    font.chars_per_col = atlas_chars_per_col;
    font.first_char = config->first_char;
    font.char_count = config->char_count;
    font.char_spacing = 0.0f;
    font.line_spacing = 0.0f;
    
    return font;
}

// Convenience function for VCR OSD font
SITAPI RGLBitmapFont RGL_CreateVCRFont(const uint16_t* font_data) {
    RGLPackedFontConfig config = {
        .char_width = 12,
        .char_height = 14,
        .display_height = 16,
        .char_count = 128,
        .first_char = 0,
        .chars_per_row = 1,               // Linear array of characters
        .bits_per_row = 16,               // uint16_t per row
        .data_bits = 12,                  // 12 bits of actual data
        .data_bit_offset = 0,             // Data is in bits 11-0 (LSB aligned)
        .bit_order_msb_first = true,      // Bit 11 = leftmost pixel
        .top_padding = 1,
        .bottom_padding = 1,
        .left_padding = 0,
        .right_padding = 0,
        .atlas_chars_per_row = 16,
        .atlas_chars_per_col = 8
    };
    
    return RGL_CreatePackedBitmapFont(font_data, &config);
}

// Convenience function for VCR font with black outline
SITAPI RGLBitmapFont RGL_CreateVCRFontWithOutline(const uint16_t* font_data, int outline_thickness) {
    RGLPackedFontConfig config = {
        // Base VCR font config
        .char_width = 12,
        .char_height = 14,
        .display_height = 16,
        .char_count = 128,
        .first_char = 0,
        .chars_per_row = 1,
        .bits_per_row = 16,
        .data_bits = 12,
        .data_bit_offset = 0,
        .bit_order_msb_first = true,
        .top_padding = 1,
        .bottom_padding = 1,
        .left_padding = 2,
        .right_padding = 2,
        .atlas_chars_per_row = 16,
        .atlas_chars_per_col = 8,
        
        // Outline configuration
        .enable_outline = true,
        .outline_thickness = outline_thickness,
        .outline_r = 0,    // Black outline
        .outline_g = 0,
        .outline_b = 0,
        .outline_a = 255,
        .font_r = 255,     // White font
        .font_g = 255,
        .font_b = 255,
        .font_a = 255
    };
    
    return RGL_CreatePackedBitmapFont(font_data, &config);
}

SITAPI RGLBitmapFont RGL_CreateVGA8x8Font(const unsigned char* font_data) {
    RGLPackedFontConfig config = {
        .char_width = 8,
        .char_height = 8,
        .display_height = 10,
        .char_count = 256,
        .first_char = 0,
        .chars_per_row = 1,
        .bits_per_row = 8,
        .data_bits = 8,
        .data_bit_offset = 0,
        .bit_order_msb_first = true,
        .top_padding = 1,
        .bottom_padding = 1,
        .left_padding = 1,
        .right_padding = 1,
        .atlas_chars_per_row = 16,
        .atlas_chars_per_col = 16
    };
    
    return RGL_CreatePackedBitmapFont(font_data, &config);
}

SITAPI RGLBitmapFont RGL_CreateVGA8x8FontWithOutline(const unsigned char* font_data, int outline_thickness) {
    RGLPackedFontConfig config = {
        .char_width = 8,
        .char_height = 8,
        .display_height = 10,
        .char_count = 256,
        .first_char = 0,
        .chars_per_row = 1,
        .bits_per_row = 8,
        .data_bits = 8,
        .data_bit_offset = 0,
        .bit_order_msb_first = true,
        .top_padding = 1,
        .bottom_padding = 1,
        .left_padding = 1,
        .right_padding = 1,
        .atlas_chars_per_row = 16,
        .atlas_chars_per_col = 16
        
        // Outline configuration
        .enable_outline = true,
        .outline_thickness = outline_thickness,
        .outline_r = 0,    // Black outline
        .outline_g = 0,
        .outline_b = 0,
        .outline_a = 255,
        .font_r = 255,     // White font
        .font_g = 255,
        .font_b = 255,
        .font_a = 255
    };
    
    return RGL_CreatePackedBitmapFont(font_data, &config);
}

/**
 * @brief Draws text using a bitmap font
 */
SITAPI void RGL_DrawText(const char* text, vec2 position, RGLBitmapFont font, Color color) {
    if (!text || font.atlas_texture.id == 0) return;
    
    vec2 cursor = {position[0], position[1]};
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cursor[0] = position[0];
            cursor[1] += font.char_height + font.line_spacing;
            continue;
        }
        
        if (*c == '\r') continue; // Skip carriage returns
        
        // Calculate character index in atlas
        int char_index = *c - font.first_char;
        if (char_index < 0 || char_index >= font.char_count) {
            char_index = '?' - font.first_char; // Fallback to '?'
            if (char_index < 0 || char_index >= font.char_count) {
                char_index = 0; // Fallback to first character
            }
        }
        
        // Calculate source rectangle in atlas
        int atlas_x = (char_index % font.chars_per_row) * font.char_width;
        int atlas_y = (char_index / font.chars_per_row) * font.char_height;
        
        Rectangle source_rect = {
            (float)atlas_x, (float)atlas_y,
            (float)font.char_width, (float)font.char_height
        };
        
        // Create sprite and draw
        RGLSprite char_sprite = {font.atlas_texture, source_rect};
        RGL_DrawSprite(char_sprite, cursor, 0.0f, 1.0f, color);
        
        // Advance cursor
        cursor[0] += font.char_width + font.char_spacing;
    }
}

/**
 * @brief Draws TrueType font text
 */
SITAPI void RGL_DrawTextTTF(const char* text, vec2 position, RGLTrueTypeFont font, Color color) {
    if (!text || font.atlas_texture.id == 0) return;
    
    vec2 cursor = {position[0], position[1]};
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cursor[0] = position[0];
            cursor[1] += font.line_height;
            continue;
        }
        
        if (*c == '\r') continue;
        
        unsigned char char_code = (unsigned char)*c;
        if (char_code < 32 || char_code > 126) continue; // Skip non-printable
        
        // Get character rectangle from atlas
        Rectangle char_rect = font.char_rects[char_code];
        vec2 char_offset = font.char_offsets[char_code];
        
        if (char_rect.width > 0 && char_rect.height > 0) {
            vec2 draw_pos = {
                cursor[0] + char_offset[0],
                cursor[1] + char_offset[1]
            };
            
            RGLSprite char_sprite = {font.atlas_texture, char_rect};
            RGL_DrawSprite(char_sprite, draw_pos, 0.0f, 1.0f, color);
        }
        
        cursor[0] += font.char_advances[char_code];
    }
}

/**
 * @brief Extended text drawing with custom spacing
 */
SITAPI void RGL_DrawTextEx(const char* text, vec2 position, RGLBitmapFont font, float spacing, Color color) {
    if (!text || font.atlas_texture.id == 0) return;
    
    // Temporarily modify font spacing
    float original_spacing = font.char_spacing;
    font.char_spacing = spacing;
    
    RGL_DrawText(text, position, font, color);
    
    // Restore original spacing
    font.char_spacing = original_spacing;
}

/**
 * @brief Draws text within a bounding rectangle with optional word wrapping
 */
SITAPI void RGL_DrawTextBoxed(const char* text, Rectangle bounds, RGLBitmapFont font, Color color, bool word_wrap) {
    if (!text || font.atlas_texture.id == 0) return;
    
    vec2 cursor = {bounds.x, bounds.y};
    vec2 word_start = cursor;
    const char* word_start_ptr = text;
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cursor[0] = bounds.x;
            cursor[1] += font.char_height + font.line_spacing;
            if (cursor[1] + font.char_height > bounds.y + bounds.height) break; // Out of bounds
            word_start = cursor;
            word_start_ptr = c + 1;
            continue;
        }
        
        if (*c == '\r') continue;
        
        // Check if we need to wrap
        if (word_wrap && cursor[0] + font.char_width > bounds.x + bounds.width) {
            if (*c == ' ' || word_start_ptr == c) {
                // Wrap at current position
                cursor[0] = bounds.x;
                cursor[1] += font.char_height + font.line_spacing;
                if (cursor[1] + font.char_height > bounds.y + bounds.height) break;
                word_start = cursor;
                word_start_ptr = c + 1;
                continue;
            } else {
                // Wrap at word boundary
                cursor = word_start;
                cursor[1] += font.char_height + font.line_spacing;
                if (cursor[1] + font.char_height > bounds.y + bounds.height) break;
                c = word_start_ptr - 1; // Restart from word beginning
                word_start = cursor;
                continue;
            }
        }
        
        // Path word boundaries
        if (*c == ' ') {
            word_start = cursor;
            word_start[0] += font.char_width + font.char_spacing;
            word_start_ptr = c + 1;
        }
        
        // Draw character if within bounds
        if (cursor[0] >= bounds.x && cursor[0] + font.char_width <= bounds.x + bounds.width &&
            cursor[1] >= bounds.y && cursor[1] + font.char_height <= bounds.y + bounds.height) {
            
            int char_index = *c - font.first_char;
            if (char_index >= 0 && char_index < font.char_count) {
                int atlas_x = (char_index % font.chars_per_row) * font.char_width;
                int atlas_y = (char_index / font.chars_per_row) * font.char_height;
                
                Rectangle source_rect = {
                    (float)atlas_x, (float)atlas_y,
                    (float)font.char_width, (float)font.char_height
                };
                
                RGLSprite char_sprite = {font.atlas_texture, source_rect};
                RGL_DrawSprite(char_sprite, cursor, 0.0f, 1.0f, color);
            }
        }
        
        cursor[0] += font.char_width + font.char_spacing;
    }
}

/**
 * @brief Draws text with a drop shadow
 */
SITAPI void RGL_DrawTextWithShadow(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color shadow_color, vec2 shadow_offset) {
    // Draw shadow first
    vec2 shadow_pos = {position[0] + shadow_offset[0], position[1] + shadow_offset[1]};
    RGL_DrawText(text, shadow_pos, font, shadow_color);
    
    // Draw text on top
    RGL_DrawText(text, position, font, text_color);
}

/**
 * @brief Draws text with an outline effect
 */
SITAPI void RGL_DrawTextWithOutline(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color outline_color, float outline_thickness) {
    // Draw outline in 8 directions
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue; // Skip center
            
            vec2 outline_pos = {
                position[0] + dx * outline_thickness,
                position[1] + dy * outline_thickness
            };
            RGL_DrawText(text, outline_pos, font, outline_color);
        }
    }
    
    // Draw main text
    RGL_DrawText(text, position, font, text_color);
}

/**
 * @brief Draws text with a vertical gradient
 */
SITAPI void RGL_DrawTextGradient(const char* text, vec2 position, RGLBitmapFont font, Color top_color, Color bottom_color) {
    if (!text || font.atlas_texture.id == 0) return;
    
    vec2 text_size = RGL_MeasureText(text, font);
    vec2 cursor = {position[0], position[1]};
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cursor[0] = position[0];
            cursor[1] += font.char_height + font.line_spacing;
            continue;
        }
        
        if (*c == '\r') continue;
        
        // Calculate gradient factor based on Y position
        float gradient_factor = (cursor[1] - position[1]) / text_size[1];
        gradient_factor = fmaxf(0.0f, fminf(1.0f, gradient_factor));
        
        Color char_color = RGL_ColorLerp(top_color, bottom_color, gradient_factor);
        
        // Draw character
        int char_index = *c - font.first_char;
        if (char_index < 0 || char_index >= font.char_count) {
            char_index = '?' - font.first_char;
        }
        
        int atlas_x = (char_index % font.chars_per_row) * font.char_width;
        int atlas_y = (char_index / font.chars_per_row) * font.char_height;
        
        Rectangle source_rect = {
            (float)atlas_x, (float)atlas_y,
            (float)font.char_width, (float)font.char_height
        };
        
        RGLSprite char_sprite = {font.atlas_texture, source_rect};
        RGL_DrawSprite(char_sprite, cursor, 0.0f, 1.0f, char_color);
        
        cursor[0] += font.char_width + font.char_spacing;
    }
}

/**
 * @brief Draws text with a wave effect
 */
SITAPI void RGL_DrawTextWave(const char* text, vec2 position, RGLBitmapFont font, Color color, float wave_amplitude, float wave_frequency, float time) {
    if (!text || font.atlas_texture.id == 0) return;
    
    vec2 cursor = {position[0], position[1]};
    int char_index = 0;
    
    for (const char* c = text; *c != '\0'; c++, char_index++) {
        if (*c == '\n') {
            cursor[0] = position[0];
            cursor[1] += font.char_height + font.line_spacing;
            char_index = 0;
            continue;
        }
        
        if (*c == '\r') continue;
        
        // Calculate wave offset
        float wave_offset = sinf(time + char_index * wave_frequency) * wave_amplitude;
        vec2 wave_pos = {cursor[0], cursor[1] + wave_offset};
        
        // Draw character
        int atlas_char_index = *c - font.first_char;
        if (atlas_char_index < 0 || atlas_char_index >= font.char_count) {
            atlas_char_index = '?' - font.first_char;
        }
        
        int atlas_x = (atlas_char_index % font.chars_per_row) * font.char_width;
        int atlas_y = (atlas_char_index / font.chars_per_row) * font.char_height;
        
        Rectangle source_rect = {
            (float)atlas_x, (float)atlas_y,
            (float)font.char_width, (float)font.char_height
        };
        
        RGLSprite char_sprite = {font.atlas_texture, source_rect};
        RGL_DrawSprite(char_sprite, wave_pos, 0.0f, 1.0f, color);
        
        cursor[0] += font.char_width + font.char_spacing;
    }
}

/**
 * @brief Measures text dimensions
 */
SITAPI vec2 RGL_MeasureText(const char* text, RGLBitmapFont font) {
    if (!text) return (vec2){0, 0};
    
    float max_width = 0;
    float current_width = 0;
    int line_count = 1;
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            max_width = fmaxf(max_width, current_width);
            current_width = 0;
            line_count++;
        } else if (*c != '\r') {
            current_width += font.char_width + font.char_spacing;
        }
    }
    
    max_width = fmaxf(max_width, current_width);
    
    return (vec2){
        max_width - font.char_spacing, // Remove trailing spacing
        line_count * font.char_height + (line_count - 1) * font.line_spacing
    };
}

/**
 * @brief Measures TrueType font text
 */
SITAPI vec2 RGL_MeasureTextTTF(const char* text, RGLTrueTypeFont font) {
    if (!text) return (vec2){0, 0};
    
    float max_width = 0;
    float current_width = 0;
    int line_count = 1;
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            max_width = fmaxf(max_width, current_width);
            current_width = 0;
            line_count++;
        } else if (*c != '\r') {
            unsigned char char_code = (unsigned char)*c;
            if (char_code >= 32 && char_code <= 126) {
                current_width += font.char_advances[char_code];
            }
        }
    }
    
    max_width = fmaxf(max_width, current_width);
    
    return (vec2){max_width, line_count * font.line_height};
}

/**
 * @brief Counts lines needed for text with given width constraint
 */
SITAPI int RGL_GetTextLineCount(const char* text, RGLBitmapFont font, float max_width) {
    if (!text || max_width <= 0) return 1;
    
    int line_count = 1;
    float current_width = 0;
    float char_width = font.char_width + font.char_spacing;
    
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            line_count++;
            current_width = 0;
        } else if (*c != '\r') {
            current_width += char_width;
            if (current_width > max_width) {
                line_count++;
                current_width = char_width;
            }
        }
    }
    
    return line_count;
}

/**
 * @brief Cleanup functions
 */
SITAPI void RGL_UnloadBitmapFont(RGLBitmapFont font) {
    if (font.atlas_texture.id > 0) {
        RGL_UnloadTexture(font.atlas_texture);
    }
}

SITAPI void RGL_UnloadTrueTypeFont(RGLTrueTypeFont font) {
    if (font.atlas_texture.id > 0) {
        RGL_UnloadTexture(font.atlas_texture);
    }
}

// Define the default color palette once, statically.
static const RGLTestPatternColors RGL_DEFAULT_TEST_COLORS = {
    .bg_dark_gray      = { 45,  45,  45, 255 },
    .grid_white        = { 255, 255, 255, 100 },
    .bar_light_gray    = { 192, 192, 192, 255 },
    .bar_yellow        = { 192, 192,   0, 255 },
    .bar_cyan          = {   0, 192, 192, 255 },
    .bar_green         = {   0, 192,   0, 255 },
    .bar_magenta       = { 192,   0, 192, 255 },
    .bar_red           = { 192,   0,   0, 255 },
    .bar_blue          = {   0,   0, 192, 255 },
    .bar_black         = {   0,   0,   0, 255 },
    .bar_white         = { 255, 255, 255, 255 },
    .bar_mid_gray      = { 128, 128, 128, 255 },
    .bar_dark_gray     = { 64,  64,  64, 255 },
    .bar_orange        = { 208, 132,  45, 255 }
};

SITAPI RGLTestPatternConfig RGL_GetDefaultTestPatternConfig(RGLTestPatternType type) {
    static const float default_frequencies[] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    RGLTestPatternConfig config = {0};
    config.type = type;
    config.width = 640;
    config.height = 480;
    config.show_overlay_circle = (type == RGL_TESTPATTERN_SMPTE_BARS);
    config.checker_size[0] = 32.0f;
    config.checker_size[1] = 32.0f;
    config.stripe_width = 16.0f;
    config.frequencies = (type == RGL_TESTPATTERN_MULTIBURST) ? default_frequencies : NULL;
    config.num_frequencies = (type == RGL_TESTPATTERN_MULTIBURST) ? 6 : 0;
    config.grid_size = (type == RGL_TESTPATTERN_3D_GRID) ? 5 : 0;
    config.colors = &RGL_DEFAULT_TEST_COLORS;
    return config;
}

// --- Internal Helper for 3D Line Drawing ---
static void _RGL_DrawLineQuad(vec3 start, vec3 end, float thickness, Color color) {
    if (!_RGL_EnsureCommandCapacity(1)) return;

    // Calculate line direction and perpendicular vectors
    vec3 dir;
    glm_vec3_sub(end, start, dir);
    float length = glm_vec3_norm(dir);
    if (length < 0.0001f) return; // Avoid division by zero
    glm_vec3_normalize(dir);

    // Find a perpendicular vector (use cross product with an arbitrary vector)
    vec3 up = { 0, 1, 0 };
    if (fabsf(glm_vec3_dot(dir, up)) > 0.99f) { // If dir is nearly parallel to up
        up[0] = 1; up[1] = 0; up[2] = 0; // Use X axis instead
    }
    vec3 right;
    glm_vec3_cross(dir, up, right);
    glm_vec3_normalize(right);
    vec3 perp;
    glm_vec3_cross(dir, right, perp);
    glm_vec3_normalize(perp);

    // Scale perpendicular vectors by thickness
    glm_vec3_scale(right, thickness / 2.0f, right);
    glm_vec3_scale(perp, thickness / 2.0f, perp);

    // Define quad vertices
    vec3 vertices[4];
    vec3 temp;
    glm_vec3_add(start, right, temp);
    glm_vec3_add(temp, perp, vertices[0]); // Top-right-start
    glm_vec3_sub(temp, perp, vertices[1]); // Bottom-right-start
    glm_vec3_add(end, right, temp);
    glm_vec3_add(temp, perp, vertices[2]); // Top-right-end
    glm_vec3_sub(temp, perp, vertices[3]); // Bottom-right-end

    // Apply transform if set
    if (RGL.use_transform) {
        for (int i = 0; i < 4; i++) {
            vec4 temp = { vertices[i][0], vertices[i][1], vertices[i][2], 1.0f };
            glm_mat4_mulv(RGL.transform, temp, temp);
            vertices[i][0] = temp[0];
            vertices[i][1] = temp[1];
            vertices[i][2] = temp[2];
        }
    }

    // Create draw command
    size_t cmd_idx = RGL.command_count++;
    RGLInternalDraw* cmd = &RGL.commands[cmd_idx];
    cmd->texture.id = 0; // No texture
    vec2 tex_coords[4] = { {0,0}, {1,0}, {1,1}, {0,1} }; // Dummy UVs
    Color colors[4] = { color, color, color, color };
    float light_levels[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // No lighting for lines
    for (int i = 0; i < 4; i++) {
        glm_vec3_copy(vertices[i], cmd->world_positions[i]);
        glm_vec2_copy(tex_coords[i], cmd->tex_coords[i]);
        glm_vec4_copy((vec4){colors[i].r/255.0f, colors[i].g/255.0f, colors[i].b/255.0f, colors[i].a/255.0f}, cmd->colors[i]);
        cmd->light_levels[i] = light_levels[i];
    }
    cmd->z_depth = (vertices[0][2] + vertices[1][2] + vertices[2][2] + vertices[3][2]) / 4.0f;
    cmd->is_triangle = false;
}

/**
 * @section test_patterns Test Pattern Implementation
 * Complete implementation for RGL_DrawTestPattern and _RGL_DrawSmpteBars, supporting
 * SMPTE color bars, checkerboard, convergence, gradients, and grid-only patterns.
 * Uses the 2D drawing API for rendering and integrates with RGL's batching system.
 */
// Internal helper for SMPTE bars pattern
static void _RGL_DrawSmpteBars(const RGLTestPatternConfig* config) {
    if (!config || !config->colors) return;

    const RGLTestPatternColors* C = config->colors;
    int width = config->width;
    int height = config->height;
    
    // --- Layout Calculations ---
    Rectangle content_area = { width * 0.125f, height * 0.2f, width * 0.75f, height * 0.6f };
    float bar_width = content_area.width / 7.0f;

    // --- Top SMPTE Color Bars (45% of content height) ---
    float top_bars_y = content_area.y;
    float top_bars_height = content_area.height * 0.45f;
    Color top_colors[] = { 
        C->bar_light_gray, C->bar_yellow, C->bar_cyan, C->bar_green, 
        C->bar_magenta, C->bar_red, C->bar_blue 
    };
    for (int i = 0; i < 7; i++) {
        RGL_DrawRectangle((Rectangle){ content_area.x + i * bar_width, top_bars_y, bar_width, top_bars_height }, 
                         0.0f, top_colors[i]);
    }
    
    // --- Middle Gray/Black Bars (15% of content height) ---
    float middle_bars_y = top_bars_y + top_bars_height;
    float middle_bars_height = content_area.height * 0.15f;
    Color middle_colors[] = { 
        C->bar_mid_gray, C->bar_black, C->bar_black, C->bar_black, 
        C->bar_black, C->bar_black, C->bar_mid_gray 
    };
    for (int i = 0; i < 7; i++) {
        RGL_DrawRectangle((Rectangle){ content_area.x + i * bar_width, middle_bars_y, bar_width, middle_bars_height }, 
                         0.0f, middle_colors[i]);
    }

    // --- Frequency and PLUGE Bars (20% of content height) ---
    float freq_bar_y = middle_bars_y + middle_bars_height;
    float freq_bar_height = content_area.height * 0.20f;
    // Background: White
    RGL_DrawRectangle((Rectangle){ content_area.x, freq_bar_y, content_area.width, freq_bar_height }, 
                     0.0f, C->bar_white);
    
    // PLUGE bars (-4 IRE, 0 IRE, +4 IRE) on left and right
    float pluge_width = bar_width / 3.0f;
    float start_x_left = content_area.x + bar_width * 0.3f;
    float start_x_right = content_area.x + bar_width * 5.7f;
    Color pluge_colors[] = { 
        (Color){10, 10, 10, 255},  // -4 IRE
        C->bar_black,              // 0 IRE
        (Color){20, 20, 20, 255}   // +4 IRE
    };
    for (int i = 0; i < 3; i++) {
        // Left PLUGE
        RGL_DrawRectangle((Rectangle){ start_x_left + i * pluge_width, freq_bar_y, pluge_width, freq_bar_height }, 
                         0.0f, pluge_colors[i]);
        // Right PLUGE
        RGL_DrawRectangle((Rectangle){ start_x_right + i * pluge_width, freq_bar_y, pluge_width, freq_bar_height }, 
                         0.0f, pluge_colors[i]);
    }
    
    // Frequency burst patterns
    float burst_area_width = bar_width * 1.2f;
    float burst_start_x_left = content_area.x + bar_width * 1.8f;
    float burst_start_x_right = content_area.x + bar_width * 4.5f;
    int num_stripes = 12;
    float stripe_spacing = burst_area_width / num_stripes;
    for (int i = 0; i < num_stripes; i++) {
        float x_offset = i * stripe_spacing;
        Color stripe_color = (i % 2 == 0) ? C->bar_black : C->bar_white;
        // Left burst
        RGL_DrawRectangle((Rectangle){ burst_start_x_left + x_offset, freq_bar_y, stripe_spacing * 0.5f, freq_bar_height }, 
                         0.0f, stripe_color);
        // Right burst
        RGL_DrawRectangle((Rectangle){ burst_start_x_right + x_offset, freq_bar_y, stripe_spacing * 0.5f, freq_bar_height }, 
                         0.0f, stripe_color);
    }
    
    // Gray and orange bars
    RGL_DrawRectangle((Rectangle){ content_area.x + bar_width * 3.0f, freq_bar_y, bar_width * 0.5f, freq_bar_height }, 
                     0.0f, C->bar_dark_gray);
    RGL_DrawRectangle((Rectangle){ content_area.x + bar_width * 6.0f, freq_bar_y + freq_bar_height * 0.1f, bar_width * 0.8f, freq_bar_height * 0.8f }, 
                     0.0f, C->bar_orange);
    RGL_DrawRectangleOutline((Rectangle){ content_area.x + bar_width * 6.0f, freq_bar_y + freq_bar_height * 0.1f, bar_width * 0.8f, freq_bar_height * 0.8f }, 
                            1.0f, C->bar_dark_gray);

    // Downward pointing triangle
    float tri_base_y = freq_bar_y + freq_bar_height;
    vec2 tri_points[] = {
        { content_area.x + content_area.width / 2.0f - 10, tri_base_y - 10 },
        { content_area.x + content_area.width / 2.0f + 10, tri_base_y - 10 },
        { content_area.x + content_area.width / 2.0f, tri_base_y }
    };
    RGL_DrawPolygonScreen(tri_points, 3, C->bar_black);

    // --- Bottom Gradient Bar (20% of content height) ---
    float bottom_bar_y = tri_base_y;
    float bottom_bar_height = content_area.height * 0.20f;
    float bottom_bar_width = content_area.width * 0.715f; // Matches 5 bars
    Rectangle top_grad_rect = { content_area.x, bottom_bar_y, bottom_bar_width, bottom_bar_height / 2.0f };
    Rectangle bot_grad_rect = { content_area.x, bottom_bar_y + bottom_bar_height / 2.0f, bottom_bar_width, bottom_bar_height / 2.0f };
    RGL_DrawRectangleGradient(top_grad_rect, C->bar_magenta, C->bar_black, C->bar_black, C->bar_black);
    RGL_DrawRectangleGradient(bot_grad_rect, C->bar_black, C->bar_black, C->bar_blue, C->bar_black);
    
    // Final gray/black bars
    RGL_DrawRectangle((Rectangle){ content_area.x + bar_width * 5, bottom_bar_y, bar_width, bottom_bar_height }, 
                     0.0f, C->bar_dark_gray);
    RGL_DrawRectangle((Rectangle){ content_area.x + bar_width * 6, bottom_bar_y, bar_width, bottom_bar_height }, 
                     0.0f, C->bar_black);

    // --- Safe Area ---
    RGL_DrawSafeArea((Rectangle){ 0, 0, (float)width, (float)height }, 0.1f, C->bar_white);

    // --- Foreground Circle ---
    if (config->show_overlay_circle) {
        vec2 center = { (float)width / 2.0f, (float)height / 2.0f };
        float radius = content_area.height / 2.0f;
        RGL_DrawCircleOutline(center, radius, 2.0f, C->bar_white);
    }

    // --- Title Label ---
    if (RGL.debug.font.atlas_texture.id > 0) {
        RGL_DrawText("SMPTE Color Bars", (vec2){ content_area.x, content_area.y - 20 }, 
                     RGL.debug.font, C->bar_white);
    }
}

// --- Internal Helper for PLUGE Pattern ---
static void _RGL_DrawPluge(const RGLTestPatternConfig* config) {
    if (!config || !config->colors) return;

    const RGLTestPatternColors* C = config->colors;
    int width = config->width;
    int height = config->height;

    // --- Layout Calculations ---
    Rectangle content_area = { width * 0.1f, height * 0.1f, width * 0.8f, height * 0.8f };
    float bar_width = content_area.width / 10.0f; // 10 bars for flexibility
    float bar_height = content_area.height * 0.6f;
    float bar_y = content_area.y + (content_area.height - bar_height) / 2.0f;

    // --- PLUGE Bars ---
    // IRE levels: -4 (below black), 0 (black), +4 (above black), +7.5 (near black)
    Color pluge_colors[] = {
        (Color){10, 10, 10, 255},   // -4 IRE (below black)
        C->bar_black,               // 0 IRE (black)
        (Color){20, 20, 20, 255},   // +4 IRE (above black)
        (Color){30, 30, 30, 255}    // +7.5 IRE (near black)
    };
    for (int i = 0; i < 4; i++) {
        // Left PLUGE bars
        RGL_DrawRectangle((Rectangle){ content_area.x + i * bar_width, bar_y, bar_width, bar_height }, 
                         0.0f, pluge_colors[i]);
        // Right PLUGE bars (mirrored)
        RGL_DrawRectangle((Rectangle){ content_area.x + (9 - i) * bar_width, bar_y, bar_width, bar_height }, 
                         0.0f, pluge_colors[i]);
    }

    // --- Gray and White Bars (center) ---
    Color center_colors[] = { C->bar_mid_gray, C->bar_white, C->bar_dark_gray };
    for (int i = 0; i < 3; i++) {
        RGL_DrawRectangle((Rectangle){ content_area.x + (4 + i) * bar_width, bar_y, bar_width, bar_height }, 
                         0.0f, center_colors[i]);
    }

    // --- Safe Area ---
    RGL_DrawSafeArea((Rectangle){ 0, 0, (float)width, (float)height }, 0.1f, C->bar_white);

    // --- Grid Overlay ---
    float grid_spacing = (float)width / 32.0f;
    RGL_DrawGrid((vec2){ grid_spacing, grid_spacing }, (vec2){0, 0}, C->grid_white);

    // --- Labels ---
    if (RGL.debug.font.atlas_texture.id > 0) {
        // Title
        RGL_DrawText("PLUGE Pattern", (vec2){ content_area.x, content_area.y - 20 }, 
                     RGL.debug.font, C->bar_white);
        // Bar labels
        const char* pluge_labels[] = { "-4 IRE", "0 IRE", "+4 IRE", "+7.5 IRE" };
        for (int i = 0; i < 4; i++) {
            RGL_DrawText(pluge_labels[i], (vec2){ content_area.x + i * bar_width + 5, bar_y + bar_height + 5 }, 
                         RGL.debug.font, C->bar_white);
        }
        const char* center_labels[] = { "Mid Gray", "White", "Dark Gray" };
        for (int i = 0; i < 3; i++) {
            RGL_DrawText(center_labels[i], (vec2){ content_area.x + (4 + i) * bar_width + 5, bar_y + bar_height + 5 }, 
                         RGL.debug.font, C->bar_white);
        }
    }
}

static void _RGL_DrawCrosshatch(const RGLTestPatternConfig* config) {
    if (!config || !config->colors) return;
    const RGLTestPatternColors* C = config->colors;
    int width = config->width, height = config->height;
    Rectangle screen_rect = { 0, 0, (float)width, (float)height };

    // Background
    RGL_DrawRectangle(screen_rect, 0.0f, C->bg_dark_gray);

    // Crosshatch grid (e.g., 16x12 lines)
    int nx = 16, ny = 12;
    float dx = width / (float)nx, dy = height / (float)ny;
    for (int i = 0; i <= nx; i++) {
        RGL_DrawLineEx((vec2){i * dx, 0}, (vec2){i * dx, height}, 1.0f, C->grid_white);
    }
    for (int j = 0; j <= ny; j++) {
        RGL_DrawLineEx((vec2){0, j * dy}, (vec2){width, j * dy}, 1.0f, C->grid_white);
    }

    // Center crosshair
    RGL_DrawCrosshair((vec2){width / 2.0f, height / 2.0f}, 20.0f, 2.0f, C->bar_white);

    // Safe area
    RGL_DrawSafeArea(screen_rect, 0.1f, C->bar_white);

    // Label
    if (RGL.debug.font.atlas_texture.id > 0) {
        RGL_DrawText("Crosshatch Pattern", (vec2){10, 10}, RGL.debug.font, C->bar_white);
    }
}

static void _RGL_DrawMultiburst(const RGLTestPatternConfig* config) {
    if (!config || !config->colors) return;

    const RGLTestPatternColors* C = config->colors;
    int width = config->width;
    int height = config->height;

    // --- Layout Calculations ---
    Rectangle content_area = { width * 0.1f, height * 0.1f, width * 0.8f, height * 0.8f };
    float bar_height = content_area.height * 0.7f;
    float bar_y = content_area.y + (content_area.height - bar_height) / 2.0f;

    // Define frequency bands (in MHz, typical for NTSC/PAL)
    float frequencies[] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    int num_bands = 6;
    float band_width = content_area.width / num_bands;

    // --- Draw Frequency Bands ---
    for (int i = 0; i < num_bands; i++) {
        float band_x = content_area.x + i * band_width;
        // Calculate stripe width based on frequency (higher freq = narrower stripes)
        float stripe_width = 10.0f / (frequencies[i] + 0.5f); // Non-linear scaling
        int num_stripes = (int)(band_width / (stripe_width * 2.0f)); // Black + white pair
        for (int j = 0; j < num_stripes; j++) {
            float stripe_x = band_x + j * stripe_width * 2.0f;
            Color stripe_color = (j % 2 == 0) ? C->bar_white : C->bar_black;
            RGL_DrawRectangle((Rectangle){ stripe_x, bar_y, stripe_width, bar_height }, 
                             0.0f, stripe_color);
        }
        // Fill remaining band with black if needed
        if (num_stripes * stripe_width * 2.0f < band_width) {
            RGL_DrawRectangle((Rectangle){ band_x + num_stripes * stripe_width * 2.0f, bar_y, 
                                          band_width - num_stripes * stripe_width * 2.0f, bar_height }, 
                             0.0f, C->bar_black);
        }
        // Label frequency
        if (RGL.debug.font.atlas_texture.id > 0) {
            char label[16];
            snprintf(label, sizeof(label), "%.1f MHz", frequencies[i]);
            RGL_DrawText(label, (vec2){ band_x + band_width / 2.0f - 20, bar_y + bar_height + 5 }, 
                         RGL.debug.font, C->bar_white);
        }
    }

    // --- Safe Area ---
    RGL_DrawSafeArea((Rectangle){ 0, 0, (float)width, (float)height }, 0.1f, C->bar_white);

    // --- Grid Overlay ---
    float grid_spacing = (float)width / 32.0f;
    RGL_DrawGrid((vec2){ grid_spacing, grid_spacing }, (vec2){0, 0}, C->grid_white);

    // --- Title Label ---
    if (RGL.debug.font.atlas_texture.id > 0) {
        RGL_DrawText("Multiburst Pattern", (vec2){ content_area.x, content_area.y - 20 }, 
                     RGL.debug.font, C->bar_white);
    }
}

/**
 * @brief (INTERNAL) Queues 6 lit quads to form a cube by calling the public RGL_DrawQuad3D.
 *
 * This is the definitive, refactored implementation. It no longer manually assembles
 * internal draw commands. Instead, it acts as a proper helper function, calculating
 * the vertices and per-face normals for a cube and then calling the standard,
 * lighting-aware RGL_DrawQuad3D for each face. This ensures it is always compatible
 * with the main rendering pipeline.
 *
 * @param position The center of the cube in world space.
 * @param size The total side length of the cube.
 * @param material The material properties, used for diffuse color and base ambient light level.
 */
static void _RGL_DrawCubeFaces(vec3 position, float size, RGLMaterial material) {
    // 1. --- Pre-flight Check ---
    // The capacity check is now handled by RGL_DrawQuad3D, so we don't need it here.

    // 2. --- Define Base Geometry and Normals ---
    float half_size = size / 2.0f;
    vec3 local_vertices[8] = {
        {-half_size, -half_size, -half_size}, // 0: BLF
        { half_size, -half_size, -half_size}, // 1: BRF
        { half_size,  half_size, -half_size}, // 2: TRF
        {-half_size,  half_size, -half_size}, // 3: TLF
        {-half_size, -half_size,  half_size}, // 4: BLB
        { half_size, -half_size,  half_size}, // 5: BRB
        { half_size,  half_size,  half_size}, // 6: TRB
        {-half_size,  half_size,  half_size}  // 7: TLB
    };

    // Apply translation to all local vertices to get preliminary world positions
    for (int i = 0; i < 8; i++) {
        glm_vec3_add(local_vertices[i], position, local_vertices[i]);
    }

    // Define the 6 unique normal vectors, one for each face.
    const vec3 normals[6] = {
        { 0,  0, -1}, // Front
        { 0,  0,  1}, // Back
        {-1,  0,  0}, // Left
        { 1,  0,  0}, // Right
        { 0,  1,  0}, // Top
        { 0, -1,  0}  // Bottom
    };
    
    // Define the vertex indices for each face.
    // The order must match the winding expected by RGL_DrawQuad3D (e.g., BL, BR, TR, TL)
    const int faces[6][4] = {
        {0, 1, 2, 3}, // Front
        {5, 4, 7, 6}, // Back
        {4, 0, 3, 7}, // Left
        {1, 5, 6, 2}, // Right
        {3, 2, 6, 7}, // Top
        {0, 4, 5, 1}  // Bottom
    };

    // Create a dummy sprite for untextured drawing.
    RGLSprite dummy_sprite = {0};

    // 3. --- Loop and Call RGL_DrawQuad3D for Each Face ---
    for (int i = 0; i < 6; i++) {
        // Get the four vertices for the current face
        vec3 p1 = {local_vertices[faces[i][0]][0], local_vertices[faces[i][0]][1], local_vertices[faces[i][0]][2]};
        vec3 p2 = {local_vertices[faces[i][1]][0], local_vertices[faces[i][1]][1], local_vertices[faces[i][1]][2]};
        vec3 p3 = {local_vertices[faces[i][2]][0], local_vertices[faces[i][2]][1], local_vertices[faces[i][2]][2]};
        vec3 p4 = {local_vertices[faces[i][3]][0], local_vertices[faces[i][3]][1], local_vertices[faces[i][3]][2]};
        
        // Get the normal for the current face
        vec3 normal = {normals[i][0], normals[i][1], normals[i][2]};
        
        // Call the public, lighting-aware drawing function.
        // It will handle batching, transformations, and everything else.
        RGL_DrawQuad3D(
            p1, p2, p3, p4,
            normal,
            dummy_sprite,
            material.diffuse,
            material.ambient // Use the material's ambient value as the base light level
        );
    }
}

// --- Update 3D Grid Test Pattern to Use New Primitives ---
static void _RGL_Draw3DGrid(const RGLTestPatternConfig* config) {
    if (!config || !config->colors) return;
    const RGLTestPatternColors* C = config->colors;
    int width = config->width, height = config->height;

    // Set up a 3D camera
    vec3 camera_pos = { 0, 5, -10 };
    vec3 camera_target = { 0, 0, 0 };
    vec3 camera_up = { 0, 1, 0 };
    RGL_SetCamera3D(camera_pos, camera_target, camera_up, RGL_DEFAULT_FOV_DEGREES);

    // Draw a grid of cubes
    int grid_size = config->grid_size > 0 ? config->grid_size : 5;
    float spacing = 2.0f;
    RGLMaterial material = { .diffuse = C->bar_white, .ambient = 0.5f };
    for (int x = -grid_size / 2; x <= grid_size / 2; x++) {
        for (int z = -grid_size / 2; z <= grid_size / 2; z++) {
            vec3 pos = { x * spacing, 0, z * spacing };
            material.diffuse = ((x + z) % 2 == 0) ? C->bar_white : C->bar_mid_gray;
            RGL_DrawCube(pos, 1.0f, material);
        }
    }

    // Draw 3D grid lines
    for (int x = -grid_size / 2; x <= grid_size / 2; x++) {
        RGL_DrawLine3D((vec3){x * spacing, 0, -grid_size / 2 * spacing}, 
                       (vec3){x * spacing, 0, grid_size / 2 * spacing}, 
                       0.1f, C->bar_black);
    }
    for (int z = -grid_size / 2; z <= grid_size / 2; z++) {
        RGL_DrawLine3D((vec3){-grid_size / 2 * spacing, 0, z * spacing}, 
                       (vec3){grid_size / 2 * spacing, 0, z * spacing}, 
                       0.1f, C->bar_black);
    }

    // Draw 3D axes
    RGL_DrawLine3D((vec3){-10,0,0}, (vec3){10,0,0}, 0.2f, C->bar_red);   // X (red)
    RGL_DrawLine3D((vec3){0,-10,0}, (vec3){0,10,0}, 0.2f, C->bar_green); // Y (green)
    RGL_DrawLine3D((vec3){0,0,-10}, (vec3){0,0,10}, 0.2f, C->bar_blue);  // Z (blue)

    // 2D overlay for label
    RGL_SetCamera2D((vec2){width/2.0f, height/2.0f}, 0.0f, 1.0f);
    if (RGL.debug.font.atlas_texture.id > 0) {
        RGL_DrawText("3D Grid Pattern", (vec2){10, 10}, RGL.debug.font, C->bar_white);
    }
}

SITAPI void RGL_DrawTestPattern(const RGLTestPatternConfig* config) {
    if (!RGL.is_initialized || !RGL.is_batching || !config) return;

    // --- Save the current camera state ---
    RGL_PushMatrix();
    
    const RGLTestPatternColors* C = (config->colors) ? config->colors : &RGL_DEFAULT_TEST_COLORS;
    Rectangle screen_rect = { 0, 0, (float)config->width, (float)config->height };

    switch (config->type) {
        case RGL_TESTPATTERN_SMPTE_BARS: {
            RGL_DrawRectangle(screen_rect, 0.0f, C->bg_dark_gray);
            float grid_spacing = (float)config->width / 32.0f;
            RGL_DrawGrid((vec2){ grid_spacing, grid_spacing }, (vec2){0, 0}, C->grid_white);
            _RGL_DrawSmpteBars(config);
        } break;
        
        case RGL_TESTPATTERN_CHECKERBOARD: {
            int nx = (int)(screen_rect.width / config->checker_size[0]);
            int ny = (int)(screen_rect.height / config->checker_size[1]);
            for (int y = 0; y < ny; y++) {
                for (int x = 0; x < nx; x++) {
                    Color c = ((x + y) % 2 == 0) ? C->bar_white : C->bar_black;
                    RGL_DrawRectangle((Rectangle){ 
                        x * config->checker_size[0], y * config->checker_size[1], 
                        config->checker_size[0], config->checker_size[1]
                    }, 0.0f, c);
                }
            }
            if (RGL.debug.font.atlas_texture.id > 0) {
                RGL_DrawText("Checkerboard", (vec2){ 10, 10 }, RGL.debug.font, C->bar_white);
            }
        } break;
        
        case RGL_TESTPATTERN_CONVERGENCE: {
            RGL_DrawStripes(screen_rect, config->stripe_width, true, C->bar_white, C->bar_black);
            Rectangle central_rect = { 
                screen_rect.width * 0.25f, screen_rect.height * 0.25f, 
                screen_rect.width * 0.5f, screen_rect.height * 0.5f 
            };
            RGL_DrawStripes(central_rect, config->stripe_width, false, C->bar_white, C->bar_black);
            if (RGL.debug.font.atlas_texture.id > 0) {
                RGL_DrawText("Convergence Test", (vec2){ 10, 10 }, RGL.debug.font, C->bar_white);
            }
        } break;
        
        case RGL_TESTPATTERN_GRADIENTS: {
            Rectangle r1 = { 0, 0, config->width / 2.0f, config->height / 2.0f };
            Rectangle r2 = { config->width / 2.0f, 0, config->width / 2.0f, config->height / 2.0f };
            Rectangle r3 = { 0, config->height / 2.0f, config->width / 2.0f, config->height / 2.0f };
            Rectangle r4 = { config->width / 2.0f, config->height / 2.0f, config->width / 2.0f, config->height / 2.0f };
            RGL_DrawRectangleGradient(r1, C->bar_red, C->bar_green, C->bar_black, C->bar_black);
            RGL_DrawRectangleGradient(r2, C->bar_cyan, C->bar_magenta, C->bar_black, C->bar_black);
            RGL_DrawRectangleGradient(r3, C->bar_yellow, C->bar_blue, C->bar_black, C->bar_black);
            RGL_DrawRectangleGradient(r4, C->bar_white, C->bar_mid_gray, C->bar_black, C->bar_black);
            if (RGL.debug.font.atlas_texture.id > 0) {
                RGL_DrawText("Gradient Test", (vec2){ 10, 10 }, RGL.debug.font, C->bar_white);
            }
        } break;
        
        case RGL_TESTPATTERN_GRID_ONLY: {
            RGL_DrawRectangle(screen_rect, 0.0f, C->bg_dark_gray);
            float grid_spacing = (float)config->width / 32.0f;
            RGL_DrawGrid((vec2){ grid_spacing, grid_spacing }, (vec2){0, 0}, C->grid_white);
            if (RGL.debug.font.atlas_texture.id > 0) {
                RGL_DrawText("Grid Overlay", (vec2){ 10, 10 }, RGL.debug.font, C->bar_white);
            }
        } break;
        
        case RGL_TESTPATTERN_PLUGE: {
            RGL_DrawRectangle(screen_rect, 0.0f, C->bg_dark_gray);
            _RGL_DrawPluge(config);
        } break;
        case RGL_TESTPATTERN_MULTIBURST: {
            RGL_DrawRectangle(screen_rect, 0.0f, C->bg_dark_gray);
            _RGL_DrawMultiburst(config);
        } break;
        case RGL_TESTPATTERN_CROSSHATCH: {
            _RGL_DrawCrosshatch(config);
        } break;
        case RGL_TESTPATTERN_3D_GRID: {
            _RGL_Draw3DGrid(config);
        } break;
    }
    // --- Restore the previous camera state ---
    RGL_PopMatrix();
}

#endif // RGL_IMPLEMENTATION

/*
 * =====================================================================================
 *  EXAMPLE USAGE (Modern 3D Pipeline)
 * =====================================================================================
 *
 * This example demonstrates how to use the modern, true 3D rendering pipeline
 * to create a classic arcade racing game experience. It replaces the old pseudo-3D
 * "scaler" system.
 */

// --- Game-specific data (placeholders for your game's state) ---
/*typedef struct {
    RGLSprite sprite;
    vec3 world_pos;             // True 3D position
    vec2 size_in_world_units;   // Size in the 3D world
} GameObject;

// Assume these are defined and loaded by your game's asset manager
extern RGLSprite g_opponent_car_sprite;
extern RGLSprite g_player_car_sprite; // For a HUD representation
extern RGLSprite g_tree_sprite;
extern RGLTexture g_sky_panorama;
extern RGLTexture g_grass_texture;
extern GameObject g_game_objects[];
extern int g_num_game_objects;

// Player's state
static vec3 g_player_pos = {0.0f, 0.5f, 100.0f}; // X, Y, Z position in world space
static float g_player_roll_degrees = 0.0f; // Represents steering input, for camera banking
static float g_camera_pitch_offset = 2.0f; // How high the camera looks above the car
*/
/**
 * @brief Helper function to calculate a rotated 'up' vector for camera banking.
 * In a true 3D system, screen roll is achieved by rotating the camera's up vector
 * around its forward-facing axis.
 *
 * @param forward_vec The camera's forward direction vector (target - position).
 * @param roll_degrees The amount of roll to apply, in degrees.
 * @param out_up The resulting 'up' vector.
 */
/*void CalculateBankedUpVector(vec3 forward_vec, float roll_degrees, vec3 out_up) {
    // Start with the default world 'up' vector
    vec3 default_up = {0.0f, 1.0f, 0.0f};

    // Create a rotation matrix that rotates around the 'forward_vec' axis
    mat4 roll_matrix;
    glm_rotate_make(roll_matrix, glm_rad(roll_degrees), forward_vec);
    
    // Apply the rotation to the default 'up' vector
    glm_mat4_mulv3(roll_matrix, default_up, 1.0f, out_up);
}*/

/**
 * @brief Main drawing function for the game scene.
 * This replaces the old manual Path drawing loops.
 */
/*void DrawGameScene(void) {
    // --- 1. Get Interpolated Path Data for Player Position ---
    // This is used to position the camera correctly relative to the path.
    RGLPathPoint player_Path_props;
    if (!RGL_GetPathPropertiesAt(g_player_pos[2], &player_Path_props)) {
        // Handle case where player is off the path if necessary
        return;
    }

    // --- 2. Setup the 3D Camera ---
    // The camera follows the player from behind and slightly above.
    // Its position is based on the smoothly curved path's center.
    vec3 camera_pos;
    camera_pos[0] = player_Path_props.world_x_offset; // Follow path's horizontal curve
    camera_pos[1] = player_Path_props.world_y_offset + 5.0f; // Follow path's hills + height above Path
    camera_pos[2] = g_player_pos[2] - 15.0f; // Position camera behind the player

    // The camera looks forward from the player's position.
    vec3 camera_target;
    camera_target[0] = player_Path_props.world_x_offset;
    camera_target[1] = player_Path_props.world_y_offset + g_camera_pitch_offset; // Look slightly up
    camera_target[2] = g_player_pos[2] + 50.0f; // Look far ahead down the path

    // Calculate the 'up' vector, applying roll for the banking effect.
    vec3 camera_up;
    vec3 cam_forward;
    glm_vec3_sub(camera_target, camera_pos, cam_forward);
    glm_vec3_normalize(cam_forward);
    CalculateBankedUpVector(cam_forward, g_player_roll_degrees, camera_up);

    // --- 3. Begin Rendering Frame ---
    RGL_Begin(-1); // Begin rendering to the main screen

        // Set the full 3D perspective camera. This defines our viewpoint for the 3D world.
        RGL_SetCamera3D(camera_pos, camera_target, camera_up, RGL_DEFAULT_FOV_DEGREES);

        // Draw a panoramic sky. This is drawn first with depth testing off.
        float sky_scroll = -camera_pos[0] * 0.001f;
        RGL_DrawPanoramaBackground(g_sky_panorama, sky_scroll, 0.0f, 1.0f, WHITE);

        // Draw a large ground plane for the "grass". This is a simple quad that will
        // be correctly depth-tested against the path drawn on top of it.
        Color grass_colors[4] = {WHITE, WHITE, WHITE, WHITE};
        RGL_DrawQuadPro(g_grass_texture.id, (Rectangle){0,0,500,500}, 
            (vec3){camera_pos[0], 0, camera_pos[2]}, (vec2){10000, 10000}, (vec2){0.5f, 0.5f},
            (vec3){0,0,0}, (vec2){0,0}, grass_colors, NULL);

        // Draw the entire path, scenery, and tunnels with a single, powerful call.
        // 'camera_pos[2]' is used as a reference point for where to start drawing.
        RGL_DrawPathAsRoad(camera_pos[2], 300); // Draw 300 segments into the distance

        // Draw all other dynamic game objects (e.g., opponent cars, items).
        // `RGL_DrawBillboardCylindricalY` is perfect for cars that should remain upright.
        for (int i = 0; i < g_num_game_objects; i++) {
            GameObject* obj = &g_game_objects[i];
            RGL_DrawBillboardCylindricalY(
                obj->sprite,
                obj->world_pos,
                obj->size_in_world_units,
                WHITE
            );
        }

        // --- 4. Draw HUD ---
        // To draw the 2D HUD, switch to a simple orthographic camera.
        int screen_w, screen_h;
        SituationGetVirtualDisplaySize(-1, &screen_w, &screen_h);
        RGL_SetCamera2D((vec2){screen_w / 2.0f, screen_h / 2.0f}, 0.0f, 1.0f);

        // Draw the player's car sprite on the HUD (as if it's the front of the car).
        RGL_DrawSprite(g_player_car_sprite, (vec2){screen_w/2.0f - 128, (float)screen_h - 256}, 0.0f, 1.0f, WHITE);
        
        // Draw a simple speedometer bar.
        RGL_DrawRectangle((Rectangle){20, (float)screen_h - 40, 200, 20}, 0.0f, BLUE);

    RGL_End();
}*/

/**
 * @brief Initialization logic for the game.
 */
/*void GameInit(void) {
    // --- Define and Load a Path ---
    RGL_CreatePath("Monaco");

    for (int i = 0; i < 500; i++) {
        RGLPathPoint p = {0};
        p.world_z = i * 10.0f; // Points are 10 units apart
        
        // Create a gentle S-curve and a hill
        p.world_x_offset = sinf(i / 50.0f) * 300.0f;
        p.world_y_offset = sinf(i / 20.0f) * 50.0f;
        
        p.primary_ribbon_width = 20.0f;
        p.primary_lanes = 2;
        p.rumble_width = 2.0f;
        
        p.color_surface = (Color){80, 80, 80, 255};
        p.color_rumble = (Color){200, 0, 0, 255};
        p.color_lines = WHITE;
        
        // Add a tree every 5 points on the left
        if (i > 5 && i % 5 == 0) {
            p.scenery_left.type = RGL_SCENERY_SPRITE;
            p.scenery_left.x_offset = -1.5f; // Place it 1.5x Path half-width from center
            p.scenery_left.y_offset = 0.0f;
            p.scenery_left.data.visual.sprite = g_tree_sprite;
            p.scenery_left.data.visual.size_in_world_units = (vec2){15, 30};
        }
        
        RGL_AddPathPoint("Monaco", p);
    }

    RGL_SetActivePath("Monaco");
    RGL_SetPathLooping("Monaco", 0.0f); // Make the path loop seamlessly
}*/

/**
 * @example Expanded Lighting Integration
 * This example shows how to set up and render a scene with the new, varied light types.
 */
/*void UpdateAndRenderScene(float player_z, float delta_time) {
    // Create a main directional light for the sun
    static int sun_id = 0;
    if (sun_id == 0) {
        sun_id = RGL_CreateDirectionalLight((vec3){0.5f, -1.0f, -0.2f}, (Color){255, 245, 220, 255}, 0.8f);
    }

    // Set a cool ambient light for nighttime
    RGL_SetAmbientLight((Color){15, 20, 35, 255});
    
    // Animate a flickering torch light attached to the player's car
    static int car_light_id = 0;
    if (car_light_id == 0) {
        car_light_id = RGL_CreatePointLight((vec3){0, 2, player_z}, (Color){255, 180, 90, 255}, 50.f, 1.5f);
    }
    RGL_SetLightPosition(car_light_id, (vec3){0, 2, player_z});
    // Manually animate for more control
    float base_intensity = 1.5f;
    float flicker = base_intensity * (1.0f + 0.2f * sinf(SituationGetTime() * 20.0f));
    RGL_SetLightIntensity(car_light_id, flicker);

    // Create lights from scenery definitions as we pass them
    RGLScenery* scenery[32];
    int count = RGL_FindSceneryInRadius((vec3){0, 1, player_z}, 500.0f, scenery, 32);
    for (int i = 0; i < count; i++) {
        if (scenery[i]->type == RGL_SCENERY_LIGHT_SOURCE) {
            if (scenery[i]->data.light.light_id == 0) {
                vec3 pos; // Calculate scenery world position
                // ...
                int id = RGL_CreatePointLight(pos, scenery[i]->data.light.color,
                                              scenery[i]->data.light.radius,
                                              scenery[i]->data.light.intensity);
                scenery[i]->data.light.light_id = id;
            }
        }
    }

    // Update lights attached to level things
    if (RGL.active_level_index >= 0) {
        RGLLevel* level = &RGL.levels[RGL.active_level_index];
        for (size_t i = 0; i < level->thing_count; i++) {
            RGLThing* thing = &level->things[i];
            if (thing->attached_light_id > 0) {
                RGL_SetLightPosition(thing->attached_light_id, (vec3){thing->x, thing->y, thing->z});
            }
        }
    }
    
    RGL_Begin(-1);
    // ... RGL_SetCamera3D, RGL_DrawPathAsRoad, RGL_DrawLevel ...
    RGL_End();
}*/

#endif // RGL_H