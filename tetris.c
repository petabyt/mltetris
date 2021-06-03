#include <stdio.h>
#include <dryos.h>
#include <module.h>
#include <menu.h>
#include <config.h>
#include <bmp.h>
#include <console.h>
#include <math.h>

int running = 0;
int score = 0;

/*
Tetris for Zx3
(C) 2010 Jeroen Domburg (jeroen AT spritesmods.com)
This program is free software: you can redistribute it and/or modify
t under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
This was ported to Magic Lantern by Daniel C (petabyte.heb12.com).
Find the original unmodified source here: http://spritesmods.com/zx3hack/zx3-hack_src.tgz

- Current high score: 34 on speed 200
*/

#define BLOCK_HEIGHT 20
#define BLOCK_WIDTH 20
#define SPEED 200

struct playfield_t
{
    unsigned char field[10][20];
    unsigned char currBlock[4][4];
    int currBlockX, currBlockY;
};

unsigned int palette[8] =
{
    COLOR_BLACK, /* Background color */
    COLOR_RED,
    COLOR_GREEN1,
    COLOR_ORANGE,
    COLOR_DARK_RED,
    COLOR_MAGENTA,
    COLOR_ORANGE,
    COLOR_WHITE
};

unsigned char availBlocks[7][4][4] =
{
    {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {2, 2, 2, 0},
        {0, 0, 2, 0},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {0, 3, 3, 3},
        {0, 3, 0, 0},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {0, 4, 4, 0},
        {0, 4, 4, 0},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {0, 5, 5, 0},
        {5, 5, 0, 0},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {0, 6, 6, 0},
        {0, 0, 6, 6},
        {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0},
        {0, 7, 7, 7},
        {0, 0, 7, 0},
        {0, 0, 0, 0},
    }
};

/*
Routines to play Tetris. I'm too lame to document them now; if you
are interested in reverse engineering, you probably have no problem figuring
out what these do yourself :P
*/

void fieldClear(playfield_t* field)
{
    int x, y;
    for (x = 0; x < 10; x++)
    {
        for (y = 0; y < 20; y++)
        {
            field->field[x][y] = 0;
        }
    }

    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            field->currBlock[x][y] = 0;
        }
    }
}

int fieldIsPossible(playfield_t* field)
{
    int px, py;
    int x, y;
    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            if (field->currBlock[x][y] != 0)
            {
                px = x + field->currBlockX;
                py = y + field->currBlockY;
                if (px < 0 || px >= 10 || py >= 20)
                {
                    return 0;
                }

                if (py >= 0 && field->field[px][py] != 0)
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void fieldDup(playfield_t *dest, playfield_t *src)
{
    memcpy(dest, src, sizeof(playfield_t));
}

void fieldRotateBlock(playfield_t *field, int dir)
{
    unsigned char buff[4][4];
    int x, y;
    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            buff[x][y] = field->currBlock[x][y];
        }
    }

    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            if (dir > 0)
            {
                field->currBlock[x][y] = buff[3 - y][x];
            }

            else
            {
                field->currBlock[x][y] = buff[y][3 - x];
            }
        }
    }
}

void fieldSelectBlock(playfield_t *field, int blkNo)
{
    int x, y;
    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            field->currBlock[x][y] = availBlocks[blkNo][x][y];
        }
    }

    field->currBlockX = 3;
    field->currBlockY = -2;
}

int checkAndKillALine(playfield_t* field)
{
    int x, y, ry;
    int found;
    for (y = 0; y < 20; y++)
    {
        found = 1;
        for (x = 0; x < 10; x++)
        {
            if (field->field[x][y] == 0)
                found = 0;
        }

        if (found)
        {
            /* Remove line */
            for (ry = y; ry >= 0; ry--)
            {
                for (x = 0; x < 10; x++)
                {
                    if (ry != 0)
                    {
                        field->field[x][ry] = field->field[x][ry - 1];
                    }
                    else
                    {
                        field->field[x][ry] = 0;
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

int fieldFixBlock(playfield_t *field)
{
    int px, py;
    int x, y;
    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            px = x + field->currBlockX;
            py = y + field->currBlockY;
            if (field->currBlock[x][y] != 0)
            {
                field->field[px][py] = field->currBlock[x][y];
            }
        }
    }

    while (checkAndKillALine(field))
    {
        score++;
    }

    return 1;
}

void placeBlock(int col, int bx, int by)
{
    bmp_fill(palette[col & 7], bx * BLOCK_WIDTH, by * BLOCK_HEIGHT, BLOCK_WIDTH, BLOCK_HEIGHT);
}

/* Display a Tetris playing field */
void display(playfield_t *field)
{
    int x, y;
    int px, py;
    for (x = 0; x < 10; x++)
    {
        for (y = 0; y < 20; y++)
        {
            placeBlock(field->field[x][y], x, y);
        }
    }

    for (x = 0; x < 4; x++)
    {
        for (y = 0; y < 4; y++)
        {
            px = x + field->currBlockX;
            py = y + field->currBlockY;
            if (field->currBlock[x][y] != 0)
            {
                if (px >= 0 && py >= 0 && px < 10 && py < 20)
                {
                    placeBlock(field->currBlock[x][y], px, py);
                }
            }
        }
    }

    bmp_printf(FONT_LARGE, 300, 10, "Score: %d", score);
}

playfield_t field;
playfield_t testfield;
int mustFixBlock;

unsigned int randSeed = 0;

void randAddEnt(int chr)
{
    randSeed += chr;
}

int rand()
{
    randSeed = randSeed * 1103515245 + 12345;
    return randSeed % 65536;
}


void tetris_task()
{
    running = 1;
    int dead = 0;

    fieldClear(&field);
    fieldSelectBlock(&field, 2);

    bmp_fill(COLOR_BLACK, 0, 0, 300, 300);

    while (1)
    {
        /* Wait for input */
        if (!running)
        {
            msleep(1);
            continue;
        }

        fieldDup(&testfield, &field);
        mustFixBlock = 0;

        testfield.currBlockY++;
        if (fieldIsPossible(&testfield))
        {
            fieldDup(&field, &testfield);
        }
        else
        {
            mustFixBlock = 1;
        }

        display(&field);
        msleep(SPEED);

        if (mustFixBlock && dead == 0)
        {
            fieldFixBlock(&field);
            fieldSelectBlock(&field, rand() % 7);
            if (!fieldIsPossible(&field))
            {
                running = 0;
                bmp_printf(FONT_LARGE, 300, 50, "Game Over.");
                return;
            }
        }
    }
}

unsigned int tetris_keypress(unsigned int key)
{
    if (!running)
    {
        return 1;
    }

    randAddEnt(key);

    /* Tell tetris_task to stop loop when
    receiving button input */
    running = 0;

    fieldDup(&testfield, &field);
    mustFixBlock = 0;

    switch(key)
    {
    case MODULE_KEY_Q:
        running = 0;
        return 1;
    case MODULE_KEY_PRESS_LEFT:
        testfield.currBlockX--;
        if (fieldIsPossible(&testfield))
        {
            fieldDup(&field, &testfield);
        }

        break;
    case MODULE_KEY_PRESS_RIGHT:
        testfield.currBlockX++;
        if (fieldIsPossible(&testfield))
        {
            fieldDup(&field, &testfield);
        }

        break;
    case MODULE_KEY_PRESS_UP:
        fieldRotateBlock(&testfield, 1);
        if (fieldIsPossible(&testfield))
        {
            fieldDup(&field, &testfield);
        }

        break;
    case MODULE_KEY_PRESS_DOWN:
        while(fieldIsPossible(&testfield))
        {
            fieldDup(&field, &testfield);
            testfield.currBlockY++;
        }

        mustFixBlock = 1;
        break;
    }

    display(&field);
    running = 1;
    return 0;
}

struct menu_entry tetris_menu[] =
{
    {
        .name = "ML Tetris",
        .select = run_in_separate_task,
        .priv = tetris_task,
        .help = "Tetris on your DSLR",
    },
};

unsigned int tetris_init()
{
    menu_add("Debug", tetris_menu, COUNT(tetris_menu));
    return 0;
}

unsigned int tetris_deinit()
{
    return 0;
}

MODULE_CBRS_START()
MODULE_CBR(CBR_KEYPRESS, tetris_keypress, 0)
MODULE_CBRS_END()

MODULE_INFO_START()
MODULE_INIT(tetris_init)
MODULE_DEINIT(tetris_deinit)
MODULE_INFO_END()
