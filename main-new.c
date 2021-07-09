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
// | Drawing lib |
// ---------------

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800


typedef struct
{
    int x;
    int y;
    int w;
    int h;
} rect_t;

#define NTYPES 2 // match length of enum

enum particle_type
{
    SAND,
    WATER
};

const static u32 colors[NTYPES] = {
    0xFFBB00FF, // sand
    0xFF0000FF, // water
};

inline internal enum particle_type ColorToType(u32 color)
{
    switch (color)
    {
        case colors[SAND]:
            return SAND;
        case colors[WATER]:
            return WATER;
        default:
            return NTYPES;
    }
}

// particles to carry
typedef struct
{
    int x;
    int y;
    enum particle_type type;
    int dir_y; // sloshing bit (side to side, ya know, y)
} particle_t;

static const u32 nothing_color = 0x00000000;
#define NP 10000

static particle_t particles[NP];


inline internal void ValueSetUnsafe(int x, int y, u32 color, u32 *screen_pixels_prev)
{
    screen_pixels_prev[x*SCREEN_WIDTH+y] = color;
}

inline internal u32 ValueAt(int x, int y, u32 *screen_pixels_prev)
{
    if ((x >= 0) && (y >= 0) && (x < SCREEN_HEIGHT) && (y < SCREEN_WIDTH))
    {
        return screen_pixels_prev[x*SCREEN_WIDTH+y];
    }
    else
    {
        return 1; // pixels outside the screen are a boundary
    }
}


internal void InitParticles(particle_t * particle, u32 n)
{
    for (u32 i=0; i<n; i++)
    {
        particles[i].type = rand() % NTYPES;
        /* if (particles[i].type == SAND) */
        /* { */
        /*     particles[i].y = rand() % SCREEN_WIDTH/2; */
        /*     particles[i].x = rand() % SCREEN_HEIGHT; */
        /* } */
        /* else */
        /* { */
        /*     particles[i].y = SCREEN_WIDTH/2 + rand() % SCREEN_WIDTH/2; */
        /*     particles[i].x = rand() % SCREEN_HEIGHT; */
        /* } */
        particles[i].dir_y = 0;
        particles[i].y = rand() % SCREEN_WIDTH;
        particles[i].x = rand() % SCREEN_HEIGHT;
    }
}

internal void DrawParticles(particle_t * particle, u32 n, u32 *screen_pixels_prev)
{
    for (u32 i=0; i<n; i++)
    {
        ValueSetUnsafe(particle[i].x, particle[i].y, colors[particle[i].type], screen_pixels_prev);
    }
}

internal void MoveParticles(
        particle_t * particle, u32 n,
        u32 *screen_pixels_prev, u32 *screen_pixels_next
        )
{
    memset(screen_pixels_next, 0, SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(u32));

    for (u32 i=0; i<n; i++)
    {
        int dx=0;
        int dy=0;
        // check cardinal directions
        // Down
        const u32 xbelow = particles[i].x + 1;
        const u32 xabove = particles[i].x - 1;
        switch (particles[i].type)
        {
            case SAND:
                /* if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) != nothing_color ) */
                if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) == colors[SAND] )
                {
                    // sand falling
                    // if below, go left or right
                    if (rand()%2)
                    {
                        // shift right if nothing below pixel to the right
                        if (ValueAt(xbelow, particles[i].y+1, screen_pixels_prev) == nothing_color )
                        {
                            dy += 1;
                        }
                    }
                    else
                    {
                        // shift left if nothing below pixel to the left
                        if ( ValueAt(xbelow, particles[i].y-1, screen_pixels_prev) == nothing_color )
                        {
                            dy -= 1;
                        }
                    }
                }
                else // pixel below is nothing or its water
                {
                    dx++;
                }
                break;

            case WATER:
                if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) != nothing_color )
                {
                    // if below, go left or right
                    if (rand()%2)
                    {
                        // shift right if nothing below pixel to the right
                        if (ValueAt(xbelow, particles[i].y+1, screen_pixels_prev) == nothing_color )
                        {
                            dy += 1;
                        }
                    }
                    else
                    {
                        // shift left if nothing below pixel to the left
                        if ( ValueAt(xbelow, particles[i].y-1, screen_pixels_prev) == nothing_color )
                        {
                            dy -= 1;
                        }
                    }
                }
                else // pixel below is something (sand or water)
                {
                    dx++;
                }
                break;

            case 3: // BROKEN_WATER:
                // TODO: add cohesion of water (water pulls water)
                // if NOT empty below
                if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) != nothing_color )
                {
                    // If blocked to RIGHT or LEFT, switch y-direction
                    if (
                        ( ValueAt(particles[i].x, particles[i].y+1, screen_pixels_prev) != nothing_color ) ||
                        ( ValueAt(particles[i].x, particles[i].y-1, screen_pixels_prev) != nothing_color )
                       )
                    {
                        particles[i].dir_y *= -1;
                    }
                    // If direction is 0, pick rand direction
                    // TODO: check direction of neighbors
                    if (particles[i].dir_y == 0)
                    {
                        particles[i].dir_y = rand()%2 ? 1 : -1;
                    }
                    // if going right
                    if (particles[i].dir_y > 0)
                    {
                        // AND nothing to the right, then go right
                        if ( ValueAt(xbelow, particles[i].y+1, screen_pixels_prev) == nothing_color )
                        {
                            dy += 1;
                        }
                        else
                        {
                            dy += particles[i].dir_y;
                        }
                    }
                    // if going left
                    else
                    {
                        // AND nothing to the left, then go left
                        if ( ValueAt(xbelow, particles[i].y-1, screen_pixels_prev) == nothing_color )
                        {
                            dy -= 1;
                        }
                        else
                        {
                            dy += particles[i].dir_y;
                        }
                    }
                }
                else // was nothing below
                {
                    dx++;
                }
                break;

            default:
                break;
        }
        // Temporary x and y
        int tmp_y = particles[i].y;
        tmp_y += dy;
        int tmp_x = particles[i].x;
        tmp_x += dx;
        tmp_x = intmax(0,intmin(SCREEN_HEIGHT-1, tmp_x));
        tmp_y = intmax(0,intmin(SCREEN_WIDTH -1, tmp_y));
        int tmp_type = particles[i].type;

        // Check screen mask to see if value already set at tmp x,y
        if (ValueAt(tmp_x, tmp_y, screen_pixels_next) == nothing_color)
        {
            // update particles
            particles[i].x = tmp_x;
            particles[i].y = tmp_y;
        }
        else
        {
            // Density behavior goes here
            // What's there now?
            u32 color_at_loc = ValueAt(tmp_x, tmp_y, screen_pixels_next)
            // If current particle is same type as particle at loc, just go up
            if (color_at_loc == tmp_color)
            {
                tmp_x
            }
            // If current particle is sand and particle at loc is water, SWAP --
            // water goes up
            // If sand replaced water, water goes UP
            // Search -- bubble up
            for (int xx = tmp_x; xx > 0; xx--)
            {
                if (ValueAt(xx,tmp_y, screen_pixels_next) == nothing_color)
                {
                    // update particles
                    particles[i].x = xx;
                    particles[i].y = tmp_y;
                    /* particles[i].type = tmp_type; */
                    break;
                }
            }


        }
        // Draw
        ValueSetUnsafe(
                particle[i].x,
                particle[i].y,
                colors[particle[i].type],
                screen_pixels_next
                );
    }
}

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

// -----------------------
// | Logging game things |
// -----------------------
bool log_me_xy = false;

// ----------------------
// | Logging SDL things |
// ----------------------

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
    sprintf(log_msg, "\tMax texture width: %d\n", info.max_texture_width);
    log_to_file(log_msg);
    sprintf(log_msg, "\tMax texture height: %d\n", info.max_texture_height);
    log_to_file(log_msg);
}


int main(int argc, char **argv)
{

    clear_log_file();

    // ---------------
    // | Game window |
    // ---------------
    sprintf(log_msg, "Open game window: %dx%d... ", SCREEN_WIDTH, SCREEN_HEIGHT);
    log_to_file(log_msg);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
            "h,j,k,l", // const char *title
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, // int x, int y
            SCREEN_WIDTH, SCREEN_HEIGHT, // int w, int h,
            SDL_WINDOW_RESIZABLE // Uint32 flags
            );
    assert(win); log_to_file("OK\n");

    SDL_Renderer *renderer = SDL_CreateRenderer(
            win, // SDL_Window *
            0, // int index
            SDL_RENDERER_SOFTWARE // Uint32 flags
            );
    assert(renderer); log_renderer_info(renderer);


    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

    SDL_Texture *screen = SDL_CreateTexture(
            renderer, // SDL_Renderer *
            format->format, // Uint32 format,
            SDL_TEXTUREACCESS_STREAMING, // int access,
            SCREEN_WIDTH, SCREEN_HEIGHT // int w, int h
            );
    assert(screen);

    u32 *screen_pixels_prev = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    assert(screen_pixels_prev);

    u32 *screen_pixels_next = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

    bool done = false;

    // ---------
    // | Noita |
    // ---------
    InitParticles(particles, NP);

    // ---------------------------
    // | Game graphics that move |
    // ---------------------------

    // Me
    int me_w = SCREEN_WIDTH/50;
    int me_h = SCREEN_HEIGHT/50;
    rect_t me = {
        // Center me on the screen:
        SCREEN_WIDTH/2 - me_w/2,
        SCREEN_HEIGHT/2 - me_h,
        me_w,
        me_h
    };
    u32 me_color = 0x22FF00FF; // RGBA

    // ----------------------------------
    // | Game graphics that do not move |
    // ----------------------------------

    // Background
    rect_t bgnd = {0,0, SCREEN_WIDTH, SCREEN_HEIGHT};

    // Debug LED
    int debug_led_w = SCREEN_WIDTH/50;
    int debug_led_h = SCREEN_HEIGHT/50;
    rect_t debug_led = {
        SCREEN_WIDTH - debug_led_w, // top left x
        0,                          // top left y
        debug_led_w,                // width
        debug_led_h                 // height
    };

    // LED starts out green. Turns red on vertical wraparound.
    u32 debug_led_color = 0x00FF0000; // green

    // -----------------
    // | Game controls |
    // -----------------
    bool pressed_down  = false;
    bool pressed_up    = false;
    bool pressed_left  = false;
    bool pressed_right = false;


    while (!done) // GAME LOOP
    {
        // ----------------------
        // | Get keyboard input |
        // ----------------------

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = true;
            }

            SDL_Keycode code = event.key.keysym.sym;

            switch (code)
            {
                case SDLK_ESCAPE:
                    done = true;
                    break;

                case SDLK_j:
                    /* pressed_down = true; */
                    pressed_down = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_k:
                    pressed_up = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_h:
                    pressed_left = (event.type == SDL_KEYDOWN);
                    break;

                case SDLK_l:
                    pressed_right = (event.type == SDL_KEYDOWN);
                    break;

                default:
                    break;
            }
        }

        // --------
        // | DRAW |
        // --------

        // Noita
        // Make particles fall!
        MoveParticles(particles, NP, screen_pixels_prev, screen_pixels_next);

        // ---Clear the screen---
        /* u32 bgnd_color = 0x111100FF; */
        /* u32 bgnd_color = nothing_color; */
        FillRect(bgnd, nothing_color, screen_pixels_prev);

        // swap screen_pixel buffers
        {
            u32 *tmp = screen_pixels_prev;
            screen_pixels_prev = screen_pixels_next;
            screen_pixels_next = tmp;
        }
        //
        // Use memset for quickly making the screen black:
        /* memset(screen_pixels_prev, 0, SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(u32)); */

        // ---Draw me---
        //
        // Act on keypresses OUTSIDE the SDL_PollEvent loop!
            // If I calc me position inside the SDL_PollEvent loop, me only
            // responds to one keystroke at a time.
            // For example, press hj to move me diagonal down and left. If I
            // calc me position inside the SDL_PollEvent loop, the movement is
            // spread out over two iterations of the GAME LOOP.

        // TODO: control me speed
        // TODO: add small delay after initial press before repeating movement
        // TODO: change shape based on direction of movement
        if (pressed_down)
        {
            if ((me.y + me.h) < SCREEN_HEIGHT) // not at bottom yet
            {
                me.y += me.h;
                debug_led_color = 0x00FF0000; // green
            }
            else // wraparound
            {
                me.y = 0;
                debug_led_color = 0xFF000000; // red
            }
        }
        if (pressed_up)
        {
            if (me.y > me.h) // not at top yet
            {
                me.y -= me.h;
                debug_led_color = 0x00FF0000; // green
            }
            else // wraparound
            {
                me.y = SCREEN_HEIGHT - me.h;
                debug_led_color = 0xFF000000; // red
            }
        }
        if (pressed_left)
        {
            if (me.x > 0)
            {
                me.x -= me.w;
                debug_led_color = 0x00FF0000; // green
            }
            else // moving left, wrap around to right sight of screen
            {
                me.x = SCREEN_WIDTH - me.w;
                debug_led_color = 0xFF000000; // red
            }
        }
        if (pressed_right)
        {
            if (me.x < (SCREEN_WIDTH - me.w))
            {
                me.x += me.w;
                debug_led_color = 0x00FF0000; // green
            }
            else // moving right, wrap around to left sight of screen
            {
                me.x = 0;
                debug_led_color = 0xFF000000; // red
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

        // Noita
        // ---Draw all the pixels here---
        // NO: drawing in screen_pixels_next
        /* DrawParticles(particles, NP, screen_pixels_prev); */

        FillRect(me, me_color, screen_pixels_prev);

        // ---Draw debug led---
        FillRect(debug_led, debug_led_color, screen_pixels_prev);

        SDL_UpdateTexture(
                screen,        // SDL_Texture *
                NULL,          // const SDL_Rect * - NULL updates entire texture
                screen_pixels_prev, // const void *pixels
                SCREEN_WIDTH * sizeof(u32) // int pitch - n bytes in a row of pixel data
                );
        SDL_RenderClear(renderer);
        SDL_RenderCopy(
                renderer, // SDL_Renderer *
                screen,   // SDL_Texture *
                NULL, // const SDL_Rect * - SRC rect, NULL for entire TEXTURE
                NULL  // const SDL_Rect * - DEST rect, NULL for entire RENDERING TARGET
                );
        SDL_RenderPresent(renderer);

        SDL_Delay(15); // sets frame rate

    }

    return 0;
    // Why return 0? Tis what 'make' expects.
    //      Returning 0 avoids this:
    //      make: *** [Makefile:6: run] Error 144
}
