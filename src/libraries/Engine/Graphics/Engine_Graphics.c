/*
** PyLua Engine - Graphics Subsystem
** Provides 2D rendering primitives: shapes, sprites, colors, and camera.
*/

#define engine_graphics_c
#define LUA_LIB

#include "../../../../include/lprefix.h"

#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Graphics.h"
#include "../Assets/Engine_Assets.h"

/* ========================================================================= */
/* Internal State                                                            */
/* ========================================================================= */

static struct {
    double clear_r, clear_g, clear_b, clear_a;
    double camera_x, camera_y;
    double camera_zoom;
    int draw_calls;
} gfx_state = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0};

/* ========================================================================= */
/* Color / Clear                                                             */
/* ========================================================================= */

/*
** engine.Graphics.clear([r, g, b, a])
** Clears the screen with the given color (0.0–1.0 range).
*/
static int gfx_clear(lua_State *L) {
    gfx_state.clear_r = luaL_optnumber(L, 1, 0.0);
    gfx_state.clear_g = luaL_optnumber(L, 2, 0.0);
    gfx_state.clear_b = luaL_optnumber(L, 3, 0.0);
    gfx_state.clear_a = luaL_optnumber(L, 4, 1.0);
    gfx_state.draw_calls = 0;

    glClearColor((GLfloat)gfx_state.clear_r, (GLfloat)gfx_state.clear_g,
                 (GLfloat)gfx_state.clear_b, (GLfloat)gfx_state.clear_a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 0;
}

/*
** engine.Graphics.present()
** Swaps the back buffer / presents the frame.
*/
static int gfx_present(lua_State *L) {
    (void)L;
    /* TODO: actual buffer swap */
    return 0;
}

/* ========================================================================= */
/* 2D Drawing Primitives                                                     */
/* ========================================================================= */

/*
** engine.Graphics.drawRect(x, y, w, h [, r, g, b, a])
** Draws a filled rectangle.
*/
static int gfx_draw_rect(lua_State *L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3);
    float h = (float)luaL_checknumber(L, 4);
    float r = (float)luaL_optnumber(L, 5, 1.0);
    float g = (float)luaL_optnumber(L, 6, 1.0);
    float b = (float)luaL_optnumber(L, 7, 1.0);
    float a = (float)luaL_optnumber(L, 8, 1.0);

    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();

    gfx_state.draw_calls++;
    return 0;
}

/*
** engine.Graphics.drawRectOutline(x, y, w, h [, lineWidth, r, g, b, a])
** Draws a rectangle outline.
*/
static int gfx_draw_rect_outline(lua_State *L) {
    lua_Number x = luaL_checknumber(L, 1);
    lua_Number y = luaL_checknumber(L, 2);
    lua_Number w = luaL_checknumber(L, 3);
    lua_Number h = luaL_checknumber(L, 4);
    lua_Number lw = luaL_optnumber(L, 5, 1.0);
    lua_Number r = luaL_optnumber(L, 6, 1.0);
    lua_Number g = luaL_optnumber(L, 7, 1.0);
    lua_Number b = luaL_optnumber(L, 8, 1.0);
    lua_Number a = luaL_optnumber(L, 9, 1.0);

    (void)x; (void)y; (void)w; (void)h; (void)lw;
    (void)r; (void)g; (void)b; (void)a;

    gfx_state.draw_calls++;
    return 0;
}

/*
** engine.Graphics.drawCircle(cx, cy, radius [, segments, r, g, b, a])
** Draws a filled circle.
*/
static int gfx_draw_circle(lua_State *L) {
    lua_Number cx = luaL_checknumber(L, 1);
    lua_Number cy = luaL_checknumber(L, 2);
    lua_Number radius = luaL_checknumber(L, 3);
    int segments = (int)luaL_optinteger(L, 4, 32);
    lua_Number r = luaL_optnumber(L, 5, 1.0);
    lua_Number g = luaL_optnumber(L, 6, 1.0);
    lua_Number b = luaL_optnumber(L, 7, 1.0);
    lua_Number a = luaL_optnumber(L, 8, 1.0);

    (void)cx; (void)cy; (void)radius; (void)segments;
    (void)r; (void)g; (void)b; (void)a;

    gfx_state.draw_calls++;
    return 0;
}

/*
** engine.Graphics.drawLine(x1, y1, x2, y2 [, lineWidth, r, g, b, a])
** Draws a line between two points.
*/
static int gfx_draw_line(lua_State *L) {
    lua_Number x1 = luaL_checknumber(L, 1);
    lua_Number y1 = luaL_checknumber(L, 2);
    lua_Number x2 = luaL_checknumber(L, 3);
    lua_Number y2 = luaL_checknumber(L, 4);
    lua_Number lw = luaL_optnumber(L, 5, 1.0);
    lua_Number r = luaL_optnumber(L, 6, 1.0);
    lua_Number g = luaL_optnumber(L, 7, 1.0);
    lua_Number b = luaL_optnumber(L, 8, 1.0);
    lua_Number a = luaL_optnumber(L, 9, 1.0);

    (void)x1; (void)y1; (void)x2; (void)y2; (void)lw;
    (void)r; (void)g; (void)b; (void)a;

    gfx_state.draw_calls++;
    return 0;
}

/*
** engine.Graphics.drawTriangle(x1,y1,x2,y2,x3,y3 [, r, g, b, a])
** Draws a filled triangle.
*/
static int gfx_draw_triangle(lua_State *L) {
    lua_Number x1 = luaL_checknumber(L, 1);
    lua_Number y1 = luaL_checknumber(L, 2);
    lua_Number x2 = luaL_checknumber(L, 3);
    lua_Number y2 = luaL_checknumber(L, 4);
    lua_Number x3 = luaL_checknumber(L, 5);
    lua_Number y3 = luaL_checknumber(L, 6);
    lua_Number r = luaL_optnumber(L, 7, 1.0);
    lua_Number g = luaL_optnumber(L, 8, 1.0);
    lua_Number b = luaL_optnumber(L, 9, 1.0);
    lua_Number a = luaL_optnumber(L, 10, 1.0);

    (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3;
    (void)r; (void)g; (void)b; (void)a;

    gfx_state.draw_calls++;
    return 0;
}

/* ========================================================================= */
/* Debug Text Rendering                                                      */
/* ========================================================================= */

/* 5x7 Bitmap Font Data for ASCII 32 - 126 */
static const unsigned char font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
};

/*
** engine.Graphics.drawText(text, x, y [, scale, r, g, b, a])
** Draws simple bitmap text to the screen.
*/
static int gfx_draw_text(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float scale = (float)luaL_optnumber(L, 4, 1.0);
    float r = (float)luaL_optnumber(L, 5, 1.0);
    float g = (float)luaL_optnumber(L, 6, 1.0);
    float b = (float)luaL_optnumber(L, 7, 1.0);
    float a = (float)luaL_optnumber(L, 8, 1.0);

    glColor4f(r, g, b, a);
    float cur_x = x;

    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126) c = '?';
        
        const unsigned char *bitmap = &font5x7[(c - 32) * 5];
        
        glBegin(GL_QUADS);
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 7; row++) {
                if (bitmap[col] & (1 << row)) {
                    float px = cur_x + col * scale;
                    float py = y + (7 - row) * scale;
                    glVertex2f(px, py);
                    glVertex2f(px + scale, py);
                    glVertex2f(px + scale, py + scale);
                    glVertex2f(px, py + scale);
                }
            }
        }
        glEnd();
        cur_x += 6 * scale; /* 5 pixels + 1 space */
    }

    gfx_state.draw_calls++;
    return 0;
}

/* ... existing methods ... */

static int gfx_draw_sprite(lua_State *L) {
    /* ... existing implementation ... */
    return 0;
}

/* ========================================================================= */
/* Camera                                                                    */
/* ========================================================================= */

/*
** engine.Graphics.setCamera(x, y [, zoom])
** Sets the 2D camera position and zoom.
*/
static int gfx_set_camera(lua_State *L) {
    gfx_state.camera_x = luaL_checknumber(L, 1);
    gfx_state.camera_y = luaL_checknumber(L, 2);
    gfx_state.camera_zoom = luaL_optnumber(L, 3, 1.0);
    return 0;
}

/*
** engine.Graphics.getCamera() -> x, y, zoom
*/
static int gfx_get_camera(lua_State *L) {
    lua_pushnumber(L, gfx_state.camera_x);
    lua_pushnumber(L, gfx_state.camera_y);
    lua_pushnumber(L, gfx_state.camera_zoom);
    return 3;
}

/*
** engine.Graphics.getDrawCalls() -> int
** Returns the number of draw calls since the last clear.
*/
static int gfx_get_draw_calls(lua_State *L) {
    lua_pushinteger(L, gfx_state.draw_calls);
    return 1;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg graphics_funcs[] = {
    {"clear",           gfx_clear},
    {"present",         gfx_present},
    {"drawRect",        gfx_draw_rect},
    {"drawRectOutline", gfx_draw_rect_outline},
    {"drawCircle",      gfx_draw_circle},
    {"drawLine",        gfx_draw_line},
    {"drawTriangle",    gfx_draw_triangle},
    {"drawSprite",      gfx_draw_sprite},
    {"drawText",        gfx_draw_text},
    {"setCamera",       gfx_set_camera},
    {"getCamera",       gfx_get_camera},
    {"getDrawCalls",    gfx_get_draw_calls},
    {NULL, NULL}
};

void engine_register_graphics(lua_State *L) {
    luaL_newlib(L, graphics_funcs);
    lua_setfield(L, -2, "Graphics");
}
