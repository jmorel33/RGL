# RGL API Reference

This document provides a detailed reference for the RGL API, generated from the function declarations in `rgl.h`.

## Callback Function Types

| Signature | Description |
| --- | --- |
| `typedef void (*RGLPathSegmentDrawCallback)(const RGLPathPoint* p_near, const RGLPathPoint* p_far, const vec3* normal, void* user_data);` | Defines a function that can draw a single segment of a path. |
| `typedef void (*RGLSceneryDrawCallback)(const RGLScenery* scenery, const RGLPathPoint* path_point, const vec3* world_pos, void* user_data);` | Defines a function that can draw a single scenery object. |

## Core Module: Lifecycle, State, and Frame Management

| Signature | Description |
| --- | --- |
| `SITAPI bool RGL_Init(void);` | Initializes the renderer; must be called after `SituationInit()`. |
| `SITAPI void RGL_Shutdown(void);` | Shuts down the renderer and frees all associated resources. |
| `SITAPI void RGL_Begin(int virtual_display_id);` | Begins a new render frame, targeting a specific virtual display (-1 for main screen). |
| `SITAPI void RGL_End(void);` | Ends the render frame, flushing all batched commands. |
| `SITAPI void RGL_SetTransform(mat4 transform);` | Sets a global 3D model matrix for subsequent `RGL_Draw...` calls. |
| `SITAPI void RGL_ResetTransform(void);` | Resets the global 3D model matrix to identity. |

## Camera & View Module

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_SetCamera2D(vec2 target, float rotation_degrees, float zoom);` | Configures an orthographic camera for 2D rendering. |
| `SITAPI void RGL_SetCamera3D(vec3 position, vec3 target, vec3 up, float fov_y_degrees);` | Configures a perspective camera for 3D rendering. |
| `SITAPI void RGL_PushMatrix(void);` | Pushes the current camera matrices onto a stack, saving the current view. |
| `SITAPI void RGL_PopMatrix(void);` | Pops the last saved camera matrices from the stack, restoring the view. |
| `SITAPI void RGL_GetViewMatrix(mat4 out_view);` | Gets a copy of the current view matrix. |
| `SITAPI void RGL_GetProjectionMatrix(mat4 out_proj);` | Gets a copy of the current projection matrix. |
| `SITAPI vec2 RGL_WorldToScreen(vec3 world_pos);` | Projects a 3D world-space coordinate to a 2D screen-space coordinate. |
| `SITAPI vec3 RGL_ScreenToWorld(vec2 screen_pos, float z_depth);` | Un-projects a 2D screen-space coordinate back into the 3D world. |
| `SITAPI Rectangle RGL_GetScreenRect(void);` | Returns a rectangle representing the current rendering viewport. |

## World Systems: Path Management

| Signature | Description |
| --- | --- |
| `SITAPI bool RGL_CreatePath(const char* path_name);` | Creates a new, empty, named path for building a world. |
| `SITAPI void RGL_AddPathPoint(const char* path_name, RGLPathPoint point);` | Adds a new control point to the end of a named path. |
| `SITAPI bool RGL_SetPathStyle(const char* path_name, const RGLPathStyle* style);` | Assigns a custom drawing style to a named path. |
| `SITAPI bool RGL_SetActivePath(const char* path_name);` | Sets the currently active path for all drawing and query functions. |
| `SITAPI bool RGL_SetPathLooping(const char* path_name, float z_pos);` | Configures a path to loop back to a specific Z-position. |
| `SITAPI bool RGL_DestroyPathByName(const char* path_name);` | Destroys a named path and frees its memory. |
| `SITAPI void RGL_DrawPath(float player_z, int draw_distance);` | Draws the active path using its currently assigned custom style. |
| `SITAPI void RGL_DrawPathAsRoad(float player_z, int draw_distance);` | Convenience wrapper to draw the active path as a classic road. |
| `SITAPI void RGL_DrawPathAsMap(RGLTexture target, vec2 center_pos_xz, float world_width, Color bg_color);` | Renders a top-down 2D map of the active path to a texture. |
| `SITAPI const RGLPathStyle* RGL_GetDefaultRoadStyle(void);` | Gets a pointer to the built-in road style, for use with `RGL_SetPathStyle`. |

## World Systems: Level Management

| Signature | Description |
| --- | --- |
| `SITAPI bool RGL_CreateLevel(const char* level_name);` | Creates a new, empty, named level for building geometry. |
| `SITAPI int RGL_AddVertex(const char* level_name, RGLVertex3D_pos vertex);` | Adds a 2D vertex (for XZ plane) to a named level. |
| `SITAPI bool RGL_AddWall(const char* level_name, RGLWall wall);` | Adds a vertical wall segment to a named level. |
| `SITAPI bool RGL_AddFlat(const char* level_name, RGLFlat flat);` | Adds a horizontal floor or ceiling polygon to a named level. |
| `SITAPI bool RGL_AddThing(const char* level_name, RGLThing thing);` | Adds a billboard-style object (e.g., enemy, item) to a named level. |
| `SITAPI bool RGL_SetActiveLevel(const char* level_name);` | Sets the currently active level for drawing. |
| `SITAPI bool RGL_DestroyLevelByName(const char* level_name);` | Destroys a named level and frees all its associated geometry data. |
| `SITAPI void RGL_DrawLevel(void);` | Draws the active level, including walls, flats, and things, with dynamic lighting. |
| `SITAPI bool RGL_PlaceLevelOnPath(const char* level_name, const char* path_name, float path_z, vec3 offset, float yaw_offset_degrees);` | Positions a level relative to a point on a path. |
| `SITAPI void RGL_DrawWorld(float camera_z, int Path_draw_distance);` | High-level helper to draw both the active path and all loaded levels. |

## World Systems: Queries & Scenery

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_RegisterSceneryStyle(RGLSceneryType type, const RGLSceneryStyle* style);` | Registers a custom drawing function for a type of scenery. |
| `SITAPI void RGL_UpdatePathScenery(float player_z, float view_distance);` | Updates dynamic scenery state, such as creating lights from scenery definitions. |
| `SITAPI bool RGL_GetPathPropertiesAt(float z_pos, RGLPathPoint* out_point);` | Gets the interpolated geometry and properties of the active path at a Z-position. |
| `SITAPI bool RGL_GetGroundAt(vec2 world_xz, RGLGroundInfo* out_info);` | Finds the ground surface (Y-position and normal) at a world XZ coordinate. |
| `SITAPI bool RGL_QueryJunction(float player_z, float search_radius, RGLJunctionInfo* out_info);` | Queries the active path for a junction trigger and returns its connection info. |
| `SITAPI bool RGL_GetDistanceToMarker(float player_z, const char* marker_name, float* out_distance);` | Finds the distance to the next event marker with a specific name. |
| `SITAPI int RGL_FindMarkersInRange(float start_z, float end_z, RGLMarkerInfo out_markers[], int max_markers);` | Finds all event markers within a Z-range. |
| `SITAPI int RGL_FindSceneryInRange(float start_z, float end_z, RGLScenery* out_scenery[], int max_scenery);` | Finds all scenery objects within a Z-range. |
| `SITAPI int RGL_FindSceneryInRadius(vec3 world_pos, float radius, RGLScenery* out_objects[], int max_objects);` | Finds all scenery objects within a 3D radius. |
| `SITAPI vec3 RGL_LevelToWorld(const char* level_name, vec3 local_pos);` | Converts a 3D coordinate from a level's local space to global world space. |
| `SITAPI vec3 RGL_WorldToLevel(const char* level_name, vec3 world_pos);` | Converts a 3D coordinate from global world space to a level's local space. |

## Mesh & Resource Management

| Signature | Description |
| --- | --- |
| `SITAPI RGLTexture RGL_LoadTexture(const char* filename, GLenum wrap_mode, GLenum filter_mode);` | Loads a texture from a file with basic parameters. |
| `SITAPI RGLTexture RGL_LoadTextureWithParams(const char* filename, const LTTextureParams* params);` | Loads a texture from a file with advanced parameters. |
| `SITAPI RGLTexture RGL_CreateRenderTexture(int width, int height);` | Creates a new texture that can be used as a rendering target. |
| `SITAPI void RGL_SetRenderTarget(RGLTexture texture);` | Sets the current rendering target to a specific texture. |
| `SITAPI void RGL_ResetRenderTarget(void);` | Resets the rendering target back to the main screen or virtual display. |
| `SITAPI void RGL_UnloadTexture(RGLTexture texture);` | Unloads a texture from memory. |
| `SITAPI void RGL_DestroyRenderTexture(RGLTexture texture);` | Destroys a render texture and its associated framebuffer object. |
| `SITAPI Rectangle RGL_GetTextureRect(RGLTexture texture);` | Returns a rectangle representing the full dimensions of a texture. |
| `SITAPI RGLMesh RGL_LoadMeshFromFile(const char* filename);` | Loads a 3D model from a .obj file into a manageable mesh object. |
| `SITAPI RGLMesh RGL_CreateMeshFromLevel(const char* level_name);` | Creates a CPU-only mesh from a level's geometry, for use with stencil shadows. |
| `SITAPI bool RGL_SaveMeshToFile(RGLMesh mesh, const char* filename);` | Saves a mesh's CPU-side geometry to a .obj file. |
| `SITAPI void RGL_DestroyMesh(RGLMesh* mesh);` | Frees all CPU and GPU resources associated with a mesh. |

## Procedural Mesh Generation

| Signature | Description |
| --- | --- |
| `SITAPI RGLMesh RGL_GenMeshPlane(float width, float length, int subdivisions_x, int subdivisions_z);` | Generates a flat plane mesh on the XZ axis. |
| `SITAPI RGLMesh RGL_GenMeshCube(float width, float height, float depth);` | Generates a cube mesh with the specified dimensions. |
| `SITAPI RGLMesh RGL_GenMeshSphere(float radius, int slices, int stacks);` | Generates a sphere mesh. |
| `SITAPI RGLMesh RGL_GenMeshCylinder(float radius, float height, int slices);` | Generates a cylinder mesh oriented along the Y-axis. |
| `SITAPI RGLMesh RGL_GenMeshTorus(float major_radius, float tube_radius, int major_segments, int tube_segments);` | Generates a torus (donut) mesh. |
| `SITAPI RGLMesh RGL_GenMeshCapsule(float radius, float height, int slices, int stacks);` | Generates a capsule mesh (cylinder with hemispherical caps). |
| `SITAPI RGLMesh RGL_GenMeshIcosahedron(float radius);` | Generates a 20-sided icosahedron mesh. |
| `SITAPI RGLMesh RGL_GenMeshDodecahedron(float radius);` | Generates a 12-sided dodecahedron mesh. |
| `SITAPI RGLMesh RGL_GenMeshKnot(float major_radius, float tube_radius, int major_segments, int tube_segments);` | Generates a trefoil knot mesh. |
| `SITAPI RGLMesh RGL_GenMeshRock(float radius, int subdivisions, int seed);` | Generates a randomized, rocky asteroid-like mesh. |

## Dynamic Lighting

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_SetAmbientLight(Color color);` | Sets the global ambient light color for the entire scene. |
| `SITAPI int RGL_CreatePointLight(vec3 position, Color color, float radius, float intensity);` | Creates a new point light that radiates in all directions. |
| `SITAPI int RGL_CreateDirectionalLight(vec3 direction, Color color, float intensity);` | Creates a new directional light (e.g., sun) that affects the whole scene. |
| `SITAPI int RGL_CreateSpotLight(vec3 position, vec3 direction, Color color, float radius, float intensity, float outer_angle_deg, float inner_angle_deg);` | Creates a new cone-shaped spot light. |
| `SITAPI int RGL_CreatePointLightYPQ(vec3 position, ColorYPQA ypq_color, float radius, float intensity);` | Creates a point light using a YPQ color, for retro aesthetics. |
| `SITAPI void RGL_SetLightActive(int light_id, bool active);` | Activates or deactivates a light. |
| `SITAPI void RGL_SetLightPosition(int light_id, vec3 position);` | Updates the world position of a point or spot light. |
| `SITAPI void RGL_SetLightDirection(int light_id, vec3 direction);` | Updates the direction of a directional or spot light. |
| `SITAPI void RGL_SetLightDirectionFromYPR(int light_id, vec3 ypr_degrees);` | Updates a light's direction using Yaw, Pitch, and Roll angles. |
| `SITAPI void RGL_SetLightColor(int light_id, Color color);` | Updates the color of a light. |
| `SITAPI void RGL_SetLightIntensity(int light_id, float intensity);` | Updates the intensity (brightness) of a light. |
| `SITAPI void RGL_AnimateLight(int light_id, float time, float frequency, float amplitude);` | Applies a simple sinusoidal flicker to a light's intensity. |
| `SITAPI void RGL_DestroyLight(int light_id);` | Destroys a light and frees its slot. |

## Shadow Rendering

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_CastStencilShadowFromMesh(RGLMesh mesh, mat4 transform, const RGLShadowConfig* config);` | Casts a high-quality, perspective-correct stencil shadow from a mesh. |
| `SITAPI void RGL_DrawSpriteWithShadow(RGLSprite sprite, vec3 world_pos, vec2 size, const RGLShadowConfig* config);` | Convenience wrapper to cast a stencil shadow from a billboard sprite. |
| `SITAPI void RGL_DrawSpriteWithSimpleShadow(RGLSprite sprite, vec3 world_pos, vec2 size, int light_id);` | Simplified helper to cast a default stencil shadow from a light. |
| `SITAPI void RGL_DrawSpriteDownwardShadow(RGLSprite sprite, vec3 world_pos, vec2 size, Color shadow_tint);` | Draws a fast, simple "blob" shadow projected vertically onto the ground. |

## 2D & 3D Primitive Drawing

### 2D Primitives

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_DrawPixel(vec2 position, Color color);` | Draws a single pixel at a 2D coordinate. |
| `SITAPI void RGL_DrawLine(vec2 start, vec2 end, Color color);` | Draws a 1-pixel-thick line between two points. |
| `SITAPI void RGL_DrawLineEx(vec2 start_pos, vec2 end_pos, float thick, Color color);` | Draws a line with a specified thickness. |
| `SITAPI void RGL_DrawLineBezier(vec2 start, vec2 end, vec2 control1, vec2 control2, float thickness, Color color);` | Draws a smooth, cubic Bezier curve. |
| `SITAPI void RGL_DrawPolyline(vec2* points, int point_count, float thickness, Color color, bool closed);` | Draws a series of connected lines. |
| `SITAPI void RGL_DrawRectangle(Rectangle rect, float roll_degrees, Color color);` | Draws a color-filled rectangle with optional rotation. |
| `SITAPI void RGL_DrawRectangleOutline(Rectangle rect, float thickness, Color color);` | Draws the outline of a rectangle. |
| `SITAPI void RGL_DrawRectangleRounded(Rectangle rect, float roundness, Color color);` | Draws a color-filled rectangle with rounded corners. |
| `SITAPI void RGL_DrawRectangleRoundedOutline(Rectangle rect, float roundness, float thickness, Color color);` | Draws the outline of a rectangle with rounded corners. |
| `SITAPI void RGL_DrawRectangleGradient(Rectangle rect, Color top_left, Color top_right, Color bottom_left, Color bottom_right);` | Draws a rectangle with a smooth, per-vertex color gradient. |
| `SITAPI void RGL_DrawCircle(vec2 center, float radius, Color color);` | Draws a color-filled circle. |
| `SITAPI void RGL_DrawCircleOutline(vec2 center, float radius, float thickness, Color color);` | Draws the outline of a circle. |
| `SITAPI void RGL_DrawEllipse(vec2 center, vec2 radii, Color color);` | Draws a color-filled ellipse. |
| `SITAPI void RGL_DrawRing(vec2 center, float inner_radius, float outer_radius, Color color);` | Draws a color-filled ring (donut shape). |
| `SITAPI void RGL_DrawArc(vec2 center, float radius, float start_angle, float end_angle, Color color);` | Draws a color-filled arc or pie-slice shape. |
| `SITAPI void RGL_DrawPolygon(vec2* points, int point_count, float z_depth, Color color);` | Draws a filled, convex polygon on a specific Z-plane in world space. |
| `SITAPI void RGL_DrawPolygonScreen(vec2* points, int point_count, Color color);` | Draws a filled, convex polygon in screen space for UI. |
| `SITAPI void RGL_DrawCircleYPQ(vec2 center, float radius, ColorYPQA color);` | CRT-specific color |
| `SITAPI void RGL_DrawRectangleYPQ(Rectangle rect, ColorYPQA color);` | CRT-specific color |

### 3D Primitive & Sprite Drawing

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_DrawTriangle3D(vec3 p1, vec3 p2, vec3 p3, vec3 normal, vec2 uv1, vec2 uv2, vec2 uv3, RGLSprite sprite, Color tint, float base_light);` | Draws a single lit, textured 3D triangle with explicit UVs. |
| `SITAPI void RGL_DrawQuad3D(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 normal, RGLSprite sprite, Color tint, float base_light);` | Draws a single lit, textured 3D quad defined by four corner points. |
| `SITAPI void RGL_DrawCube(vec3 position, float size, RGLMaterial material);` | Draws a solid-colored, dynamically lit 3D cube. |
| `SITAPI void RGL_DrawLine3D(vec3 start, vec3 end, float thickness, Color color);` | Draws a 3D line with specified thickness. |
| `SITAPI void RGL_DrawSprite(RGLSprite sprite, vec2 position, float roll_degrees, float scale, Color tint);` | Draws a simple 2D sprite with rotation and scaling. |
| `SITAPI void RGL_DrawTexturePro(RGLSprite sprite, Rectangle dest_rect, vec2 origin, float rotation_degrees, Color tint);` | Draws a textured quad with transformation options. |
| `SITAPI void RGL_DrawSpritePro(RGLSprite sprite, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]);` | The ultimate low-level sprite/quad drawing function with full options. |
| `SITAPI void RGL_DrawBillboard(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint);` | Draws a sprite in 3D that always faces the camera (spherical). |
| `SITAPI void RGL_DrawBillboardCylindricalY(RGLSprite sprite, vec3 world_pos, vec2 size, Color tint);` | Draws a sprite in 3D that only pivots on the Y-axis to face the camera. |
| `SITAPI void RGL_DrawPanoramaBackground(RGLTexture texture, float scroll_offset_x, float y_offset_pct, float height_scale, Color tint);` | Draws a horizontally-scrolling panoramic background. |
| `SITAPI void RGL_DrawQuadPro(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color colors[4], float light_levels[4]);` | DEPRECATED - Use DrawSpritePro. |
| `SITAPI void RGL_DrawQuad(RGLTexture texture, Rectangle source_rect, vec3 position, vec2 size, vec2 origin_pct, vec3 rotation_eul_deg, vec2 skew, Color tint);` | DEPRECATED - Use DrawTexturePro or DrawSpritePro. |

## Particle System

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_InitParticles(size_t max_particles);` | Initializes the particle system with a maximum capacity. |
| `SITAPI void RGL_EmitParticles(RGLParticleEmitter emitter);` | Emits a burst of particles from a defined emitter. |
| `SITAPI void RGL_UpdateParticles(float delta_time);` | Updates the physics and lifetime of all active particles. |
| `SITAPI void RGL_DrawParticles(void);` | Draws all active particles as billboards. |

## Font & Text Module

### Font Creation

| Signature | Description |
| --- | --- |
| `SITAPI RGLBitmapFont RGL_LoadBitmapFont(const char* texture_filepath, int char_width, int char_height, int first_char);` | Loads a font from a pre-made grid texture atlas. |
| `SITAPI RGLTrueTypeFont RGL_LoadTrueTypeFont(const char* font_path, float font_size);` | Loads a .ttf font and bakes it into a texture atlas for high-speed rendering. |
| `SITAPI RGLBitmapFont RGL_CreateCP437Font(const unsigned char* font_data_8x16);` | Creates a classic 8x16 CP437 terminal font from memory. |
| `SITAPI RGLBitmapFont RGL_CreatePackedBitmapFont(const void* packed_data, const RGLPackedFontConfig* config);` | Creates a bitmap font from custom-packed bit data. |
| `SITAPI RGLBitmapFont RGL_CreateVCRFont(const uint16_t* font_data);` | Creates a 12x14 VCR-style font from custom packed data. |
| `SITAPI RGLBitmapFont RGL_CreateVCRFontWithOutline(const uint16_t* data, int outline_thickness);` | Creates a VCR-style font with an outline. |
| `SITAPI RGLBitmapFont RGL_CreateVGA8x8Font(const unsigned char* font_data);` | Creates an 8x8 VGA-style font from custom packed data. |
| `SITAPI RGLBitmapFont RGL_CreateVGA8x8FontWithOutline(const unsigned char* data, int outline_thickness);` | Creates a VGA-style font with an outline. |
| `SITAPI RGLBitmapFont RGL_CreateBitmapFontFromSystemFont(const char* font_name, int font_size, int char_width, int char_height);` | |
| `SITAPI RGLBitmapFont RGL_CreateTerminalFont(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char);` | |
| `SITAPI RGLBitmapFont RGL_CreateTerminalFontEx(const unsigned char* font_data, int char_width, int char_height, int char_count, int chars_per_row, int first_char, float char_spacing, float line_spacing);` | |
| `SITAPI RGLBitmapFont RGL_CreateASCIIFont(const unsigned char* font_data, int char_width, int char_height);` | |

### Font Rendering

| Signature | Description |
| --- | --- |
| `SITAPI RGLTexture RGL_StampTextToTextureAdvanced(const char* text, RGLTrueTypeFont font, Color text_color, Color bg_color, float wrap_width, int* out_width, int* out_height);` | Renders TTF text to a texture with word wrapping. |
| `SITAPI RGLTexture RGL_StampTextToTexture(const char* text, RGLBitmapFont font, Color text_color, Color bg_color, int* out_width, int* out_height);` | Renders text to a new texture. |
| `SITAPI void RGL_DrawTextEx(const char* text, vec2 position, RGLBitmapFont font, float spacing, Color color);` | Draws bitmap text with custom character spacing. |
| `SITAPI void RGL_DrawText(const char* text, vec2 position, RGLBitmapFont font, Color color);` | Draws text using a bitmap font. |
| `SITAPI void RGL_DrawTextTTF(const char* text, vec2 position, RGLTrueTypeFont font, Color color);` | Draws text using a high-quality baked TrueType font. |
| `SITAPI void RGL_DrawTextBoxed(const char* text, Rectangle bounds, RGLBitmapFont font, Color color, bool word_wrap);` | Draws text within a rectangle, with optional word wrapping. |
| `SITAPI void RGL_UnloadBitmapFont(RGLBitmapFont font);` | Unloads a bitmap font's texture atlas. |
| `SITAPI void RGL_UnloadTrueTypeFont(RGLTrueTypeFont font);` | Unloads a TrueType font's texture atlas. |

### Font Effects

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_DrawTextWithShadow(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color shadow_color, vec2 shadow_offset);` | Draws text with a drop shadow. |
| `SITAPI void RGL_DrawTextWithOutline(const char* text, vec2 position, RGLBitmapFont font, Color text_color, Color outline_color, float outline_thickness);` | Draws text with an outline. |
| `SITAPI void RGL_DrawTextGradient(const char* text, vec2 position, RGLBitmapFont font, Color top_color, Color bottom_color);` | Draws text with a vertical color gradient. |
| `SITAPI void RGL_DrawTextWave(const char* text, vec2 position, RGLBitmapFont font, Color color, float wave_amplitude, float wave_frequency, float time);` | Draws text with a sinusoidal wave effect. |

### Font Utilities

| Signature | Description |
| --- | --- |
| `SITAPI vec2 RGL_MeasureText(const char* text, RGLBitmapFont font);` | Measures the pixel dimensions of a string for a bitmap font. |
| `SITAPI vec2 RGL_MeasureTextTTF(const char* text, RGLTrueTypeFont font);` | Measures the pixel dimensions of a string for a TrueType font. |
| `SITAPI int RGL_GetTextLineCount(const char* text, RGLBitmapFont font, float max_width);` | |

## Math & Color Utilities

### General Math

| Signature | Description |
| --- | --- |
| `SITAPI float RGL_Lerp(float a, float b, float t);` | Linearly interpolates between two float values. |
| `SITAPI float RGL_Clamp(float value, float min, float max);` | Clamps a float value between a minimum and a maximum. |
| `SITAPI float RGL_Normalize(float value, float start, float end);` | Normalizes a value from a given range to the [0, 1] range. |
| `SITAPI float RGL_Remap(float value, float input_start, float input_end, float output_start, float output_end);` | Remaps a value from one range to another. |
| `SITAPI vec2 RGL_Vector2Lerp(vec2 a, vec2 b, float t);` | Linearly interpolates between two 2D vectors. |
| `SITAPI vec2 RGL_Vector2Rotate(vec2 v, float angle_degrees);` | Rotates a 2D vector by a given angle in degrees. |
| `SITAPI float RGL_Vector2Angle(vec2 v);` | Calculates the angle of a 2D vector in degrees. |
| `SITAPI bool RGL_IsPointInRectangle(vec2 point, Rectangle rect);` | Checks if a 2D point is inside a rectangle. |
| `SITAPI bool RGL_IsPointInCircle(vec2 point, vec2 center, float radius);` | Checks if a 2D point is inside a circle. |

### RGB Color

| Signature | Description |
| --- | --- |
| `SITAPI Color RGL_ColorFromHSV(float hue, float saturation, float value);` | Creates a Color from Hue, Saturation, and Value components. |
| `SITAPI Color RGL_ColorFromHex(unsigned int hex_value);` | Creates a Color from a 24-bit or 32-bit hexadecimal value. |
| `SITAPI vec3 RGL_ColorToHSV(Color color);` | Converts an RGBA Color to a vec3 of Hue, Saturation, and Value. |
| `SITAPI unsigned int RGL_ColorToHex(Color color);` | Converts a Color to its 32-bit hexadecimal value (AARRGGBB). |
| `SITAPI Color RGL_ColorLerp(Color c1, Color c2, float t);` | Linearly interpolates between two colors. |
| `SITAPI Color RGL_FadeColor(Color color, float alpha);` | Creates a faded version of a color by modifying its alpha. |
| `SITAPI Color RGL_ColorMultiply(Color c1, Color c2);` | Multiplies two colors component-wise. |
| `SITAPI Color RGL_ColorAdd(Color c1, Color c2);` | Adds two colors component-wise, clamping at 255. |
| `SITAPI Color RGL_ColorSubtract(Color c1, Color c2);` | Subtracts one color from another, clamping at 0. |
| `SITAPI Color RGL_ColorBrightness(Color color, float factor);` | Adjusts the brightness of a color by a multiplicative factor. |
| `SITAPI Color RGL_ColorContrast(Color color, float contrast);` | Adjusts the contrast of a color. |
| `SITAPI Color RGL_ColorSaturate(Color color, float saturation);` | Adjusts the saturation of a color. |
| `SITAPI Color RGL_ColorDesaturate(Color color);` | Converts a color to its perceptually-weighted grayscale equivalent. |
| `SITAPI Color RGL_ColorInvert(Color color);` | Inverts the RGB channels of a color. |
| `SITAPI Color RGL_ColorGamma(Color color, float gamma);` | Applies gamma correction to a color. |

### Color Palettes & Schemes

| Signature | Description |
| --- | --- |
| `SITAPI Color RGL_ColorFromPalette(const Color* palette, int palette_size, float t);` | Samples a color from a palette using linear interpolation. |
| `SITAPI void RGL_GenerateGradientPalette(Color start, Color end, Color* out_palette, int steps);` | Generates a palette by creating a linear gradient between two colors. |
| `SITAPI void RGL_GenerateRainbowPalette(Color* out_palette, int steps);` | Generates a vibrant rainbow palette by cycling through hue. |

### Color Analysis

| Signature | Description |
| --- | --- |
| `SITAPI float RGL_ColorLuminance(Color color);` | Calculates the perceptual luminance (brightness) of a color. |
| `SITAPI float RGL_ColorDistance(Color color1, Color color2);` | Calculates the Euclidean distance between two colors in RGB space. |
| `SITAPI bool RGL_ColorEquals(Color color1, Color color2, float tolerance);` | Checks if two colors are similar within a given distance tolerance. |
| `SITAPI Color RGL_ColorClosest(Color target, const Color* palette, int palette_size);` | Finds the color in a palette that is closest to a target color. |

### YPQ Color Space Utilities (For CRT/NTSC Emulation)

| Signature | Description |
| --- | --- |
| `SITAPI ColorYPQA RGL_YPQLerp(ColorYPQA color1, ColorYPQA color2, float t);` | Interpolates between two YPQ colors, preserving hue correctly. |
| `SITAPI ColorYPQA RGL_YPQAdjustLuminance(ColorYPQA color, float luminance_factor);` | Adjusts the brightness (Y channel) of a YPQ color. |
| `SITAPI ColorYPQA RGL_YPQAdjustPhase(ColorYPQA color, int phase_shift);` | Rotates the hue (P channel) of a YPQ color. |
| `SITAPI ColorYPQA RGL_YPQAdjustQuadrature(ColorYPQA color, float quad_factor);` | Adjusts the saturation (Q channel) of a YPQ color. |
| `SITAPI ColorYPQA RGL_YPQMultiply(ColorYPQA color1, ColorYPQA color2);` | Multiplies two YPQ colors component-wise. |
| `SITAPI Color RGL_ColorScanline(ColorYPQA color, float scanline_y, float intensity);` | Applies a scanline effect by dimming a YPQ color on alternating lines. |
| `SITAPI Color RGL_ColorTVNoise(ColorYPQA base_color, float noise_strength, vec2 screen_pos);` | Applies a procedural TV noise effect to a YPQ color. |
| `SITAPI Color RGL_ColorCRTBloom(ColorYPQA color, float bloom_strength);` | Applies a CRT phosphor bloom effect to a YPQ color. |
| `SITAPI Color RGL_ColorTVGhost(ColorYPQA color, float ghost_offset, float ghost_strength);` | Applies a TV signal ghosting effect to a YPQ color. |

### YPQ Palettes & Gradients

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_GenerateYPQGradient(ColorYPQA start, ColorYPQA end, Color* out_palette, int steps);` | Generates a palette by interpolating in YPQ space for more natural results. |
| `SITAPI ColorYPQA RGL_YPQFromTVChannel(int channel, float signal_strength);` | Generates a pseudo-random TV-like color based on channel and signal strength. |
| `SITAPI Color SituationColorFromYPQPalette(const ColorYPQA* ypq_palette, int palette_size, float t);` | Samples a color from a YPQ palette. |

### YPQ Analysis & Utilities

| Signature | Description |
| --- | --- |
| `SITAPI float RGL_YPQGetLuminance(ColorYPQA color)` | Gets the luminance (Y channel) of a YPQ color as a normalized float [0-1]. |
| `SITAPI float RGL_YPQGetChroma(ColorYPQA color)` | Gets the chroma/saturation (Q channel) of a YPQ color as a normalized float [0-1]. |
| `SITAPI float RGL_YPQGetHue(ColorYPQA color)` | Gets the hue/phase (P channel) of a YPQ color as degrees [0-360]. |
| `SITAPI bool RGL_YPQEquals(ColorYPQA color1, ColorYPQA color2, unsigned char tolerance);` | Checks if two YPQ colors are similar within a tolerance. |
| `SITAPI ColorYPQA RGL_YPQClosest(ColorYPQA target, const ColorYPQA* palette, int palette_size);` | Finds the closest color in a YPQ palette to a target YPQ color. |

### YPQ Constants for Classic ANSi Colors

| Signature | Description |
| --- | --- |
| `SITAPI ColorYPQA RGL_GetYPQBlack(void)` | Gets the YPQ representation of ANSI Black |
| `SITAPI ColorYPQA RGL_GetYPQRed(void)` | Gets the YPQ representation of ANSI Red |
| `SITAPI ColorYPQA RGL_GetYPQGreen(void)` | Gets the YPQ representation of ANSI Green |
| `SITAPI ColorYPQA RGL_GetYPQYellow(void)` | Gets the YPQ representation of ANSI Yellow (often brown) |
| `SITAPI ColorYPQA RGL_GetYPQBlue(void)` | Gets the YPQ representation of ANSI Blue |
| `SITAPI ColorYPQA RGL_GetYPQMagenta(void)` | Gets the YPQ representation of ANSI Magenta |
| `SITAPI ColorYPQA RGL_GetYPQCyan(void)` | Gets the YPQ representation of ANSI Cyan |
| `SITAPI ColorYPQA RGL_GetYPQWhite(void)` | Gets the YPQ representation of ANSI White (light gray) |
| `SITAPI ColorYPQA RGL_GetYPQBBlack(void)` | Gets the YPQ representation of ANSI Bright Black (dark gray) |
| `SITAPI ColorYPQA RGL_GetYPQBRed(void)` | Gets the YPQ representation of ANSI Bright Red |
| `SITAPI ColorYPQA RGL_GetYPQBGreen(void)` | Gets the YPQ representation of ANSI Bright Green |
| `SITAPI ColorYPQA RGL_GetYPQBYellow(void)` | Gets the YPQ representation of ANSI Bright Yellow |
| `SITAPI ColorYPQA RGL_GetYPQBBlue(void)` | Gets the YPQ representation of ANSI Bright Blue |
| `SITAPI ColorYPQA RGL_GetYPQBMagenta(void)` | Gets the YPQ representation of ANSI Bright Magenta |
| `SITAPI ColorYPQA RGL_GetYPQBCyan(void)` | Gets the YPQ representation of ANSI Bright Cyan |
| `SITAPI ColorYPQA RGL_GetYPQBWhite(void)` | Gets the YPQ representation of ANSI Bright White |

## Debug & Calibration Module

| Signature | Description |
| --- | --- |
| `SITAPI void RGL_SetDebugDrawTriggers(bool enabled);` | Sets whether invisible triggers (junctions, events) are visualized. |
| `SITAPI bool RGL_GetDebugDrawTriggers(void);` | Gets the current state of trigger visualization. |
| `SITAPI void RGL_ToggleDebugDrawTriggers(void);` | Toggles the visualization of invisible triggers. |
| `SITAPI void RGL_DrawPerformanceOverlay(void);` | Draws an overlay with real-time rendering statistics (FPS, draw calls, batch stats, etc.). |
| `SITAPI void RGL_DrawWireframeBounds(vec3 min_bounds, vec3 max_bounds, Color color);` | Draws a 3D wireframe bounding box for debugging. |
| `SITAPI void RGL_DrawPathDebugInfo(float player_z, bool show_control_points, bool show_splines);` | Renders an in-world debug view of the active path's structure. |
| `SITAPI void RGL_DrawShadowVolumeDebug(vec3 world_pos, vec2 size, const RGLShadowConfig* config);` | Renders a visualization of a stencil shadow volume for debugging. |
| `SITAPI void RGL_DrawLevelDebug(void);` | Renders a wireframe debug view of the active level's geometry. |
| `SITAPI void RGL_DrawTestPattern(const RGLTestPatternConfig* config);` | Draws a standard video test pattern for calibration and testing. |
| `SITAPI RGLTestPatternConfig RGL_GetDefaultTestPatternConfig(RGLTestPatternType type);` | Gets a default configuration for a specific test pattern type. |
| `SITAPI void RGL_DrawGrid(vec2 spacing, vec2 offset, Color color);` | Draws a 2D grid of lines for calibration |
| `SITAPI void RGL_DrawCheckerboard(Rectangle rect, vec2 tile_size, Color color1, Color color2);` | Fills a rectangle with a checkerboard pattern. |
| `SITAPI void RGL_DrawStripes(Rectangle rect, float stripe_width, bool vertical, Color color1, Color color2);` | Fills a rectangle with a stripe pattern. |
| `SITAPI void RGL_DrawSafeArea(Rectangle screen, float overscan_pct, Color color);` | Draws an outline representing the TV-safe area. |
| `SITAPI void RGL_DrawCrosshair(vec2 center, float size, float thickness, Color color);` | Draws a crosshair marker. |
| `SITAPI void RGL_DrawArrow(vec2 start, vec2 end, float head_size, float thickness, Color color);` | Draws a line with an arrowhead at the end. |
| `SITAPI void RGL_DrawRuler(vec2 start, vec2 end, float tick_spacing, float tick_length, Color color);` | Draws a ruler with tick marks for calibration. |
| `SITAPI void RGL_DrawLabeledRectangle(Rectangle rect, const char* label, RGLBitmapFont font, Color rect_color, Color text_color);` | Draws a rectangle with a centered text label. |
