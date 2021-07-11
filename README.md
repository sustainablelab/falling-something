- [FALLING SOMETHING](README.md#falling-something)
- [Concept](README.md#concept)
- [TO DO](README.md#to-do)

# FALLING SOMETHING

*Pixels as particles of sand, water, fire, etc.*

Run it:

    ./falling-something.exe

Quit it:

    Esc

Move the cursor around:

    h,j,k,l

The cursor **obliterates** pixels. Try *diving* through the sand
to stir up some action.

Teleport to the edges of the screen:

    H,J,K,L

Change the cursor size:

    Up-Arrow, Down-Arrow

*BUG*: playing with the big cursor eventually causes a seg fault

Add a lot more sand and water:

    Space

Add a little more sand or water:

    s,w


# Concept

Pixels are particles.

- **yellow** means **sand**
- **blue** means **water**

*Pixel **color** determines particle **type**.*

## Deal with the entire screen

Particle behavior comes from rules based on what is on the
screen. In the simple case where particles are blocked by any
space that is not empty, the rules are based on which pixels are
empty. In other words, look at a pixel, look at its neighbors,
and decide what to do.

That sounds obvious, but it's probably not the natural approach
using modern programming languages. The natural approach is to
create a Particle class and make an instance of Particle for each
particle on the screen. Particle instances track their own state
and calculate their next state internally. The abstraction of the
natural approach is appealing -- the screen is too low-level,
don't think about it. But I think this abstraction leads to
inefficient data processing. To be fair, I haven't tried it the
object-oriented way.

How do I track what is happening without creating Particle
objects? I pick the rule to apply based on the pixel color
(because pixel color implies particle type).

This simple, stateless scheme works for falling sand, but that's
about it. For more complex behavior, I also track momentum
(velocity).

## Double buffer

Nolla Games updates the falling sand simulation in the same frame
buffer that is being rendered. I'm not doing that because it
seems too complicated. Instead, I use two buffers, `screen[]` and
`screen_next[]`:

- `screen[]` is a buffer of all pixel colors on the screen
- `screen_next[]` is where I store calculate results for the next
  frame

The game loop flows like this:

- display `screen[]`
- calculate each pixel in next frame:
    - read old pixel values from `screen[]`
    - write new pixel values to `screen_next[]`
- after all pixels are calculated, copy `screen_next[]` to `screen[]`
- repeat

## Track color and momentum

Water is like sand but it flows. To simulate water flow, I need
to track particle momentum. And having particle momentum, I can
do more with the sand, like blast it upwards in the air and
impose gravity on its trajectory.

I think handling momentum *requires* state because I can't come
up with a purely position based rule for momentum. And Nolla
Games also tracks momentum (they call it velocity in the GDC
talk).

## Memory footprint

What is the memory footprint of double buffered color and
momentum values? Color is a 4-byte value: RGBA -- 8-bit red,
green, blue, alpha channels. Momentum is a 4-byte value: XXYY --
16-bit x and y momentum vectors. Each pixel has a color and a
momentum. I think of this as two layers of information for what
is on the screen: a color layer and a momentum layer. Since color
and momentum values are the same size, 4 bytes, these layers are
the same size: 4 bytes times the number of pixels on the screen.
Since the screen is double buffered, each screen layer is
doubled.

Summarizing the sizes of things:

- the screen has two layers (color and momentum)
- each layer has two buffers (current values and next values)
- each buffer has WxH pixels (the number of pixels on the screen)
- each pixel is 4 bytes (color and momentum are each 4-byte words)

Expressed as a dimensional analysis:

    // layers/screen * buffs/layer * pixels/buff * bytes/pixel
             2       *      2      *    WxH      *      4

Or simply put, the memory footprint is 16x the number of pixels
on the screen. Assuming 2k resolution (2048 x 1080 pixels),
that's about 35MB:

    16 * 2048 * 1080 = 35,389,440

That doesn't sound unreasonable to me, but I'll have to play
around and see.

## Color is also position

**Ignore momentum for a moment. Start by thinking about falling
sand because falling sand only depends on position.**

The pixels are on a screen. Represent the screen as an array:

    00 01 02 03 04 05 06 07 08 09
    10 11 12 13 14 15 16 17 18 19
    20 21 22 23 24 25 26 27 28 29
    30 31 32 33 34 35 36 37 38 39
    40 41 42 43 44 45 46 47 48 49

Above is a 5x10 screen, showing the index of each pixel. The
pixel color at the top left of the screen is screen[0], and the
pixel color at the bottom right is screen[49].

For this tiny screen thought experiment, I'm using base 10 and I
made the screen 10 pixels wide so that the index into the array
also has this alternate interpretation:

    row, col

In other words:

    top left is     (row,col) = (0,0)
    bottom right is (row,col) = (4,9)

In the more general case (screen is many pixels and screen width
is more than ten pixels), the relationship between the index into
the array and (row,col) position on the screen is not obvious, so
I need to transform from (row,col) coordinates to array index:

    index = (row * SCREEN_WIDTH) + col

Using the 50-pixel thought experiment, I check this formula is
correct.

*Example:* I expect (row,col) coordinate (3,4) is screen index
34. Test the transform works:

                  col ─┐
    SCREEN_WIDTH ─┐    │
      row ───┐    │    │
             │    │    │
    index = (3 * 10) + 4

And to avoid returning an invalid index:

```c
assert(    (index >= 0)
        && (index < (SCREEN_WIDTH * SCREEN_HEIGHT)
      )
```

*Example*: What does the screen look like with one grain of sand
at (3,4)?

Here is a visual for sand at (3,4):

    o o o o o o o o o o
    o o o o o o o o o o
    o o o o o o o o o o
    o o o o . o o o o o
    o o o o o o o o o o

Above, `o` is empty space and `.` is sand.

Instead of `o` and `.`, use 0xRGBA colors to represent empty space and sand:

    00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    00000000 00000000 00000000 00000000 FFBB00FF 00000000 00000000 00000000 00000000 00000000
    00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

The takeaway is I get position for free. This screen array
stores the color of every pixel on the screen, so with just this
one array, I know the position and color (type) of the particle.

The state displayed on the screen is saved in `screen`. I do my
*next frame* calculations based on `screen` but I write the
calculation results to `screen_next`. When the calculation is
done, I copy `screen_next` to `screen`. That eliminates the need
to be clever/careful about the order of the calculations.

## Pixel size

To make it easier to see what is going on, I scale the pixels up.

For example, here I make a window that is 50x100 pixels:

```c
#define SCREEN_WIDTH 50
#define SCREEN_HEIGHT 100
```

As far as my code is concerned, the screen is 50x100.

I define a pixel scale to make it easy to change the pixel size:

```c
#define PIXEL_SCALE 3
#define SCALED_SCREEN_WIDTH  (PIXEL_SCALE*SCREEN_WIDTH)
#define SCALED_SCREEN_HEIGHT (PIXEL_SCALE*SCREEN_HEIGHT)
```

The "scaled" value only has to show up in one place: creating the
window. When I create the window, I tell SDL to give me something
3x bigger than what I need. SDL automatically handles stretching
my "screen" to fit this enlarged window.

```c
    SDL_Window *win = SDL_CreateWindow(
            "h,j,k,l", // const char *title
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, // int x, int y
            SCALED_SCREEN_WIDTH, SCALED_SCREEN_HEIGHT, // int w, int h,
            SDL_WINDOW_RESIZABLE // Uint32 flags
            );
```

In fact, thanks to the `RESIZABLE` flag, I could also manually
set the window size by resizing the window with my mouse. The
effect is the same. But by passing a scaled size here, I can set
a precise scale factor.


## Sand

This is all it takes for nice piles of sand:

- FALL if there's nothing below
- if there's something below:
    - FALL towards the empty side if there's something below, but
      nothing one pixel to the side of that something
    - FALL towards a random side if the thing below is empty on both
      sides
- DO NOTHING if there is something below and something to either
  side of that

## Water

Water is similar but a little more tricky. The main difference is
that water flows sideways instead of falling diagonally. The
tricky part is that it is necessary to test for empty space using
both the current frame and the NEXT frame, otherwise water will
obliterate itself resulting in a single-pixel tall layer of
water.

For a first pass at water, I just want it to flow to find its own
level. I am not attempting any wave behavior yet.

- FALL if there's nothing below AND nothing will be below in the
  next frame
- If there's something below:
    - FLOW to whichever side is empty AND will continue to be
      empty in the next frame
    - FLOW towards a random side if both sides are empty AND will
      continue to be empty in the next frame
- if both sides are empty, pick one at random
- if neither side is empty, do nothing

This kind of works. The trouble is water is too slow at finding
its own level. Try holding `w` and watch the column of water
build up. I think adding wave behavior will fix this.

To add wave behavior, I eliminate the random choice. Once water
is flowing, it should continue to flow until it hits an obstacle.
In addition, if the obstacle is water, the water should
continue its motion but constructively interfere, i.e., the water
rides over the water in its way and continues moving in the same
direction.

Before I can implement wave behavior, I need to add momentum.

## Add momentum

Adding momentum is simple. Imagine the screen. Imagine a pixel on
the screen. That pixel is the current position of the particle.
Now draw two arrows from that pixel. One arrow runs along the
columns, the other arrow runs along the rows. The vector sum of
these two arrows is the momentum vector. The momentum vector
shows where the pixel will be on the next frame. In other words,
the next (row,col) position is the current position plus the
(Δrow,Δcol) momentum vector.

This will get into collision detection in the general case, so
for now, I'm starting with the simple case that will give water
wavelike behavior. In this simple case, the momentum vector is a
single pixel to the left or to the right.

# Rendering pipeline

This all started because I wanted transparency in my colors.

Water. It's blue but kind of see through. What should I see
through the water? In the 2D plane of the particles falling,
particles cannot go behind or in front of each other. So I need
to think about some amount of behind and in front before I can
define what I want alpha to do.

For example, maybe water should penetrate some distance into
neighboring sand. Over that penetration distance, I want water
in front of sand, and I want the color of the pixel to be a blend
of the water and sand colors.

Similarly, if sand falls into water, I want it to sink in the
water and again, the color of the sunken particle is a blend.

So what tools does SDL give me?

I think the only way to use the alpha channel for blending via
the SDL library is to put the things to blend on different
textures, set the blend mode for the textures, then copy the
textures to the renderer. Here is an example.

But for the example to be helpful, I need to explain a bit about
SDL.

I setup a **window**,  a **renderer**, a **texture**, and a
**buffer**. The data flows like this:

    make some pixel artwork in the buffer by setting the color of
    individual pixels
    -> copy buffer to texture
    -> copy texture to renderer
    -> present renderer in the window

What is the texture doing for me if I just copy it to the
renderer? `SDL_RenderCopy()` *expects* a texture. The texture
carries other information about the pixel artwork such as the
size of the texture in pixel dimensions and the pixel format.

The size of texture can simply be the size of the window, but it
doesn't have to be. I want the pixels to appear larger than
actual screen pixels, so I use different sizes for the pixel
dimensions of the window and the texture. I start the window at a
size scaled up from the size of the textures. I set this scale
with `PIXEL_SCALE`. In the example below, my texture is 150 x 100
pixels, but my window will be 600 x 400.

SDL supports many pixel formats, but on the two Windows 10
computers I've tried, `SDL_GetRendererInfo()` tells me that only
three formats are available:

    SDL_PIXELFORMAT_ARGB8888
    SDL_PIXELFORMAT_YV12
    SDL_PIXELFORMAT_IYUV

I'm using the ARGB8888. This means each pixel is an unsigned
four-byte value in the format `0xAARRGGBB`. The alpha byte is the
most significant byte, and the blue byte is the least
significant.

There is another reason for using textures, which gets to the
point of this example. The textures have a blend mode. If I make
multiple textures, I use blend mode to tell the GPU how to mix
the textures. The default is no blend, so whichever texture I
copy to the renderer last sits on top of the textures copied
before it. With one of the blend modes enabled, the GPU will
blend the textures, and the order in which I copy them also
affects this blending behavior.

First I set up the **window** and **renderer**.

```c
#define SCREEN_WIDTH 150
#define SCREEN_HEIGHT 100
#define PIXEL_SCALE 4
#define SCALED_SCREEN_WIDTH  (PIXEL_SCALE*SCREEN_WIDTH)
#define SCALED_SCREEN_HEIGHT (PIXEL_SCALE*SCREEN_HEIGHT)

int main(int arc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
            "h,j,k,l,H,J,K,L,Space,s,w,Up,Down,Esc", // const char *title
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, // int x, int y
            SCALED_SCREEN_WIDTH, SCALED_SCREEN_HEIGHT, // int w, int h,
            SDL_WINDOW_RESIZABLE // Uint32 flags
            );
    assert(win);

    SDL_Renderer *renderer = SDL_CreateRenderer(
            win, // SDL_Window *
            -1, // int index
            SDL_RENDERER_ACCELERATED // Uint32 flags
            );
    assert(renderer);
```

Note the window is scaled up.

For the renderer, I explicitly specify the
`SDL_RENDERER_ACCELERATED` flag to say "use hardware
acceleration" (i.e., use the GPU), but that flag is the default
value anyway.

Next, I need **buffers** and **textures** for my pixel artwork.

The buffer is just CPU memory that I set aside with `calloc`.
`calloc` is like `malloc` but `calloc` sets the memory to 0.

These are global buffers. I never free the buffers, I just keep
overwriting them. The memory will free when the program exits.

The particle artwork in the buffers is copied to the textures in
`SDL_UpdateTexture()`.

Continuing from the previous snippet:

```c
    // Do pixel artwork in these global buffers.
    u32 *layer_green_pixels = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    u32 *layer_red_pixels   = (u32*) calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

    // Set pixel format: 0xAARRGGBB
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

    // Create a texture for drawing a green shape
    SDL_Texture *layer_green = SDL_CreateTexture(
            renderer, // SDL_Renderer *
            format->format, // u32 SDL_PIXELFORMAT_ARGB8888
            SDL_TEXTUREACCESS_STREAMING, // Changes frequently
            SCREEN_WIDTH, SCREEN_HEIGHT // int w, int h
            );
    assert(layer_green);

    // Create a texture for drawing a red shape
    SDL_Texture *layer_red = SDL_CreateTexture(
            renderer, // SDL_Renderer *
            format->format, // u32 SDL_PIXELFORMAT_RGBA888
            SDL_TEXTUREACCESS_STREAMING, // Changes frequently
            SCREEN_WIDTH, SCREEN_HEIGHT // int w, int h
            );
    assert(layer_red);

    // Make these layers blend.
    // Default is no blend:
    //  -- one layer will completely hide the other.
    // There are four blend modes. I'm using alpha blend.
    // The math for each is in the comments in the header `SDL_blendmode.h`.
    SDL_SetTextureBlendMode(layer_green, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(layer_red, SDL_BLENDMODE_BLEND);
```

I'm making green and red shapes static images for illustrating
alpha, so I generate their artwork outside the game loop.


```c
    // Put big green rect on left side
    rect_t green_shape = {
        (1.0/4.0)*SCREEN_WIDTH,  // x top-left
        (1.0/4.0)*SCREEN_HEIGHT, // y top-left
        (1.0/2.0)*SCREEN_WIDTH,  // width
        (1.0/2.0)*SCREEN_HEIGHT, // height
    };
    // Offset smaller red rect to the right and down a bit
    rect_t red_shape = {
        (1.0/2.0)*SCREEN_WIDTH,  // x top-left
        (1.0/3.0)*SCREEN_HEIGHT, // y top-left
        (1.0/3.0)*SCREEN_WIDTH,  // width
        (1.0/3.0)*SCREEN_HEIGHT, // height
    };
    // Both buffers start off empty because calloc sets all bytes
    // to 0x00000000.
    // I only need to add color in the rect.
    // I use Alpha = 0x80 for half-transparent.
    FillRect(green_shape, 0x8000FF00, layer_green_pixels);
    FillRect(red_shape, 0x80FF0000, layer_red_pixels);

```

The updating happens at the end of the game loop. I have to keep
updating even though is static because other artwork in the game
is not static, so this would get lost after the first frame.

```c
        // Copy buffers to textures
        SDL_UpdateTexture(
                layer_green, // SDL_Texture *
                NULL, // NULL updates entire texture
                layer_green_pixels, // const void *pixels
                SCREEN_WIDTH * sizeof(u32) // int pitch
                );
        SDL_UpdateTexture(
                layer_red, // SDL_Texture *
                NULL, // NULL updates entire texture
                layer_red_pixels, // const void *pixels
                SCREEN_WIDTH * sizeof(u32) // int pitch
                );

        // Render the next frame
        SDL_RenderClear(renderer);

        SDL_RenderCopy(
                renderer, // SDL_Renderer *
                layer_green, // SDL_Texture *
                NULL, NULL
                );
        SDL_RenderCopy(
                renderer, // SDL_Renderer *
                layer_red, // SDL_Texture *
                NULL, NULL
                );

        SDL_RenderPresent(renderer);
```

With that set up, now I experiment with the blend modes. Mixing
green and red, I expected yellow but I got orange because I have
a brownish background texture (code for that background is not
shown in the snippets above).

The blendmode is alpha blend -- all blending is via the alpha
channel. If alpha is 0xFF, I get red on top of green, no
blending. So I use alpha 0x80 (about half) to mix.

Here is the math shown in the `SDL_blendmode.h` header:

```c
    SDL_BLENDMODE_BLEND = 0x00000001,    /**< alpha blending
                                              dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA))
                                              dstA = srcA + (dstA * (1-srcA)) */
```

To mix red and green to get yellow, I change the alphas channels
to 0xFF and use the ADD blendmode. ADD uses the alpha channel to
decide how much of the source to mix into the destination.

```c
    SDL_BLENDMODE_ADD = 0x00000002,      /**< additive blending
                                              dstRGB = (srcRGB * srcA) + dstRGB
                                              dstA = dstA */
```

# TO DO

- finish figuring out transparency
    - I figured out I need to use more than texture
    - the alpha is used to blend the textures
    - try playing with the blend mode
    - the order in which textures are copied to the renderer also
      makes a big difference
    - try getting the green "me" to look like it's in the water
      by using transparency -- I think it has to go on its own
      texture, but then I think I'll lose the particle erasing
      behavior
    - I need to add more complicated code for how the player
      interacts with the particles instead of the nice default
      behavior I get by putting them all on the same texture --
      pull them apart, then check for collisions and code the
      proper behavior
    - make the background a pattern instead of a single color so
      I can see the water and slime transparency
- add simple momentum for waves
- make waves
- add more general momentum
- test for collisions when using momentum to get new position
- add density behavior so that sand sinks in water
