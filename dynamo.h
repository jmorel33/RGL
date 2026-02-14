/**
 * @file dynamo.h
 * @brief A 2D/3D Physics Engine for the Situation Engine.
 *
 * @version 1.0
 * @date June 10, 2025
 *
 * @section overview Overview
 *   dynamo.h is a simple, data-oriented physics library designed to work alongside rgl.h.
 *   It provides basic tools for managing motion, gravity, and simple collisions in either
 *   a 2D (XY) or 3D (XYZ) context. It does not handle rendering.
 *
 * @section design_philosophy Design Philosophy
 *   - **Data-Oriented:** The library provides data structures (e.g., DynamoBody) and functions
 *     that operate on that data. Your game holds the state.
 *   - **Renderer-Agnostic:** Dynamo only cares about positions and velocities. It is the
 *     job of a rendering library like rgl.h to draw the objects.
 *   - **Simple & Fast:** Implements basic Euler integration, suitable for arcade-style physics.
 *     It is not a replacement for a complex physics engine like Box2D or Bullet.
 *
 * @section usage_example
 *
 *   // --- In your game's state ---
 *   DynamoBody player_body;
 *   Dynamo_InitBody(&player_body, (vec3){0, 100, 0}, 70.0f, 0.5f, 0.1f);
 *
 *   // --- In your game loop ---
 *   float delta_time = GetFrameTime();
 *
 *   // Apply player input forces
 *   if (IsKeyDown(KEY_RIGHT)) {
 *       Dynamo_ApplyForce(&player_body, (vec3){500.0f, 0, 0});
 *   }
 *
 *   // Update physics for all bodies
 *   Dynamo_Update3D(&player_body, delta_time);
 *
 *   // Check for and resolve collisions
 *   RGLGroundInfo ground;
 *   if (RGL_GetGroundAt((vec2){player_body.position[0], player_body.position[2]}, &ground)) {
 *       if (player_body.position[1] < ground.ground_y) {
 *           Dynamo_ResolveCollision(&player_body, ground.ground_y, ground.surface_normal);
 *       }
 *   }
 *
 *   // Draw the player using the body's position
 *   RGL_DrawBillboard(player_sprite, player_body.position, ...);
 */
#ifndef DYNAMO_H
#define DYNAMO_H

#include <cglm/cglm.h>
#include <stdbool.h>

// --- Configuration ---
#define DYNAMO_GRAVITY_3D ((vec3){0.0f, -9.81f, 0.0f}) // World units (meters) per second^2
#define DYNAMO_GRAVITY_2D ((vec2){0.0f, -9.81f})       // For 2D side-scrollers

// --- Public Types and Structs ---

/**
 * @brief Represents a single physical point-mass body in the world.
 * This is the core data structure for all physics operations.
 */
typedef struct {
    // State
    vec3 position;
    vec3 velocity;
    vec3 acceleration;

    // Properties
    float mass;         ///< In kilograms. Use 0 for a static/immovable object.
    float bounciness;   ///< Coefficient of restitution (0.0 to 1.0). 0=dead, 1=perfect bounce.
    float drag;         ///< Damping factor to simulate air resistance.

} DynamoBody;


// --- API Function Declarations ---

/**
 * @brief Initializes a DynamoBody with default values.
 * @param body Pointer to the body to initialize.
 * @param position The initial world-space position.
 * @param mass The mass in kg. Use 0 for a static body.
 * @param bounciness The coefficient of restitution (0.0 - 1.0).
 * @param drag The air resistance factor.
 */
SITAPI void Dynamo_InitBody(DynamoBody* body, vec3 position, float mass, float bounciness, float drag);

/**
 * @brief Applies a continuous force to a body (e.g., from a thruster or wind).
 * Force is applied according to F=ma (a = F/m).
 * @param body The body to apply the force to.
 * @param force The force vector to apply.
 */
SITAPI void Dynamo_ApplyForce(DynamoBody* body, vec3 force);

/**
 * @brief Applies an instantaneous change in velocity (e.g., from an explosion or jump).
 * @param body The body to apply the impulse to.
 * @param impulse The change in velocity vector.
 */
SITAPI void Dynamo_ApplyImpulse(DynamoBody* body, vec3 impulse);

/**
 * @brief Updates the position and velocity of a body in a 3D environment with gravity.
 * @param body The body to update.
 * @param delta_time The time elapsed since the last frame.
 */
SITAPI void Dynamo_Update3D(DynamoBody* body, float delta_time);

/**
 * @brief Updates the position and velocity of a body in a 2D (XY) environment with gravity.
 * @param body The body to update.
 * @param delta_time The time elapsed since the last frame.
 */
SITAPI void Dynamo_Update2D(DynamoBody* body, float delta_time);

/**
 * @brief Resolves a collision with a static surface (like the ground).
 * This function should be called after detecting a collision.
 * @param body The body that has collided.
 * @param contact_y The Y-coordinate of the surface the body has penetrated.
 * @param surface_normal The normal vector of the surface at the point of contact.
 */
SITAPI void Dynamo_ResolveCollision(DynamoBody* body, float contact_y, vec3 surface_normal);


// ===================================================================================
// --- IMPLEMENTATION ---
// ===================================================================================
#ifdef DYNAMO_IMPLEMENTATION

#include <string.h> // For memset

SITAPI void Dynamo_InitBody(DynamoBody* body, vec3 position, float mass, float bounciness, float drag) {
    if (!body) return;
    memset(body, 0, sizeof(DynamoBody));
    glm_vec3_copy(position, body->position);
    body->mass = (mass > 0.0f) ? mass : 0.0f; // Mass cannot be negative
    body->bounciness = bounciness;
    body->drag = drag;
}

SITAPI void Dynamo_ApplyForce(DynamoBody* body, vec3 force) {
    if (!body || body->mass == 0.0f) return;
    // a = F/m
    vec3 acceleration_change;
    glm_vec3_scale(force, 1.0f / body->mass, acceleration_change);
    glm_vec3_add(body->acceleration, acceleration_change, body->acceleration);
}

SITAPI void Dynamo_ApplyImpulse(DynamoBody* body, vec3 impulse) {
    if (!body || body->mass == 0.0f) return;
    glm_vec3_add(body->velocity, impulse, body->velocity);
}

SITAPI void Dynamo_Update3D(DynamoBody* body, float delta_time) {
    if (!body || body->mass == 0.0f) return;

    // Apply gravity to acceleration
    glm_vec3_add(body->acceleration, DYNAMO_GRAVITY_3D, body->acceleration);
    
    // Apply air drag (velocity-dependent force)
    vec3 drag_force;
    glm_vec3_scale(body->velocity, -body->drag, drag_force);
    Dynamo_ApplyForce(body, drag_force);

    // Update velocity from acceleration: v = v0 + at
    vec3 scaled_accel;
    glm_vec3_scale(body->acceleration, delta_time, scaled_accel);
    glm_vec3_add(body->velocity, scaled_accel, body->velocity);

    // Update position from velocity: p = p0 + vt
    vec3 scaled_vel;
    glm_vec3_scale(body->velocity, delta_time, scaled_vel);
    glm_vec3_add(body->position, scaled_vel, body->position);

    // Reset acceleration for the next frame (forces are re-applied each frame)
    glm_vec3_zero(body->acceleration);
}

SITAPI void Dynamo_Update2D(DynamoBody* body, float delta_time) {
    if (!body || body->mass == 0.0f) return;

    // Apply gravity to acceleration
    body->acceleration[0] += DYNAMO_GRAVITY_2D[0];
    body->acceleration[1] += DYNAMO_GRAVITY_2D[1];

    // Apply air drag
    vec3 drag_force = { -body->velocity[0] * body->drag, -body->velocity[1] * body->drag, 0.0f };
    Dynamo_ApplyForce(body, drag_force);

    // Update velocity from acceleration
    body->velocity[0] += body->acceleration[0] * delta_time;
    body->velocity[1] += body->acceleration[1] * delta_time;
    
    // Update position from velocity
    body->position[0] += body->velocity[0] * delta_time;
    body->position[1] += body->velocity[1] * delta_time;
    
    // Reset acceleration
    glm_vec3_zero(body->acceleration);
}

SITAPI void Dynamo_ResolveCollision(DynamoBody* body, float contact_y, vec3 surface_normal) {
    if (!body) return;
    
    // 1. Correct position to prevent sinking
    body->position[1] = contact_y;

    // 2. Calculate impulse for bouncing
    // Reflect the velocity vector across the surface normal
    float dot = glm_vec3_dot(body->velocity, surface_normal);
    if (dot < 0) { // Moving towards the surface
        vec3 reflection;
        glm_vec3_scale(surface_normal, -2.0f * dot, reflection);
        glm_vec3_add(body->velocity, reflection, body->velocity);

        // Apply bounciness (dampen the reflected velocity)
        glm_vec3_scale(body->velocity, body->bounciness, body->velocity);
    }
}

#endif // DYNAMO_IMPLEMENTATION
#endif // DYNAMO_H