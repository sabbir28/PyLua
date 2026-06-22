/*
** PyLua Engine - Physics Subsystem
** Provides 2D physics simulation including bodies, worlds, and collisions.
*/

#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

#include "../../../../include/lua.h"

typedef enum {
    BODY_STATIC,
    BODY_DYNAMIC
} BodyType;

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECT
} ShapeType;

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    ShapeType type;
    union {
        float radius; /* For circle */
        struct { float width, height; } rect; /* For rectangle (AABB) */
    } data;
} Shape;

typedef struct PhysicsBody {
    Vec2 position;
    Vec2 velocity;
    float mass;
    float inv_mass;
    float restitution;
    BodyType type;
    Shape shape;
    struct PhysicsBody *next;
} PhysicsBody;

typedef struct {
    PhysicsBody *a;
    PhysicsBody *b;
    float penetration;
    Vec2 normal;
} Manifold;

typedef struct {
    Vec2 gravity;
    PhysicsBody *bodies;
} PhysicsWorld;

/* Register the Physics sub-table into the engine table at stack top */
void engine_register_physics(lua_State *L);

#endif /* ENGINE_PHYSICS_H */
