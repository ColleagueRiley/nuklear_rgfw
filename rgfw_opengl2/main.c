/* nuklear - v1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION

#define OEMRESOURCE

#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"

#define NK_RGFW_GL2_IMPLEMENTATION
#include "nuklear_rgfw_gl2.h"

#define RGFW_IMPLEMENTATION
#include "RGFW.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the defines */
/* #define INCLUDE_ALL          */
/* #define INCLUDE_STYLE        */
/* #define INCLUDE_CALCULATOR   */
/* #define INCLUDE_CANVAS       */
/* #define INCLUDE_FILE_BROWSER */
/* #define INCLUDE_OVERVIEW     */
/* #define INCLUDE_NODE_EDITOR  */

#ifdef INCLUDE_ALL
  #define INCLUDE_STYLE
  #define INCLUDE_CALCULATOR
  #define INCLUDE_CANVAS
  #define INCLUDE_FILE_BROWSER
  #define INCLUDE_OVERVIEW
  #define INCLUDE_NODE_EDITOR
#endif

#ifdef INCLUDE_STYLE
  #include "../../demo/common/style.c"
#endif
#ifdef INCLUDE_CALCULATOR
  #include "../../demo/common/calculator.c"
#endif
#ifdef INCLUDE_CANVAS
  #include "../../demo/common/canvas.c"
#endif
#ifdef INCLUDE_FILE_BROWSER
  #include "../../demo/common/file_browser.c"
#endif
#ifdef INCLUDE_OVERVIEW
  #include "../../demo/common/overview.c"
#endif
#ifdef INCLUDE_NODE_EDITOR
  #include "../../demo/common/node_editor.c"
#endif

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(void)
{
    /* GUI */
    struct nk_context *ctx;
    struct nk_colorf bg;
#ifdef INCLUDE_FILE_BROWSER
    struct file_browser browser;
    struct media media;
#endif

    /* RGFW */
    RGFW_window* win = RGFW_createWindow("RGFW Demo", RGFW_RECT(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), RGFW_windowCenter);
    
    /* GUI */
    #ifdef API_OPENGL2
    ctx = nk_RGFW_init(win, NK_RGFW_INSTALL_CALLBACKS);
    #else
    ctx = nk_RGFW_init(win, NK_RGFW_INSTALL_CALLBACKS);
    #endif
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {struct nk_font_atlas *atlas;
    nk_RGFW_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_RGFW_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &droid->handle);*/}

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    #ifdef INCLUDE_FILE_BROWSER
    /* icons */
    glEnable(GL_TEXTURE_2D);
    media.icons.home = icon_load("../../demo/common/filebrowser/icon/home.png");
    media.icons.directory = icon_load("../../demo/common/filebrowser/icon/directory.png");
    media.icons.computer = icon_load("../../demo/common/filebrowser/icon/computer.png");
    media.icons.desktop = icon_load("../../demo/common/filebrowser/icon/desktop.png");
    media.icons.default_file = icon_load("../../demo/common/filebrowser/icon/default.png");
    media.icons.text_file = icon_load("../../demo/common/filebrowser/icon/text.png");
    media.icons.music_file = icon_load("../../demo/common/filebrowser/icon/music.png");
    media.icons.font_file =  icon_load("../../demo/common/filebrowser/icon/font.png");
    media.icons.img_file = icon_load("../../demo/common/filebrowser/icon/img.png");
    media.icons.movie_file = icon_load("../../demo/common/filebrowser/icon/movie.png");
    media_init(&media);

    file_browser_init(&browser, &media);
    #endif
    char buf[256] = {0};

    while (!RGFW_window_shouldClose(win))
    {
        /* Input */
        RGFW_window_checkEvents(win, RGFW_eventNoWait);
        nk_RGFW_new_frame();

        /* GUI */
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                bg = nk_color_picker(ctx, bg, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                nk_combo_end(ctx);
            }
          nk_edit_string_zero_terminated (ctx, NK_EDIT_FIELD, buf, 255, nk_filter_default);
        }
        nk_end(ctx);

        /* -------------- EXAMPLES ---------------- */
        #ifdef INCLUDE_CALCULATOR
          calculator(ctx);
        #endif
        #ifdef INCLUDE_CANVAS
          canvas(ctx);
        #endif
        #ifdef INCLUDE_FILE_BROWSER
          file_browser_run(&browser, ctx);
        #endif
        #ifdef INCLUDE_OVERVIEW
          overview(ctx);
        #endif
        #ifdef INCLUDE_NODE_EDITOR
          node_editor(ctx);
        #endif
        /* ----------------------------------------- */

        /* Draw */
        glViewport(0, 0, win->r.w, win->r.h);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        /* IMPORTANT: `nk_RGFW_render` modifies some global OpenGL state
         * with blending, scissor, face culling and depth test and defaults everything
         * back into a default state. Make sure to either save and restore or
         * reset your own state after drawing rendering the UI. */
        nk_RGFW_render(NK_ANTI_ALIASING_ON);
        RGFW_window_swapBuffers(win);
    }

    #ifdef INCLUDE_FILE_BROWSER
    glDeleteTextures(1,(const GLuint*)&media.icons.home.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.directory.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.computer.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.desktop.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.default_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.text_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.music_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.font_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.img_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&media.icons.movie_file.handle.id);

    file_browser_free(&browser);
    #endif

    nk_RGFW_shutdown();
    RGFW_window_close(win);
    return 0;
}
