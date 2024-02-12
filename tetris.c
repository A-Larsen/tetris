#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH_PX 400U
#define SCREEN_HEIGHT_PX 800U
#define ARENA_WIDTH 8U
#define ARENA_HEIGHT 18U
#define ARENA_SIZE 144U
#define PIECE_WIDTH 4U
#define PIECE_HEIGHT 4U
#define PIECE_SIZE 16U
#define TETROMINOS_DATA_SIZE 16U
#define PIECE_SIZE_PX 50U
#define TETROMINOS_COUNT 7U
#define XKBDDELAY_DEFAULT 660
#define XKBDRATE_DEFAULT (1000/40)
#define FPS 60.0f
#define MSPD (1.0f / FPS) * 1000.0f

enum {PIECE_I, PIECE_J, PIECE_L, PIECE_O, PIECE_S, PIECE_T, PIECE_Z, 
      PIECE_COUNT};
enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_SIZE};
enum {FLIP_NORMAL, FLIP_LEFT, FLIP_RIGHT, FLIP_UPSIDEDOWN};
enum {COLLIDE_LEFT = 0b1000, COLLIDE_RIGHT = 0b0100, 
      COLLIDE_TOP = 0b0010, COLLIDE_BOTTOM = 0b0001, COLLIDE_PIECE = 0b1111};

typedef struct _Size {
    int w;
    int h;
    uint8_t start_x;
    uint8_t start_y;
} Size;

typedef struct _PlacedPeice {
    uint8_t piece[PIECE_SIZE];
    uint8_t color;
    SDL_Point position;
} PlacedPeice;

static void update_main(uint64_t frame, SDL_KeyCode key, bool keydown);

static uint8_t placed[ARENA_SIZE]; // 8 x 18
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static uint8_t placed_pieces_count = 0;
static PlacedPeice *placed_pieces = NULL;
static TTF_Font *font = NULL;
static SDL_Texture *texture_lost_text = NULL;
static const char * loose_text = "You Lost";
typedef void (*Update_callback)(uint64_t frame, SDL_KeyCode key, bool keydown);
static uint8_t current_piece[PIECE_SIZE];
static uint8_t current_piece_color = COLOR_RED;
static Update_callback update = update_main;
static int old_repeat_delay;
static int old_repeat_interval;

static const SDL_Color colors[] = {
    [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
    [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
    [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
    [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
};

const uint8_t tetris_tetrominos[TETROMINOS_COUNT]
                               [PIECE_SIZE] = {
    // each peice has 4 ones
    [PIECE_I] = {0,0,0,0,
                 1,1,1,1,
                 0,0,0,0,
                 0,0,0,0},

    [PIECE_J] = {0,0,0,0,
                 1,0,0,0,
                 1,1,1,0,
                 0,0,0,0},

    [PIECE_L] = {0,0,0,0,
                 0,0,1,0,
                 1,1,1,0,
                 0,0,0,0},

    [PIECE_O] = {0,0,0,0,
                 0,1,1,0,
                 0,1,1,0,
                 0,0,0,0},

    [PIECE_S] = {0,0,0,0,
                 0,1,1,0,
                 1,1,0,0,
                 0,0,0,0},

    [PIECE_T] = {0,0,0,0,
                 0,1,0,0,
                 1,1,1,0,
                 0,0,0,0},

    [PIECE_Z] = {0,0,0,0,
                 1,1,0,0,
                 0,1,1,0,
                 0,0,0,0},
};

void
tetris_addToArena(uint8_t i)
{
    // bounds checking
    if (i < ARENA_SIZE && i >= 0) placed[i] = 1;
}

uint8_t
tetris_getPlacedPosition(SDL_Point pos)
{
    uint8_t i = pos.y * ARENA_WIDTH + pos.x;
    return i < ARENA_SIZE ? i : ARENA_SIZE - 1;
}

void
tetris_setColor(uint8_t color)
{
    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

void tetris_getXY(uint8_t i, int *x, int *y) {
    *x = i % PIECE_WIDTH;
    *y = floor((float)i / PIECE_HEIGHT);
}

void
tetris_getPieceSize(uint8_t *piece, Size *size)
{
    memset(size, 0, sizeof(Size));

    bool foundStartx = false;
    bool foundStarty = false;

    for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
        for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
            uint8_t i = y * PIECE_WIDTH + x;
            if (piece[i]) {
                if (!foundStartx) {
                    size->start_x = x;
                    foundStartx = true;
                }
                size->w++;
                break;
            }
        }
    }

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t i = y * PIECE_WIDTH + x;
            if (piece[i]) {
                if (!foundStarty) {
                    size->start_y = y;
                    foundStarty = true;
                }
                size->h++;
                break;
            }
        }
    }
}


void
tetris_rotatePiece(uint8_t rotated[PIECE_SIZE], uint8_t flip)
{
    memset(rotated, 0, sizeof(uint8_t) * PIECE_SIZE);

    for (uint8_t i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        int x, y; tetris_getXY(i, &x, &y);

        switch(flip) {
            case FLIP_LEFT: {
                uint8_t j = x * PIECE_WIDTH + y;
                rotated[i] = current_piece[j];
            }
        }
    }
}

void
tetris_drawTetromino(SDL_Renderer *renderer, uint8_t piece[PIECE_SIZE],
                     SDL_Point position, uint8_t color)
{
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!piece[i]) continue;
        int x, y; tetris_getXY(i, &x, &y);

        SDL_Rect rect = {
            .x = (x + position.x) * PIECE_SIZE_PX,
            .y = (y + position.y - 2) *
                 PIECE_SIZE_PX,

            .w = PIECE_SIZE_PX, .h = PIECE_SIZE_PX
        };
        tetris_setColor(color);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void
tetris_printPlaced()
{
    uint8_t i = 0;

    for (int y = 0; y < ARENA_HEIGHT; ++y) {
        printf("%.3d: ", y * ARENA_WIDTH);
        for (int x = 0; x < ARENA_WIDTH; ++x) {
            i = (y * ARENA_WIDTH) + x;
            printf("%d", placed[i]);
        }
        printf("\n");
    }
}

void
tetris_addToPlaced(SDL_Point position)
{

    uint8_t pos = tetris_getPlacedPosition(position);

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t piece_i = y * PIECE_WIDTH + x;
            uint8_t placed_i = y * ARENA_WIDTH + x;
            if (current_piece[piece_i])
                tetris_addToArena(pos + placed_i);
        }
    }

    uint8_t i = placed_pieces_count++;
    placed_pieces = realloc(placed_pieces, sizeof(PlacedPeice) *
                           placed_pieces_count);
    memcpy(&placed_pieces[i].position, &position, sizeof(SDL_Point));
    memcpy(&placed_pieces[i].color, &current_piece_color, sizeof(uint8_t));
    memcpy(&placed_pieces[i].piece, &current_piece, sizeof(uint8_t) *
                                    PIECE_SIZE);
    tetris_printPlaced();
    printf("\n");
}

uint8_t
tetris_collisionCheck(uint8_t *piece, SDL_Point position)
{
    Size size; tetris_getPieceSize(piece, &size);
    uint8_t placed_pos = tetris_getPlacedPosition(position);

    if (position.x < -size.start_x) {
        return COLLIDE_LEFT;
    }

    if (position.x + size.start_x + size.w > ARENA_WIDTH) {
        return COLLIDE_RIGHT;
    }

    if (position.y + size.start_y + size.h > ARENA_HEIGHT) {
        return COLLIDE_BOTTOM;
    }

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t piece_i = y * PIECE_WIDTH + x;
            uint8_t placed_i = placed_pos + (y * ARENA_WIDTH + x);
            if (piece[piece_i] && placed[placed_i]) 
                return COLLIDE_PIECE;
        }
    }

    return 0;
}

void
tetris_pickPeice()
{
    uint8_t piece = (float)((float)rand() / (float)RAND_MAX) * PIECE_COUNT;
    memcpy(&current_piece, &tetris_tetrominos[piece], sizeof(uint8_t) *
           PIECE_SIZE);
    current_piece_color = ((current_piece_color) + 1) % COLOR_SIZE;
}

void
tetris_init()
{
    srand(time(NULL));
    memset(&placed, 0, sizeof(uint8_t) * ARENA_SIZE);
    tetris_pickPeice();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "could not initialize SDL2\n%s", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "could not initialize TTF\n%s", SDL_GetError());
        exit(1);
    }
    font = TTF_OpenFont("./fonts/NotoSansMono-Regular.ttf", 50);

    if (font == NULL) {
        fprintf(stderr, "could not open font %s\n", SDL_GetError());
    }


    window = SDL_CreateWindow("tetris", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH_PX, 
                     SCREEN_HEIGHT_PX, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "could not create window\n%s", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);

    if (renderer == NULL) {
        fprintf(stderr, "could not create renderer\n%s", SDL_GetError());
        exit(1);
    }

    SDL_Color font_color = {.r = 255, .g = 255, .b =255, .a = 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, loose_text, font_color);

    if (surface == NULL) {
        fprintf(stderr, "Could not create text surface\n%s", SDL_GetError());
    }
    texture_lost_text = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);
}

void
tetris_drawLooseText()
{
    int w = 0;
    int h = 0;
    TTF_SizeText(font, loose_text, &w, &h);

    SDL_Rect rect = {
        .x = (SCREEN_WIDTH_PX / 2) - (w / 2),
        .y = (SCREEN_HEIGHT_PX / 2) - (h / 2),
        .w = w,
        .h = h,
    };
    int padding = 8;
    SDL_Rect text_background = {
        .x = rect.x - padding,
        .y = rect.y - padding,
        .w = rect.w + padding * 2,
        .h = rect.h + padding * 2
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &text_background);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &text_background);
    SDL_RenderCopy(renderer, texture_lost_text, NULL, &rect);
}

void
tetris_drawPlaced() {
    for (uint8_t i = 0; i < placed_pieces_count; ++i) {
        /* placed_pieces[i].position.y += (ARENA_WIDTH * 2); */
        SDL_Point pos = {
            .x = placed_pieces[i].position.x,
            .y = placed_pieces[i].position.y

        };
        tetris_drawTetromino(renderer, placed_pieces[i].piece,
        pos, placed_pieces[i].color);
    }
}

static void
update_loose(uint64_t frame, SDL_KeyCode key, bool keydown)
{
    tetris_drawPlaced();
    tetris_drawLooseText();
}

static void
update_main(uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static SDL_Point piece_position = {.x = 0, .y = -1};
    static uint8_t fall_speed = 30;

    if (piece_position.y == -1) {
        Size size;
        tetris_getPieceSize(current_piece, &size);
        piece_position.x = (ARENA_WIDTH / 2) - (size.w / 2);
    }

    tetris_drawTetromino(renderer, current_piece, piece_position,
            current_piece_color);
    if (!keydown) fall_speed = 30;

    switch(key) {
        case SDLK_d: {
            SDL_Point check = {.x = piece_position.x + 1,
                               .y = piece_position.y};
            if (!tetris_collisionCheck(current_piece, check)) {
                piece_position.x++;
            }
            break;
        }
        case SDLK_a: {
            SDL_Point check = {.x = piece_position.x - 1,
                               .y = piece_position.y};

            if (!tetris_collisionCheck(current_piece, check)) {
                piece_position.x--;
            }
            break;
        }
        case SDLK_s: {
            fall_speed = 1;
            break;
        }
        case SDLK_r: {
            uint8_t rotated[PIECE_SIZE];
            tetris_rotatePiece(rotated, FLIP_LEFT);
            uint8_t collide = tetris_collisionCheck(rotated, piece_position);
            bool canRotate = true;
            switch(collide) {
                case COLLIDE_LEFT: {
                    while(true) {
                        piece_position.x++;
                        if (!tetris_collisionCheck(rotated, piece_position))
                            break;
                    }
                    break;
                }
                case COLLIDE_RIGHT: {
                    while(true) {
                        piece_position.x--;
                        if (!tetris_collisionCheck(rotated, piece_position))
                            break;
                    }
                    break;
                }
                case COLLIDE_BOTTOM:
                case COLLIDE_PIECE: {
                    canRotate = false;
                    break;
                }
            }
            if (canRotate)
                memcpy(current_piece, rotated, sizeof(uint8_t) * PIECE_SIZE);
            break;
        }
    }

    if (frame % fall_speed == 0) {
        SDL_Point check = {.x = piece_position.x,
                           .y = piece_position.y + 1};

        if (!tetris_collisionCheck(current_piece, check))  {
            piece_position.y++;
        } else{
            // TODO:
            // fix the case when the pieces height is greater than
            // the screen space available
            if (piece_position.y <= 0) {
                Size size;
                tetris_getPieceSize(current_piece, &size);
                /* if (size.h > 1) piece_position.y--; */
                tetris_addToPlaced(piece_position);
                update = update_loose;
            } else {
                fall_speed = 30;
                tetris_addToPlaced(piece_position);
                tetris_pickPeice();
                piece_position.y = -1;
            }
        } 
    }

    tetris_drawPlaced();
}

void
tetris_update()
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;

    while (!quit) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_Event event;
        uint32_t start = SDL_GetTicks();
        SDL_KeyCode key = 0;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.repeat == 0) {
                      key = event.key.keysym.sym;
                      keydown = true;
                    }
                    break;
                }
                
                case SDL_KEYUP: keydown = false; break;
                   
                case SDL_QUIT: quit = true; break;
               
            }
        }
        update(frame, key, keydown);


        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < MSPD) {
            elapsed_time = MSPD - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

        SDL_RenderPresent(renderer);
        frame++;
    }
}

void
tetris_quit()
{
    TTF_CloseFont(font);
    SDL_DestroyTexture(texture_lost_text);
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(placed_pieces);
}

int
main(void)
{
    tetris_init();
    tetris_update();
    tetris_quit();
    return 0;
}
