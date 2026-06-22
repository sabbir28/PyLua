/*
** PyLua Engine - Physics Subsystem
** Implementation of a simple 2D physics engine.
*/

#define engine_physics_c
#define LUA_LIB

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Physics.h"

#define ENGINE_PHYSICS_WORLD_MT "Engine.Physics.World"
#define ENGINE_PHYSICS_BODY_MT "Engine.Physics.Body"

/* ========================================================================= */
/* Physics Helpers                                                           */
/* ========================================================================= */

static float clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static float vec2_length_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

static float vec2_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

static Vec2 vec2_mul(Vec2 v, float s) {
    return (Vec2){v.x * s, v.y * s};
}

static Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

/* ========================================================================= */
/* Collision Detection                                                       */
/* ========================================================================= */

static int solve_circle_v_circle(Manifold *m) {
    Vec2 relative = vec2_sub(m->b->position, m->a->position);
    float dist_sq = vec2_length_sq(relative);
    float radius_sum = m->a->shape.data.radius + m->b->shape.data.radius;

    if (dist_sq > radius_sum * radius_sum) return 0;

    float dist = sqrtf(dist_sq);
    if (dist != 0) {
        m->penetration = radius_sum - dist;
        m->normal = vec2_mul(relative, 1.0f / dist);
    } else {
        m->penetration = m->a->shape.data.radius;
        m->normal = (Vec2){1, 0};
    }
    return 1;
}

static int solve_aabb_v_aabb(Manifold *m) {
    PhysicsBody *a = m->a;
    PhysicsBody *b = m->b;

    float a_min_x = a->position.x - a->shape.data.rect.width / 2;
    float a_max_x = a->position.x + a->shape.data.rect.width / 2;
    float b_min_x = b->position.x - b->shape.data.rect.width / 2;
    float b_max_x = b->position.x + b->shape.data.rect.width / 2;

    float x_overlap = fminf(a_max_x, b_max_x) - fmaxf(a_min_x, b_min_x);
    if (x_overlap <= 0) return 0;

    float a_min_y = a->position.y - a->shape.data.rect.height / 2;
    float a_max_y = a->position.y + a->shape.data.rect.height / 2;
    float b_min_y = b->position.y - b->shape.data.rect.height / 2;
    float b_max_y = b->position.y + b->shape.data.rect.height / 2;

    float y_overlap = fminf(a_max_y, b_max_y) - fmaxf(a_min_y, b_min_y);
    if (y_overlap <= 0) return 0;

    if (x_overlap < y_overlap) {
        m->penetration = x_overlap;
        m->normal = (a->position.x < b->position.x) ? (Vec2){-1, 0} : (Vec2){1, 0};
    } else {
        m->penetration = y_overlap;
        m->normal = (a->position.y < b->position.y) ? (Vec2){0, -1} : (Vec2){0, 1};
    }
    return 1;
}

static void resolve_collision(Manifold *m) {
    Vec2 relative_vel = vec2_sub(m->b->velocity, m->a->velocity);
    float vel_along_normal = vec2_dot(relative_vel, m->normal);

    if (vel_along_normal > 0) return;

    float e = fminf(m->a->restitution, m->b->restitution);
    float j = -(1 + e) * vel_along_normal;
    j /= m->a->inv_mass + m->b->inv_mass;

    Vec2 impulse = vec2_mul(m->normal, j);
    m->a->velocity = vec2_sub(m->a->velocity, vec2_mul(impulse, m->a->inv_mass));
    m->b->velocity = vec2_add(m->b->velocity, vec2_mul(impulse, m->b->inv_mass));

    /* Positional correction to prevent sinking */
    const float percent = 0.2f;
    const float slop = 0.01f;
    Vec2 correction = vec2_mul(m->normal, (fmaxf(m->penetration - slop, 0.0f) / (m->a->inv_mass + m->b->inv_mass)) * percent);
    m->a->position = vec2_sub(m->a->position, vec2_mul(correction, m->a->inv_mass));
    m->b->position = vec2_add(m->b->position, vec2_mul(correction, m->b->inv_mass));
}

/* ========================================================================= */
/* Lua API - World                                                           */
/* ========================================================================= */

static PhysicsWorld *check_world(lua_State *L, int idx) {
    return (PhysicsWorld *)luaL_checkudata(L, idx, ENGINE_PHYSICS_WORLD_MT);
}

static int phys_world_create(lua_State *L) {
    float gx = (float)luaL_optnumber(L, 1, 0.0);
    float gy = (float)luaL_optnumber(L, 2, -9.81);

    PhysicsWorld *world = (PhysicsWorld *)lua_newuserdatauv(L, sizeof(PhysicsWorld), 0);
    world->gravity = (Vec2){gx, gy};
    world->bodies = NULL;

    luaL_getmetatable(L, ENGINE_PHYSICS_WORLD_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int phys_world_step(lua_State *L) {
    PhysicsWorld *world = check_world(L, 1);
    float dt = (float)luaL_checknumber(L, 2);

    /* 1. Integration */
    for (PhysicsBody *b = world->bodies; b; b = b->next) {
        if (b->type == BODY_STATIC) continue;
        b->velocity = vec2_add(b->velocity, vec2_mul(world->gravity, dt));
        b->position = vec2_add(b->position, vec2_mul(b->velocity, dt));
    }

    /* 2. Collision detection and resolution */
    for (PhysicsBody *a = world->bodies; a; a = a->next) {
        for (PhysicsBody *b = a->next; b; b = b->next) {
            if (a->inv_mass == 0 && b->inv_mass == 0) continue;

            Manifold m;
            m.a = a;
            m.b = b;
            int collided = 0;

            if (a->shape.type == SHAPE_CIRCLE && b->shape.type == SHAPE_CIRCLE) {
                collided = solve_circle_v_circle(&m);
            } else if (a->shape.type == SHAPE_RECT && b->shape.type == SHAPE_RECT) {
                collided = solve_aabb_v_aabb(&m);
            }

            if (collided) {
                resolve_collision(&m);
            }
        }
    }

    return 0;
}

/* ========================================================================= */
/* Lua API - Body                                                            */
/* ========================================================================= */

static PhysicsBody *check_body(lua_State *L, int idx) {
    return (PhysicsBody *)luaL_checkudata(L, idx, ENGINE_PHYSICS_BODY_MT);
}

static int phys_world_add_body(lua_State *L) {
    PhysicsWorld *world = check_world(L, 1);
    int type = (int)luaL_checkinteger(L, 2);
    float x = (float)luaL_checknumber(L, 3);
    float y = (float)luaL_checknumber(L, 4);

    PhysicsBody *body = (PhysicsBody *)lua_newuserdatauv(L, sizeof(PhysicsBody), 0);
    memset(body, 0, sizeof(PhysicsBody));
    body->position = (Vec2){x, y};
    body->type = (BodyType)type;
    body->mass = (type == BODY_STATIC) ? 0.0f : 1.0f;
    body->inv_mass = (type == BODY_STATIC) ? 0.0f : 1.0f;
    body->restitution = 0.5f;
    body->shape.type = SHAPE_CIRCLE; /* Default */
    body->shape.data.radius = 0.5f;

    body->next = world->bodies;
    world->bodies = body;

    luaL_getmetatable(L, ENGINE_PHYSICS_BODY_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int phys_body_set_circle(lua_State *L) {
    PhysicsBody *body = check_body(L, 1);
    body->shape.type = SHAPE_CIRCLE;
    body->shape.data.radius = (float)luaL_checknumber(L, 2);
    return 0;
}

static int phys_body_set_rect(lua_State *L) {
    PhysicsBody *body = check_body(L, 1);
    body->shape.type = SHAPE_RECT;
    body->shape.data.rect.width = (float)luaL_checknumber(L, 2);
    body->shape.data.rect.height = (float)luaL_checknumber(L, 3);
    return 0;
}

static int phys_body_get_pos(lua_State *L) {
    PhysicsBody *body = check_body(L, 1);
    lua_pushnumber(L, (lua_Number)body->position.x);
    lua_pushnumber(L, (lua_Number)body->position.y);
    return 2;
}

static int phys_body_set_vel(lua_State *L) {
    PhysicsBody *body = check_body(L, 1);
    body->velocity.x = (float)luaL_checknumber(L, 2);
    body->velocity.y = (float)luaL_checknumber(L, 3);
    return 0;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg world_methods[] = {
    {"step", phys_world_step},
    {"addBody", phys_world_add_body},
    {NULL, NULL}
};

static const luaL_Reg body_methods[] = {
    {"getPosition", phys_body_get_pos},
    {"setVelocity", phys_body_set_vel},
    {"setCircle",   phys_body_set_circle},
    {"setRect",     phys_body_set_rect},
    {NULL, NULL}
};

static const luaL_Reg phys_funcs[] = {
    {"createWorld", phys_world_create},
    {NULL, NULL}
};

void engine_register_physics(lua_State *L) {
    /* World Metatable */
    luaL_newmetatable(L, ENGINE_PHYSICS_WORLD_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, world_methods, 0);
    lua_pop(L, 1);

    /* Body Metatable */
    luaL_newmetatable(L, ENGINE_PHYSICS_BODY_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, body_methods, 0);
    lua_pop(L, 1);

    /* Create the Physics sub-table */
    luaL_newlib(L, phys_funcs);
    lua_setfield(L, -2, "Physics");
}
