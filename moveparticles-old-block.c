internal void MoveParticles( u32 *screen_pixels_prev, u32 *screen_pixels_next)
{
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
                if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) != nothing_color )
                /* if ( ValueAt(xbelow, particles[i].y, screen_pixels_prev) == colors[SAND] ) */
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
                else // pixel below is something (sand or water)
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

        // Check screen mask to see if value already set at tmp x,y
        if (ValueAt(tmp_x, tmp_y, screen_pixels_next) == nothing_color)
        {
            // update particles
            particles[i].x = tmp_x;
            particles[i].y = tmp_y;

        }
        /* else if (ValueAt(tmp_x, tmp_y, screen_pixels_next) == colors[particle[i].type) */
        else
        {
            // Density behavior goes here
            // TEMP: search for spot to put particle in eight spaces around it
            bool done = false;
            // radius r, grow if can't find a home for this particle
            for (int r=0; (r < 3) && (!done); r++)
            {
                for(int xx = r-1; (xx <= (r + 1)) && (!done); xx++)
                {
                    if (abs(xx) < r) continue;
                    for (int yy = r-1; yy <= (r + 1); yy++)
                    {
                        if ( (abs(yy) < r) ) continue;

                        // Check if free space
                        if (ValueAt(tmp_x+xx, tmp_y+yy, screen_pixels_next) == nothing_color)
                        {
                            // update particles
                            particles[i].x = tmp_x+xx;
                            particles[i].y = tmp_y+yy;
                            done = true;
                            break;
                        }
                    }
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
