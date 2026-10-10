/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger 2D OpenGL GUI Rendering Primitives
*/

#ifndef DEBUGGER_RENDER_H
#define DEBUGGER_RENDER_H

#include "Debugger_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 2D Primitives */
void dbg_draw_rect(float x, float y, float w, float h, float r, float g, float b, float a);
void dbg_draw_rect_outline(float x, float y, float w, float h, float r, float g, float b, float a, float lw);
void dbg_draw_line(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float lw);
void dbg_draw_circle(float cx, float cy, float radius, float r, float g, float b, float a);
void dbg_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a);

/* Text Rendering */
void dbg_draw_char(float x, float y, char c, float r, float g, float b, float scale);
void dbg_draw_text(float x, float y, const char *text, float r, float g, float b, float scale);

/* Interactive Controls */
int dbg_button(float x, float y, float w, float h, const char *text, float accent_r, float accent_g, float accent_b);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGER_RENDER_H */
