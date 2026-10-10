#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "../../../../include/lualib.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stb_image.h"

typedef struct {
    int frameIndex;
    float duration;
} EditorKeyframe;

typedef struct {
    char name[32];
    EditorKeyframe keyframes[256];
    int keyframe_count;
} EditorSequence;

typedef struct {
    unsigned int texture_id;
    int tex_w, tex_h;
    int frame_w, frame_h;
    char texture_path[256];
    char current_save_path[256];
    
    EditorSequence sequences[16];
    int sequence_count;
    int current_sequence_idx;
    int selected_key_idx;
    
    int playing;
    float playback_time;
} EditorState;

static EditorState state = {0, 0, 0, 64, 64, "", "animator.panim", {{ "idle", {{0, 0.2f}}, 1 }}, 1, 0, 0, 0, 0.0f};

static void load_texture_for_editor(const char *path) {
    int w, h, c;
    unsigned char *data = stbi_load(path, &w, &h, &c, 4);
    if (!data) return;
    if (state.texture_id != 0) glDeleteTextures(1, &state.texture_id);
    glGenTextures(1, &state.texture_id);
    glBindTexture(GL_TEXTURE_2D, state.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(data);
    state.tex_w = w; state.tex_h = h;
    strncpy(state.texture_path, path, 255);
}

static void save_animator(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite("PANIM\x04", 1, 6, f);
    int path_len = (int)strlen(state.texture_path);
    fwrite(&path_len, 4, 1, f);
    fwrite(state.texture_path, 1, path_len, f);
    fwrite(&state.frame_w, 4, 1, f);
    fwrite(&state.frame_h, 4, 1, f);
    fwrite(&state.sequence_count, 4, 1, f);
    for (int i = 0; i < state.sequence_count; i++) {
        int name_len = (int)strlen(state.sequences[i].name);
        fwrite(&name_len, 4, 1, f);
        fwrite(state.sequences[i].name, 1, name_len, f);
        fwrite(&state.sequences[i].keyframe_count, 4, 1, f);
        for (int k = 0; k < state.sequences[i].keyframe_count; k++) {
            fwrite(&state.sequences[i].keyframes[k].frameIndex, 4, 1, f);
            fwrite(&state.sequences[i].keyframes[k].duration, 4, 1, f);
        }
    }
    fclose(f);
    printf("[Editor] Saved Animator v4 to %s\n", path);
}

/* ISSUE: Missing validation on path_len, name_len, sequence_count, and keyframe_count allowed
   stack/heap buffer overflows and out-of-bounds array writes when loading malformed PANIM files.
   FIX: Added strict length bounds, header verification, and array capacity clamping. */
static void load_animator(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;
    char header[6];
    if (fread(header, 1, 6, f) != 6) { fclose(f); return; }
    if (memcmp(header, "PANIM", 5) != 0 || header[5] < '\x04') {
        fclose(f);
        printf("[Editor] Binary format invalid or too old for animator\n");
        return;
    }
    int path_len = 0;
    if (fread(&path_len, 4, 1, f) != 1 || path_len <= 0 || path_len >= (int)sizeof(state.texture_path)) {
        fclose(f);
        printf("[Editor] Invalid texture path length in animator file\n");
        return;
    }
    if ((int)fread(state.texture_path, 1, (size_t)path_len, f) != path_len) {
        fclose(f);
        return;
    }
    state.texture_path[path_len] = '\0';
    load_texture_for_editor(state.texture_path);

    if (fread(&state.frame_w, 4, 1, f) != 1 || fread(&state.frame_h, 4, 1, f) != 1) {
        fclose(f);
        return;
    }
    if (state.frame_w <= 0) state.frame_w = 64;
    if (state.frame_h <= 0) state.frame_h = 64;

    int seq_count = 0;
    if (fread(&seq_count, 4, 1, f) != 1) { fclose(f); return; }
    if (seq_count < 1) seq_count = 1;
    if (seq_count > 16) seq_count = 16;
    state.sequence_count = seq_count;

    for (int i = 0; i < state.sequence_count; i++) {
        int name_len = 0;
        if (fread(&name_len, 4, 1, f) != 1) break;
        if (name_len < 0 || name_len >= (int)sizeof(state.sequences[i].name)) {
            name_len = (int)sizeof(state.sequences[i].name) - 1;
        }
        if (name_len > 0 && (int)fread(state.sequences[i].name, 1, (size_t)name_len, f) != name_len) break;
        state.sequences[i].name[name_len] = '\0';

        int kf_count = 0;
        if (fread(&kf_count, 4, 1, f) != 1) break;
        if (kf_count < 1) kf_count = 1;
        if (kf_count > 256) kf_count = 256;
        state.sequences[i].keyframe_count = kf_count;

        for (int k = 0; k < state.sequences[i].keyframe_count; k++) {
            if (fread(&state.sequences[i].keyframes[k].frameIndex, 4, 1, f) != 1) break;
            if (fread(&state.sequences[i].keyframes[k].duration, 4, 1, f) != 1) break;
        }
    }
    fclose(f);
    state.current_sequence_idx = 0;
    state.selected_key_idx = 0;
    printf("[Editor] Loaded Animator: %s\n", path);
}

static void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b); glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

static void draw_editor_ui() {
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    draw_rect(-1.0f, -1.0f, 0.2f, 2.0f, 0.15f, 0.15f, 0.17f); /* Sidebar */
    draw_rect(-0.8f, -1.0f, 1.8f, 0.3f, 0.08f, 0.08f, 0.1f);  /* Timeline */

    if (state.current_sequence_idx < 0 || state.current_sequence_idx >= state.sequence_count) {
        state.current_sequence_idx = 0;
    }
    EditorSequence *cur_seq = &state.sequences[state.current_sequence_idx];
    float total_dur = 0;
    for (int i = 0; i < cur_seq->keyframe_count; i++) total_dur += cur_seq->keyframes[i].duration;

    float current_x = -0.75f;
    for (int i = 0; i < cur_seq->keyframe_count; i++) {
        float kw = (cur_seq->keyframes[i].duration / (total_dur + 0.001f)) * 1.7f;
        float r = 0.3f, g = 0.3f, b = 0.35f;
        if (i == state.selected_key_idx) { r = 0.8f; g = 0.5f; b = 0.2f; }
        draw_rect(current_x, -0.95f, kw - 0.01f, 0.2f, r, g, b);
        current_x += kw;
    }

    if (state.texture_id != 0 && state.tex_w > 0 && state.tex_h > 0 && state.frame_w > 0 && state.frame_h > 0) {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, state.texture_id); glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS); /* Sheet View */
        glTexCoord2f(0, 0); glVertex2f(-0.75f, 0.9f); glTexCoord2f(1, 0); glVertex2f(0.05f, 0.9f);
        glTexCoord2f(1, 1); glVertex2f(0.05f, 0.2f); glTexCoord2f(0, 1); glVertex2f(-0.75f, 0.2f);
        glEnd();

        float t = fmodf(state.playback_time, total_dur + 0.001f);
        int cur_frame = (cur_seq->keyframe_count > 0) ? cur_seq->keyframes[0].frameIndex : 0;
        float sum = 0;
        for (int i = 0; i < cur_seq->keyframe_count; i++) {
            if (t < sum + cur_seq->keyframes[i].duration) { cur_frame = cur_seq->keyframes[i].frameIndex; break; }
            sum += cur_seq->keyframes[i].duration;
        }
        /* ISSUE: Unchecked division state.tex_w / state.frame_w caused SIGFPE if frame_w was zero.
           FIX: Verified frame_w and frame_h are strictly positive before division. */
        int cols = state.tex_w / state.frame_w;
        if (cols <= 0) cols = 1;
        float u1 = (float)((cur_frame % cols) * state.frame_w) / (float)state.tex_w;
        float v1 = (float)((cur_frame / cols) * state.frame_h) / (float)state.tex_h;
        float u2 = u1 + (float)state.frame_w / (float)state.tex_w;
        float v2 = v1 + (float)state.frame_h / (float)state.tex_h;
        glBegin(GL_QUADS); /* Preview */
        glTexCoord2f(u1, v1); glVertex2f(0.3f, 0.8f); glTexCoord2f(u2, v1); glVertex2f(0.8f, 0.8f);
        glTexCoord2f(u2, v2); glVertex2f(0.8f, -0.1f); glTexCoord2f(u1, v2); glVertex2f(0.3f, -0.1f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods; if (action != GLFW_PRESS) return;
    EditorSequence *cur_seq = &state.sequences[state.current_sequence_idx];

    switch (key) {
        case GLFW_KEY_L: load_texture_for_editor("assets/test_sheet.png"); break;
        case GLFW_KEY_S: save_animator(state.current_save_path); break;
        case GLFW_KEY_O: load_animator(state.current_save_path); break;
        case GLFW_KEY_SPACE: state.playing = !state.playing; break;
        case GLFW_KEY_TAB: state.current_sequence_idx = (state.current_sequence_idx + 1) % state.sequence_count; state.selected_key_idx = 0;
                           printf("[Editor] Switched to sequence: %s\n", state.sequences[state.current_sequence_idx].name); break;
        case GLFW_KEY_N: if (state.sequence_count < 16) {
                            sprintf(state.sequences[state.sequence_count].name, "anim_%d", state.sequence_count);
                            state.sequences[state.sequence_count].keyframes[0] = (EditorKeyframe){0, 0.2f};
                            state.sequences[state.sequence_count].keyframe_count = 1;
                            state.current_sequence_idx = state.sequence_count++;
                            printf("[Editor] Added sequence: %s\n", state.sequences[state.current_sequence_idx].name);
                         } break;
        case GLFW_KEY_A: if (cur_seq->keyframe_count < 256) {
                            int ins = state.selected_key_idx + 1;
                            for(int i=cur_seq->keyframe_count; i > ins; i--) cur_seq->keyframes[i] = cur_seq->keyframes[i-1];
                            cur_seq->keyframes[ins] = (EditorKeyframe){0, 0.2f};
                            cur_seq->keyframe_count++; state.selected_key_idx = ins;
                         } break;
        case GLFW_KEY_DELETE: if (cur_seq->keyframe_count > 1) {
                            for(int i=state.selected_key_idx; i<cur_seq->keyframe_count-1; i++) cur_seq->keyframes[i] = cur_seq->keyframes[i+1];
                            cur_seq->keyframe_count--; if (state.selected_key_idx >= cur_seq->keyframe_count) state.selected_key_idx = cur_seq->keyframe_count - 1;
                         } break;
        case GLFW_KEY_RIGHT: if (state.selected_key_idx < cur_seq->keyframe_count - 1) state.selected_key_idx++; break;
        case GLFW_KEY_LEFT: if (state.selected_key_idx > 0) state.selected_key_idx--; break;
        case GLFW_KEY_UP: cur_seq->keyframes[state.selected_key_idx].frameIndex++; break;
        case GLFW_KEY_DOWN: if (cur_seq->keyframes[state.selected_key_idx].frameIndex > 0) cur_seq->keyframes[state.selected_key_idx].frameIndex--; break;
        case GLFW_KEY_EQUAL: cur_seq->keyframes[state.selected_key_idx].duration += 0.05f; break;
        case GLFW_KEY_MINUS: if (cur_seq->keyframes[state.selected_key_idx].duration > 0.05f) cur_seq->keyframes[state.selected_key_idx].duration -= 0.05f; break;
    }
}

int engine_launch_editor(lua_State *L, const char *mode) {
    (void)L; if (strcmp(mode, "animation") != 0) return 0;
    if (!glfwInit()) return 0;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "PyLua Animator Editor PRO", NULL, NULL);
    /* ISSUE: Leaked GLFW resources if window creation failed.
       FIX: Added glfwTerminate() call before returning on failure. */
    if (!window) {
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    printf("[Editor] Multi-Animation Mode Active\n");
    printf("  TAB: Cycle Sequences | N: New Sequence | SPACE: Play\n");
    printf("  A: Insert Key | DEL: Delete | Arrows: Change Frame | +/-: Duration\n");

    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        float delta = (float)(current_time - last_time);
        last_time = current_time;
        if (state.playing) state.playback_time += delta;
        draw_editor_ui(); glfwSwapBuffers(window); glfwPollEvents();
    }
    glfwTerminate(); return 1;
}
