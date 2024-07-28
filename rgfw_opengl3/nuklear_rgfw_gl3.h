/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_RGFW_GL3_H_
#define NK_RGFW_GL3_H_

#include "RGFW.h"

enum nk_RGFW_init_state{
    NK_RGFW_DEFAULT=0,
    NK_RGFW_INSTALL_CALLBACKS
};

#ifndef NK_RGFW_TEXT_MAX
#define NK_RGFW_TEXT_MAX 256
#endif

struct nk_RGFW_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture tex_null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

struct nk_RGFW {
    RGFW_window *win;
    int width, height;
    int display_width, display_height;
    struct nk_RGFW_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;
    unsigned int text[NK_RGFW_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
    double last_button_click;
    int is_double_click_down;
    struct nk_vec2 double_click_pos;
};

NK_API struct nk_context*   nk_RGFW_init(struct nk_RGFW* RGFW, RGFW_window *win, enum nk_RGFW_init_state);
NK_API void                 nk_RGFW_shutdown(struct nk_RGFW* RGFW);
NK_API void                 nk_RGFW_font_stash_begin(struct nk_RGFW* RGFW, struct nk_font_atlas **atlas);
NK_API void                 nk_RGFW_font_stash_end(struct nk_RGFW* RGFW);
NK_API void                 nk_RGFW_new_frame(struct nk_RGFW* RGFW);
NK_API void                 nk_RGFW_render(struct nk_RGFW* RGFW, enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);

NK_API void                 nk_RGFW_device_destroy(struct nk_RGFW* RGFW);
NK_API void                 nk_RGFW_device_create(struct nk_RGFW* RGFW);

NK_API void                 nk_RGFW_char_callback(RGFW_window *win, unsigned int codepoint);
NK_API void                 nk_gflw3_scroll_callback(RGFW_window *win, double xoff, double yoff);
NK_API void                 nk_RGFW_mouse_button_callback(RGFW_window *win, int button, int action, int mods);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_RGFW_GL3_IMPLEMENTATION
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef NK_RGFW_DOUBLE_CLICK_LO
#define NK_RGFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_RGFW_DOUBLE_CLICK_HI
#define NK_RGFW_DOUBLE_CLICK_HI 0.2
#endif

struct nk_RGFW_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

#ifdef __APPLE__
  #define NK_SHADER_VERSION "#version 150\n"
#else
  #define NK_SHADER_VERSION "#version 300 es\n"
#endif

NK_API void
nk_RGFW_device_create(struct nk_RGFW* RGFW)
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 TexCoord;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    struct nk_RGFW_device *dev = &RGFW->ogl;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_RGFW_vertex);
        size_t vp = offsetof(struct nk_RGFW_vertex, position);
        size_t vt = offsetof(struct nk_RGFW_vertex, uv);
        size_t vc = offsetof(struct nk_RGFW_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NK_INTERN void
nk_RGFW_device_upload_atlas(struct nk_RGFW* RGFW, const void *image, int width, int height)
{
    struct nk_RGFW_device *dev = &RGFW->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_RGFW_device_destroy(struct nk_RGFW* RGFW)
{
    struct nk_RGFW_device *dev = &RGFW->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

NK_API void
nk_RGFW_render(struct nk_RGFW* RGFW, enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    struct nk_RGFW_device *dev = &RGFW->ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)RGFW->width;
    ortho[1][1] /= (GLfloat)RGFW->height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0,0,(GLsizei)RGFW->display_width,(GLsizei)RGFW->display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        nk_size offset = 0;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_RGFW_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_RGFW_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_RGFW_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_RGFW_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_RGFW_vertex);
            config.tex_null = dev->tex_null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(&RGFW->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &RGFW->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * RGFW->fb_scale.x),
                (GLint)((RGFW->height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * RGFW->fb_scale.y),
                (GLint)(cmd->clip_rect.w * RGFW->fb_scale.x),
                (GLint)(cmd->clip_rect.h * RGFW->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, (const void*) offset);
            offset += cmd->elem_count * sizeof(nk_draw_index);
        }
        nk_clear(&RGFW->ctx);
        nk_buffer_clear(&dev->cmds);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API void
nk_RGFW_char_callback(RGFW_window *win, unsigned int codepoint)
{
    struct nk_RGFW* RGFW = RGFWGetWindowUserPointer(win);
    if (RGFW->text_len < NK_RGFW_TEXT_MAX)
        RGFW->text[RGFW->text_len++] = codepoint;
}

NK_API void
nk_gflw3_scroll_callback(RGFW_window *win, double xoff, double yoff)
{
    struct nk_RGFW* RGFW = RGFWGetWindowUserPointer(win);
    (void)xoff;
    RGFW->scroll.x += (float)xoff;
    RGFW->scroll.y += (float)yoff;
}

NK_API void
nk_RGFW_mouse_button_callback(RGFW_window* win, int button, int action, int mods)
{
    struct nk_RGFW* RGFW = RGFWGetWindowUserPointer(win);
    double x, y;
    NK_UNUSED(mods);
    if (button != RGFW_MOUSE_BUTTON_LEFT) return;
    RGFWGetCursorPos(win, &x, &y);
    if (action == RGFW_PRESS)  {
        double dt = RGFWGetTime() - RGFW->last_button_click;
        if (dt > NK_RGFW_DOUBLE_CLICK_LO && dt < NK_RGFW_DOUBLE_CLICK_HI) {
            RGFW->is_double_click_down = nk_true;
            RGFW->double_click_pos = nk_vec2((float)x, (float)y);
        }
        RGFW->last_button_click = RGFWGetTime();
    } else RGFW->is_double_click_down = nk_false;
}

NK_INTERN void
nk_RGFW_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    struct nk_RGFW* RGFW = (struct nk_RGFW*)usr.ptr;
    const char *text = RGFWGetClipboardString(RGFW->win);
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

NK_INTERN void
nk_RGFW_clipboard_copy(nk_handle usr, const char *text, int len)
{
    struct nk_RGFW* RGFW = (struct nk_RGFW*)usr.ptr;
    char *str = 0;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    RGFWSetClipboardString(RGFW->win, str);
    free(str);
}

NK_API struct nk_context*
nk_RGFW_init(struct nk_RGFW* RGFW, RGFW_window *win, enum nk_RGFW_init_state init_state)
{
    RGFWSetWindowUserPointer(win, RGFW);
    RGFW->win = win;
    if (init_state == NK_RGFW_INSTALL_CALLBACKS) {
        RGFWSetScrollCallback(win, nk_gflw3_scroll_callback);
        RGFWSetCharCallback(win, nk_RGFW_char_callback);
        RGFWSetMouseButtonCallback(win, nk_RGFW_mouse_button_callback);
    }
    nk_init_default(&RGFW->ctx, 0);
    RGFW->ctx.clip.copy = nk_RGFW_clipboard_copy;
    RGFW->ctx.clip.paste = nk_RGFW_clipboard_paste;
    RGFW->ctx.clip.userdata = nk_handle_ptr(&RGFW);
    RGFW->last_button_click = 0;
    nk_RGFW_device_create(RGFW);

    RGFW->is_double_click_down = nk_false;
    RGFW->double_click_pos = nk_vec2(0, 0);

    return &RGFW->ctx;
}

NK_API void
nk_RGFW_font_stash_begin(struct nk_RGFW* RGFW, struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&RGFW->atlas);
    nk_font_atlas_begin(&RGFW->atlas);
    *atlas = &RGFW->atlas;
}

NK_API void
nk_RGFW_font_stash_end(struct nk_RGFW* RGFW)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&RGFW->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_RGFW_device_upload_atlas(RGFW, image, w, h);
    nk_font_atlas_end(&RGFW->atlas, nk_handle_id((int)RGFW->ogl.font_tex), &RGFW->ogl.tex_null);
    if (RGFW->atlas.default_font)
        nk_style_set_font(&RGFW->ctx, &RGFW->atlas.default_font->handle);
}

NK_API void
nk_RGFW_new_frame(struct nk_RGFW* RGFW)
{
    int i;
    double x, y;
    struct nk_context *ctx = &RGFW->ctx;
    struct RGFW_window *win = RGFW->win;

    RGFWGetWindowSize(win, &RGFW->width, &RGFW->height);
    RGFWGetFramebufferSize(win, &RGFW->display_width, &RGFW->display_height);
    RGFW->fb_scale.x = (float)RGFW->display_width/(float)RGFW->width;
    RGFW->fb_scale.y = (float)RGFW->display_height/(float)RGFW->height;

    nk_input_begin(ctx);
    for (i = 0; i < RGFW->text_len; ++i)
        nk_input_unicode(ctx, RGFW->text[i]);

#ifdef NK_RGFW_GL3_MOUSE_GRABBING
    /* optional grabbing behavior */
    if (ctx->input.mouse.grab)
        RGFWSetInputMode(RGFW.win, RGFW_CURSOR, RGFW_CURSOR_HIDDEN);
    else if (ctx->input.mouse.ungrab)
        RGFWSetInputMode(RGFW->win, RGFW_CURSOR, RGFW_CURSOR_NORMAL);
#endif

    nk_input_key(ctx, NK_KEY_DEL, RGFWGetKey(win, RGFW_KEY_DELETE) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, RGFWGetKey(win, RGFW_KEY_ENTER) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, RGFWGetKey(win, RGFW_KEY_TAB) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, RGFWGetKey(win, RGFW_KEY_BACKSPACE) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, RGFWGetKey(win, RGFW_KEY_UP) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, RGFWGetKey(win, RGFW_KEY_DOWN) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_START, RGFWGetKey(win, RGFW_KEY_HOME) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_END, RGFWGetKey(win, RGFW_KEY_END) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_START, RGFWGetKey(win, RGFW_KEY_HOME) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_END, RGFWGetKey(win, RGFW_KEY_END) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, RGFWGetKey(win, RGFW_KEY_PAGE_DOWN) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_UP, RGFWGetKey(win, RGFW_KEY_PAGE_UP) == RGFW_PRESS);
    nk_input_key(ctx, NK_KEY_SHIFT, RGFWGetKey(win, RGFW_KEY_LEFT_SHIFT) == RGFW_PRESS||
                                    RGFWGetKey(win, RGFW_KEY_RIGHT_SHIFT) == RGFW_PRESS);

    if (RGFWGetKey(win, RGFW_KEY_LEFT_CONTROL) == RGFW_PRESS ||
        RGFWGetKey(win, RGFW_KEY_RIGHT_CONTROL) == RGFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, RGFWGetKey(win, RGFW_KEY_C) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, RGFWGetKey(win, RGFW_KEY_V) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, RGFWGetKey(win, RGFW_KEY_X) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, RGFWGetKey(win, RGFW_KEY_Z) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, RGFWGetKey(win, RGFW_KEY_R) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, RGFWGetKey(win, RGFW_KEY_LEFT) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, RGFWGetKey(win, RGFW_KEY_RIGHT) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, RGFWGetKey(win, RGFW_KEY_B) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, RGFWGetKey(win, RGFW_KEY_E) == RGFW_PRESS);
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, RGFWGetKey(win, RGFW_KEY_LEFT) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_RIGHT, RGFWGetKey(win, RGFW_KEY_RIGHT) == RGFW_PRESS);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
    }

    RGFWGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
#ifdef NK_RGFW_GL3_MOUSE_GRABBING
    if (ctx->input.mouse.grabbed) {
        RGFWSetCursorPos(RGFW->win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
#endif
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, RGFWGetMouseButton(win, RGFW_MOUSE_BUTTON_LEFT) == RGFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, RGFWGetMouseButton(win, RGFW_MOUSE_BUTTON_MIDDLE) == RGFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, RGFWGetMouseButton(win, RGFW_MOUSE_BUTTON_RIGHT) == RGFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)RGFW->double_click_pos.x, (int)RGFW->double_click_pos.y, RGFW->is_double_click_down);
    nk_input_scroll(ctx, RGFW->scroll);
    nk_input_end(&RGFW->ctx);
    RGFW->text_len = 0;
    RGFW->scroll = nk_vec2(0,0);
}

NK_API
void nk_RGFW_shutdown(struct nk_RGFW* RGFW)
{
    nk_font_atlas_clear(&RGFW->atlas);
    nk_free(&RGFW->ctx);
    nk_RGFW_device_destroy(RGFW);
    memset(RGFW, 0, sizeof(*RGFW));
}

#endif
