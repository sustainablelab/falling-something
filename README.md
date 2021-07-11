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
- add simple momentum for waves
- make waves
- add more general momentum
- test for collisions when using momentum to get new position
- add density behavior so that sand sinks in water
