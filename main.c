/** FALLING SOMETHING
 *  \brief Pixels as particles of sand, water, fire, etc.
 *
 *  Author: sustainablelab with lots of help from bc
 *  Dependencies: SDL2
 *      Go to https://wiki.libsdl.org/SDL_blah for docs and examples
 *
 *  ---Cheatsheet---
 *
 *  Build and run:
 *      ;m<Space> -- same as doing :make
 *  Open func sig in SDL header:
 *      ;w Ctrl-]  - open in NEW WINDOW
 *      ;w Shift-] - open in PREVIEW WINDOW
 *
 *  New computer?
 *  1. Install build dependencies:
 *      $ pacman -S pkg-config
 *      $ pacman -S mingw-w64-x86_64-SDL2 
 *  2. Set up tags for hopping and omni-complete.
 *      Install ctags:
 *          $ pacman -S ctags
 *      Setup and update the tags files:
 *          :make tags # update tags for project src only
 *          :make lib-tags # updates tags for header file dependencies
 *      I separate into src and lib (header) tags for speedier src tag updates.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_video.h>

typedef uint32_t u32;
typedef uint8_t bool;

#define true 1
#define false 0

#define internal static // static functions are "internal"


// ------------
// | Math lib |
// ------------

int intmax(int a, int b)
{
    return (a < b) ? b : a;
}

int intmin(int a, int b)
{
    return (a < b) ? a : b;
}


// ---------------
// | Logging lib |
// ---------------

FILE *f;
internal void clear_log_file(void)
{
    f = fopen("log.txt", "w");
    fclose(f);
}

internal void log_to_file(char * log_msg)
{
    f = fopen("log.txt", "a");
    fprintf(f, log_msg);
    fclose(f);
}

#define MAX_LOG_MSG 1024
char log_msg[MAX_LOG_MSG];

// ---Logging game things---
bool log_me_xy = false;

// ---Logging SDL things---
internal void log_renderer_info(SDL_Renderer * renderer)
{
    // Get renderer info for logging.

    SDL_RendererInfo info;
    SDL_GetRendererInfo(
            renderer, // SDL_Renderer *
            &info // SDL_RendererInfo *
            );

    // Log name.
    log_to_file("Renderer info:\n");
    sprintf(log_msg, "\tname: %s\n", info.name);
    log_to_file(log_msg);

    // Log which flags are supported.
    log_to_file("\tSupported SDL_RendererFlags:\n");
    sprintf(log_msg,
            "\t\tSDL_RENDERER_SOFTWARE: %s\n",
            (info.flags & SDL_RENDERER_SOFTWARE) ? "True" : "False"
            );
    log_to_file(log_msg);
    sprintf(log_msg,
            "\t\tSDL_RENDERER_ACCELERATED: %s\n",
            (info.flags & SDL_RENDERER_ACCELERATED) ? "True" : "False"
            );
    log_to_file(log_msg);
    sprintf(log_msg,
            "\t\tSDL_RENDERER_PRESENTVSYNC: %s\n",
            (info.flags & SDL_RENDERER_PRESENTVSYNC) ? "True" : "False"
            );
    log_to_file(log_msg);
    sprintf(log_msg,
            "\t\tSDL_RENDERER_TARGETTEXTURE: %s\n",
            (info.flags & SDL_RENDERER_TARGETTEXTURE) ? "True" : "False"
            );
    log_to_file(log_msg);

    // Log texture info.
    sprintf(log_msg, "\tNumber of available texture formats: %d\n", info.num_texture_formats);
    log_to_file(log_msg);
    log_to_file("\tAvailable texture formats:\n");
    for (int i=0; i < info.num_texture_formats; i++)
    {
        u32 format = info.texture_formats[i];
        sprintf(log_msg, "\t\tName: %s, u32 value: %lu\n", SDL_GetPixelFormatName(format), format);
        log_to_file(log_msg);
        int bpp; // bits per pixel
        u32 Rmask;
        u32 Gmask;
        u32 Bmask;
        u32 Amask;
        bool conversion_OK = SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
        if (conversion_OK)
        {
            sprintf(log_msg, "\t\t\tbpp: %d\n", bpp);
            log_to_file(log_msg);
            sprintf(log_msg, "\t\t\tAmask: 0x%8X\n", Amask); // "%lu", Amask>>24
            log_to_file(log_msg);
            sprintf(log_msg, "\t\t\tRmask: 0x%8X\n", Rmask); // "%lu", Rmask>>16
            log_to_file(log_msg);
            sprintf(log_msg, "\t\t\tGmask: 0x%8X\n", Gmask); // "%lu", Gmask>>8
            log_to_file(log_msg);
            sprintf(log_msg, "\t\t\tBmask: 0x%8X\n", Bmask);
            log_to_file(log_msg);
        }
        else
        {
            log_to_file("\t\t\tFAIL: Cannot convert PixelFormatEnum to Masks.\n");
        }
    }
    sprintf(log_msg, "\tMax texture width: %lu\n", info.max_texture_width);
    log_to_file(log_msg);
    sprintf(log_msg, "\tMax texture height: %lu\n", info.max_texture_height);
    log_to_file(log_msg);
}


// ---------------
// | Drawing lib |
// ---------------

#define SCREEN_WIDTH 150
#define SCREEN_HEIGHT 100
#define PIXEL_SCALE 4
#define SCALED_SCREEN_WIDTH  (PIXEL_SCALE*SCREEN_WIDTH)
#define SCALED_SCREEN_HEIGHT (PIXEL_SCALE*SCREEN_HEIGHT)

/** Types of artwork
 *
 * Green cursor is a rect_t.
 * Everything else is drawn pixel-by-pixel Noita style.
 * Colors are ARGBA: 0xAARRGGBB.
 * x,y conventions:
 *  Cursor artwork: x is COL, y is ROW
 *  Pixel artwork:  x is ROW, y is COL
 */

// ------------------
// | Background art |
// ------------------

#define NOTHING_COLOR       0x00000000
#define OUT_OF_BOUNDS_COLOR 0x00000001
/* #define BGND_COLOR       0xFF221100 */
#define BGND_COLOR       0x00221100

// --------------
// | Cursor art |
// --------------

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} rect_t;

internal void FillRect(rect_t rect, u32 pixel_color, u32 *screen_pixels_prev)
{
    assert(screen_pixels_prev);
    for (int row=0; row < rect.h; row++)
    {
        for (int col=0; col < rect.w; col++)
        {
            screen_pixels_prev[ (row + rect.y)*SCREEN_WIDTH + (col + rect.x) ] = pixel_color;
        }
    }
}

// -------------
// | Pixel art |
// -------------

// Each pixel is a particle.

// Number of particles to simulate
#define NP 2000 // program aborts if NP > (SCREEN_WIDTH * SCREEN_HEIGHT)

// NTYPES: Number of particle types
#define NTYPES 4
#define ALL_TYPES NTYPES

enum particle_type
{
    SAND,
    SLIME,
    WATER,
    BRICK
};

// RGBA is not available!
/* #define SAND_COLOR  0xFFBB0010 */
/* #define WATER_COLOR 0x0088FF10 */
/* #define SLIME_COLOR 0xFF88FF10 */
/* #define BRICK_COLOR 0xFF000010 */

// ARGB8888
#define SAND_COLOR  0xFFFFBB00
#define WATER_COLOR 0xC00088FF
#define SLIME_COLOR 0xD0FF88FF
#define BRICK_COLOR 0xFFFF0000

static const u32 colors[NTYPES] = {
    SAND_COLOR,
    WATER_COLOR,
    SLIME_COLOR,
    BRICK_COLOR
};

/** How pixel coordinates work
 *
 * x is row number (vertical), with 0 at top of screen
 * y is col number, with 0 at left of screen
 *
 *  index of pixel at row x, col 0 is:
 *       row number * screen width
 *  then hop to pixel in column y:
 *       + y
 */

/**
 *  \brief Set pixel color in PREV buffer.
 *
 *  \param x    Screen row number (0 is top)
 *  \param y    Screen col number (0 is left)
 *  \param color    Color to set at this pixel
 *  \param screen_pixels    Pointer to the screen buffer to write to
 */
inline internal void ColorSetUnsafe(int x, int y, u32 color, u32 *screen_pixels)
{
    screen_pixels[x*SCREEN_WIDTH+y] = color;
}

/**
 *  \brief Get pixel color
 *
 *  \param x    Screen row number (0 is top)
 *  \param y    Screen col number (0 is left)
 *  \param screen_pixels    Pointer to the screen buffer
 *
 *  \return color   ARGB as unsigned 32-bit, or 1 if (x,y) is outside screen
 */
inline internal u32 ColorAt(int x, int y, u32 *screen_pixels)
{
    if ((x >= 0) && (y >= 0) && (x < SCREEN_HEIGHT) && (y < SCREEN_WIDTH))
    {
        return screen_pixels[x*SCREEN_WIDTH+y];
    }
    else // Pixel is outside screen area
    {
        // Any value that is NOT the "NOTHING_COLOR" acts as a boundary
        assert(NOTHING_COLOR != OUT_OF_BOUNDS_COLOR);
        return OUT_OF_BOUNDS_COLOR;
    }
}


/**
 *  \brief Initial position and drawing of particles in the screen buffer
 *
 *  \param screen_pixels    Pointer to the screen buffer to write to
 *  \param nseed_particles Number of particles to initialize
 *  \param type ALL_TYPES for all types or specify one type,
 *  e.g., SAND for sand only. For specific types, I reduce the
 *  footprint for where the new particles originate.
 */
internal void InitParticles(u32 * screen_pixels, u32 nseed_particles, enum particle_type type)
{
    // Sample nseeds
    for (u32 i=0; i < nseed_particles; i++)
    {
        // Pick new x,y
        int y = rand() % (SCREEN_WIDTH-1);  // random col
        int x = rand() % (SCREEN_HEIGHT-1); // random row in top-half of screen
        // Limit specific particles to starting at the top of the screen
        if (type != ALL_TYPES)
        {
            y = rand() % SCREEN_WIDTH/2 + SCREEN_WIDTH/4;
            x = rand() % SCREEN_HEIGHT/8;
        }
        // Only put new particles in empty space
        if (ColorAt(x, y, screen_pixels) == NOTHING_COLOR)
        {
            // Let SAND be any particles between 1/m and 1/n of screen width
            if ((type == SAND) || (type == ALL_TYPES))
            {
                if (
                    ((1.0/5.0)*SCREEN_WIDTH < y )
                    && (y < (3.0/5.0)*SCREEN_WIDTH)
                   )
                {
                    ColorSetUnsafe(x, y, SAND_COLOR, screen_pixels);
                }
            }
            // And let WATER be to the RIGHT of SAND.
            if ((type == WATER) || (type == ALL_TYPES))
            {
                if (
                    ((2.5/5.0)*SCREEN_WIDTH < y )
                    && (y < (4.0/5.0)*SCREEN_WIDTH)
                   )
                {
                    ColorSetUnsafe(x, y, WATER_COLOR, screen_pixels);
                }
            }
            // And let SLIME be to the far RIGHT.
            if ((type == SLIME) || (type == ALL_TYPES))
            {
                if (
                    ((3.5/5.0)*SCREEN_WIDTH < y )
                    && (y < (5.0/5.0)*SCREEN_WIDTH)
                   )
                {
                    ColorSetUnsafe(x, y, SLIME_COLOR, screen_pixels);
                }
            }
        }
    }
}

void internal DrawBorder(u32 * screen_pixels)
{
        // ---Draw a border of bricks---
        for (int x=0; x < SCREEN_HEIGHT; x++)
        {
            ColorSetUnsafe(x, 0, colors[BRICK], screen_pixels);
            ColorSetUnsafe(x, SCREEN_WIDTH-1, colors[BRICK], screen_pixels);
        }
        for (int y=0; y < SCREEN_WIDTH; y++)
        {
            ColorSetUnsafe(0, y, colors[BRICK], screen_pixels);
            ColorSetUnsafe(SCREEN_HEIGHT-1, y, colors[BRICK], screen_pixels);
        }
}

/**
 *  \brief Draw particles in NEXT based on PREV
 *
 */
internal void DrawParticles( u32 *screen_pixels_prev, u32 *screen_pixels_next)
{
    for (int row=0; row < SCREEN_HEIGHT; row++)
    {
        for (int col=0; col < SCREEN_WIDTH; col++)
        {
            u32 color             = ColorAt(row,   col,   screen_pixels_prev);
            u32 color_below       = ColorAt(row+1, col,   screen_pixels_prev);
            u32 color_below_right = ColorAt(row+1, col+1, screen_pixels_prev);
            u32 color_below_left  = ColorAt(row+1, col-1, screen_pixels_prev);
            u32 color_right       = ColorAt(row,   col+1, screen_pixels_prev);
            u32 color_left        = ColorAt(row,   col-1, screen_pixels_prev);
            // For WATER, also need to look at color in NEXT frame
            u32 color_next        = ColorAt(row,   col,   screen_pixels_next);
            u32 color_below_next  = ColorAt(row+1, col,   screen_pixels_next);
            u32 color_right_next  = ColorAt(row,   col+1, screen_pixels_next);
            u32 color_left_next   = ColorAt(row,   col-1, screen_pixels_next);
            int dy=0; // dy is 0, +1 or -1
            int dx=0; // dx is 0, +1 or -1
            switch (color)
            {

                case SAND_COLOR:
                    // Fall down if nothing is below.
                    if (color_below == NOTHING_COLOR)
                    {
                        dx = 1;
                        dy = 0;
                    }
                    // Stop falling straight down if SAND or BRICK is below.
                    if (
                            (color_below == SAND_COLOR)
                         || (color_below == BRICK_COLOR)
                       )
                    {
                        // If nothing on either side, pick a side at RANDOM:
                        if (
                               (color_below_right == NOTHING_COLOR)
                            && (color_below_left  == NOTHING_COLOR)
                           )
                        {
                            dx = 1;
                            // Pick a random left (-1) or right (+1)
                            dy = (rand()%2 == 1) ? 1 : -1;
                        }
                        // If nothing on left only, fall to the left:
                        if (
                               (color_below_right != NOTHING_COLOR)
                            && (color_below_left  == NOTHING_COLOR)
                           )
                        {
                            dx = 1;
                            dy = -1;
                        }
                        // If nothing on right only, fall to the right:
                        if (
                               (color_below_right == NOTHING_COLOR)
                            && (color_below_left  != NOTHING_COLOR)
                            )
                        {
                            dx = 1;
                            dy = 1;
                        }
                        // If something on both sides, don't fall.
                        if (
                               (color_below_right != NOTHING_COLOR)
                            && (color_below_left  != NOTHING_COLOR)
                           )
                        {
                            dx = 0;
                            dy = 0;
                        }
                    }
                    ColorSetUnsafe(row+dx, col+dy, color, screen_pixels_next);
                    break;

                case SLIME_COLOR:
                    // Fall down if nothing is below AND nothing
                    // will be below.
                    if (
                            (color_below == NOTHING_COLOR)
                         && (color_below_next == NOTHING_COLOR)
                       )
                    {
                        dx = 1;
                        /* dy = 0; */
                    }
                    // Stop falling if ANYTHING is below.
                    else
                    {
                        // Make SLIME sticky!
                        // Give SLIME a 1 out of 47 chance of moving.
                        bool is_moving = (rand()%47 == 1) ? true : false;

                        if (is_moving)
                        {

                            /* dx = 0; */
                            // If nothing on either side, pick a side at RANDOM:
                            if (
                                    (color_right      == NOTHING_COLOR)
                                 && (color_right_next == NOTHING_COLOR)
                                 && (color_left       == NOTHING_COLOR)
                                 && (color_left_next  == NOTHING_COLOR)
                               )
                            {
                                dy = (rand()%2 == 1) ? 1 : -1;
                            }
                            // If nothing on left only, flow left:
                            else if (
                                   (color_right      != NOTHING_COLOR)
                                && (color_left       == NOTHING_COLOR)
                                && (color_left_next  == NOTHING_COLOR)
                               )
                            {
                                dy = -1;
                            }
                            // If nothing on right only, flow right:
                            else if (
                                   (color_right      == NOTHING_COLOR)
                                && (color_right_next == NOTHING_COLOR)
                                && (color_left       != NOTHING_COLOR)
                               )
                            {
                                dy = 1;
                            }
                        }
                    }
                    ColorSetUnsafe(row+dx, col+dy, color, screen_pixels_next);
                    break;

                case WATER_COLOR:
                    // Fall down if nothing is below AND nothing
                    // will be below.
                    if (
                            (color_below == NOTHING_COLOR)
                         && (color_below_next == NOTHING_COLOR)
                       )
                    {
                        dx = 1;
                        /* dy = 0; */
                    }
                    // Stop falling if ANYTHING is below.
                    else
                    {
                        /* dx = 0; */
                        // If nothing on either side, pick a side at RANDOM:
                        if (
                                (color_right      == NOTHING_COLOR)
                             && (color_right_next == NOTHING_COLOR)
                             && (color_left       == NOTHING_COLOR)
                             && (color_left_next  == NOTHING_COLOR)
                           )
                        {
                            dy = (rand()%2 == 1) ? 1 : -1;
                        }
                        // If nothing on left only, flow left:
                        else if (
                               (color_right      != NOTHING_COLOR)
                            && (color_left       == NOTHING_COLOR)
                            && (color_left_next  == NOTHING_COLOR)
                           )
                        {
                            dy = -1;
                        }
                        // If nothing on right only, flow right:
                        else if (
                               (color_right      == NOTHING_COLOR)
                            && (color_right_next == NOTHING_COLOR)
                            && (color_left       != NOTHING_COLOR)
                           )
                        {
                            dy = 1;
                        }
                    }
                    ColorSetUnsafe(row+dx, col+dy, color, screen_pixels_next);
                    break;
                case BRICK_COLOR:
                    break;
                case NOTHING_COLOR:
                    break;
                default:
                    break;
            }
        }
    }
}


int main(int argc, char **argv)
{
    clear_log_file();

    // ---------------
    // | Game window |
    // ---------------
    log_to_file("Number of particles should not exceed number of pixels\n");
    sprintf(log_msg, "%d <= %d? ", NP, (SCREEN_WIDTH * SCREEN_HEIGHT));
    log_to_file(log_msg);
    sprintf(log_msg, "%s\n", (NP <= (SCREEN_WIDTH * SCREEN_HEIGHT)) ? "PASS" : "FAIL");
    log_to_file(log_msg);
    assert(NP <= (SCREEN_WIDTH * SCREEN_HEIGHT));
    sprintf(log_msg, "Draw pixel at %dx scale.\n", PIXEL_SCALE);
    log_to_file(log_msg);

    sprintf(log_msg, "\nOpen game window: %dx%d... ", SCALED_SCREEN_WIDTH, SCALED_SCREEN_HEIGHT);
    log_to_file(log_msg);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
            "h,j,k,l,H,J,K,L,Space,s,w,Up,Down,Esc", // const char *title
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, // int x, int y
            SCALED_SCREEN_WIDTH, SCALED_SCREEN_HEIGHT, // int w, int h,
            SDL_WINDOW_RESIZABLE // Uint32 flags
            );
    assert(win); log_to_file("OK\n");

    SDL_Renderer *renderer = SDL_CreateRenderer(
            win, // SDL_Window *
            -1, // int index
            SDL_RENDERER_ACCELERATED // Uint32 flags
            );
    assert(renderer); log_renderer_info(renderer);


    // RGBA8888 is not available!
    /* SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888); */
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

    // Confirm this format has an alpha channel
    assert(SDL_ISPIXELFORMAT_ALPHA(format->format));

    SDL_Texture *screen = SDL_CreateTexture(
            renderer, // SDL_Renderer *
            format->format, // Uint32 format,
            SDL_TEXTUREACCESS_TARGET, // int access,
            SCREEN_WIDTH, SCREEN_HEIGHT // int w, int h
            );
    assert(screen);

    SDL_SetTextureBlendMode(screen, SDL_BLENDMODE_BLEND);

    // Create a separate texture for background artwork.
    // For now, just a solid color.
    SDL_Texture *bgnd = SDL_CreateTexture(
            renderer, // SDL_Renderer *
            format->format, // Uint32 format,
            SDL_TEXTUREACCESS_TARGET, // int access,
            SCREEN_WIDTH, SCREEN_HEIGHT // int w, int h
            );
    assert(bgnd);

    SDL_SetTextureBlendMode(bgnd, SDL_BLENDMODE_BLEND);

    // Check the texture format
    u32 format_check;
    int access_check;
    int w_check;
    int h_check;
    SDL_QueryTexture(screen, &format_check, &access_check, &w_check, &h_check);
    sprintf(log_msg, "\tUsing texture format: %s\n", SDL_GetPixelFormatName(format_check));
    log_to_file(log_msg);
    sprintf(log_msg, "\tUsing texture access: %d\n", access_check);
    log_to_file(log_msg);

    u32 *screen_pixels_prev = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    assert(screen_pixels_prev);

    u32 *screen_pixels_next = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    assert(screen_pixels_next);

    u32 *bgnd_pixels = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    assert(bgnd_pixels);

    bool done = false;

    // ----------------
    // | INITIAL DRAW |
    // ----------------

    // ---------------------------
    // | Game graphics that move |
    // ---------------------------

    // Me
    /* int me_w = SCREEN_WIDTH/50; */
    int me_w = 2;
    /* int me_h = SCREEN_HEIGHT/50; */
    int me_h = 2;
    rect_t me = {
        // Center me on the screen:
        SCREEN_WIDTH/2 - me_w/2,
        SCREEN_HEIGHT/2 - me_h,
        me_w,
        me_h
    };
    // RGBA is not available!
    /* u32 me_color = 0x22FF00FF; */
    // ARGB
    /* u32 me_color = 0x8022FF00; */
    u32 me_color = 0x80FFFFFF;

    // ----------------------------------
    // | Game graphics that do not move |
    // ----------------------------------

    // Empty background
    rect_t empty_space = {0,0, SCREEN_WIDTH, SCREEN_HEIGHT};

    // ---------
    // | Noita |
    // ---------
    // Put a solid color in the background.
    FillRect(empty_space, BGND_COLOR, bgnd_pixels);
    // Clear the screen for InitParticles to have a clean canvas.
    FillRect(empty_space, NOTHING_COLOR, screen_pixels_prev);
    InitParticles(screen_pixels_prev, NP, ALL_TYPES);
    DrawBorder(screen_pixels_prev);

    // -----------------
    // | Game controls |
    // -----------------
    bool pressed_down  = false;
    bool pressed_up    = false;
    bool pressed_left  = false;
    bool pressed_right = false;


    // -------------
    // | GAME LOOP |
    // -------------
    while (!done)
    {
        // ----------------------
        // | Get keyboard input |
        // ----------------------

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) // Click window close
            {
                done = true;
            }

            SDL_Keycode code = event.key.keysym.sym;

            switch (code)
            {
                case SDLK_ESCAPE: // Esc - Quit
                    done = true;
                    break;

                case SDLK_UP: // Up - Grow me
                    me.w++;
                    me.h++;
                    // Clamp at 10x10
                    // Clamp at 1x1
                    if (me.w > 10) me.w=10;
                    if (me.h > 10) me.h=10;
                    break;
                case SDLK_DOWN: // Down - Shrink me
                    me.w--;
                    me.h--;
                    // Clamp at 1x1
                    if (me.w < 1) me.w=1;
                    if (me.h < 1) me.h=1;
                    break;

                case SDLK_SPACE: // Space - more particles
                    InitParticles(screen_pixels_prev, NP, ALL_TYPES);
                    break;

                case SDLK_s: // s - a little more sand
                    InitParticles(screen_pixels_prev, NP, SAND);
                    break;

                case SDLK_w: // w - a little more water
                    InitParticles(screen_pixels_prev, NP, WATER);
                    break;
                case SDLK_p: // p - a little more slime
                    InitParticles(screen_pixels_prev, NP, SLIME);
                    break;

                case SDLK_j: // j - move me down
                    /* pressed_down = true; */
                    pressed_down = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_k: // k - move me up
                    pressed_up = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_h: // h - move me left
                    pressed_left = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_l: // l - move me right
                    pressed_right = (event.type == SDL_KEYDOWN);
                    break;

                default:
                    break;
            }
        }

        // --------
        // | DRAW |
        // --------

        FillRect(empty_space, NOTHING_COLOR, screen_pixels_next); // Clear the screen
        DrawBorder(screen_pixels_next);
        DrawParticles(screen_pixels_prev, screen_pixels_next);

        // ---Draw me---
        //
        // TODO: control me speed
        // TODO: add small delay after initial press before repeating movement
        // TODO: change shape based on direction of movement
        if (pressed_down)
        {
            if (SDL_GetModState() & KMOD_SHIFT)
            {
                me.y = SCREEN_HEIGHT - me.h;
            }
            else
            {
                if ((me.y + me.h) < SCREEN_HEIGHT) // not at bottom yet
                {
                    me.y += me.h;
                }
                else // wraparound
                {
                    me.y = 0;
                }
            }
        }
        if (pressed_up)
        {
            if (SDL_GetModState() & KMOD_SHIFT)
            {
                me.y = 0;
            }
            else
            {
                if (me.y > me.h) // not at top yet
                {
                    me.y -= me.h;
                }
                else // wraparound
                {
                    me.y = SCREEN_HEIGHT - me.h;
                }
            }
        }
        if (pressed_left)
        {
            if (SDL_GetModState() & KMOD_SHIFT)
            {
                me.x = 0;
            }
            else
            {
                if (me.x > 0)
                {
                    me.x -= me.w;
                }
                else // moving left, wrap around to right sight of screen
                {
                    me.x = SCREEN_WIDTH - me.w;
                }
            }
        }
        if (pressed_right)
        {
            if (SDL_GetModState() & KMOD_SHIFT)
            {
                me.x = SCREEN_WIDTH - me.w;
            }
            else
            {
                if (me.x < (SCREEN_WIDTH - me.w))
                {
                    me.x += me.w;
                }
                else // moving right, wrap around to left sight of screen
                {
                    me.x = 0;
                }
            }
        }
        if (log_me_xy)
        {
            if (pressed_down || pressed_up || pressed_left || pressed_right)
            {
                sprintf(log_msg, "me (x,y) = (%d, %d)\n", me.x, me.y);
                log_to_file(log_msg);
            }
        }

        // Draw me in front of everything else
        FillRect(me, me_color, screen_pixels_next);
        /* FillRect(me, me_color, me_pixels); */

        /** BUFFER COPY
         *
         *  \brief Load screen[] with screen_next[]
         *
         *  Shift NEXT screen buffer into PREV screen buffer.
         *  (PREV screen buffer is rendered in SDL_UpdateTexture).
         */
        {
            u32 *tmp = screen_pixels_prev;
            screen_pixels_prev = screen_pixels_next;
            screen_pixels_next = tmp;
        }

        SDL_UpdateTexture(
                screen,        // SDL_Texture *
                NULL,          // const SDL_Rect * - NULL updates entire texture
                screen_pixels_prev, // const void *pixels
                SCREEN_WIDTH * sizeof(u32) // int pitch - n bytes in a row of pixel data
                );
        SDL_UpdateTexture(
                bgnd,        // SDL_Texture *
                NULL,          // const SDL_Rect * - NULL updates entire texture
                bgnd_pixels, // const void *pixels
                SCREEN_WIDTH * sizeof(u32) // int pitch - n bytes in a row of pixel data
                );
        SDL_RenderClear(renderer);
        SDL_RenderCopy(
                renderer, // SDL_Renderer *
                bgnd,   // SDL_Texture *
                NULL, // const SDL_Rect * - SRC rect, NULL for entire TEXTURE
                NULL  // const SDL_Rect * - DEST rect, NULL for entire RENDERING TARGET
                );
        SDL_RenderCopy(
                renderer, // SDL_Renderer *
                screen,   // SDL_Texture *
                NULL, // const SDL_Rect * - SRC rect, NULL for entire TEXTURE
                NULL  // const SDL_Rect * - DEST rect, NULL for entire RENDERING TARGET
                );
        SDL_RenderPresent(renderer);

        SDL_Delay(15); // sets frame rate

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
    // Why return 0? Tis what 'make' expects.
    //      Returning 0 avoids this:
    //      make: *** [Makefile:6: run] Error 144
}
