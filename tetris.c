#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH_PX 400U
#define SCREEN_HEIGHT_PX 800U
#define ARENA_WIDTH 8U
#define ARENA_HEIGHT 16U
#define ARENA_SIZE 128U
#define PIECE_WIDTH 4U
#define PIECE_HEIGHT 4U
#define TETROMINOS_DATA_SIZE 16U
#define PIECE_SIZE_PX 50U
#define TETROMINOS_COUNT 7U

enum {PIECE_I, PIECE_J, PIECE_L, PIECE_O, PIECE_S, PIECE_T, PIECE_Z, 
      PIECE_COUNT};
enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_SIZE};
enum { FLIP_NORMAL, FLIP_LEFT, FLIP_RIGHT, FLIP_UPSIDEDOWN};

typedef struct _Size {
    int w;
    int h;
} Size;

typedef struct _PlacedPeice {
    uint8_t piece;
    uint8_t color;
    SDL_Point position;
} PlacedPeice;

static void update_main(uint64_t frame, SDL_KeyCode key);

static uint8_t placed[ARENA_SIZE]; // 16 x 8
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static uint8_t placed_pieces_count = 0;
static PlacedPeice *placed_pieces = NULL;
static TTF_Font *font = NULL;
static SDL_Texture *texture_lost_text = NULL;
static uint8_t flip = FLIP_NORMAL;
static const char * loose_text = "You Lost";
typedef void (*Update_callback)(uint64_t frame, SDL_KeyCode key);
static Update_callback update = update_main;


static const SDL_Color colors[] = {
    [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
    [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
    [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
    [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
};

const uint8_t tetris_tetrominos[TETROMINOS_COUNT]
                               [PIECE_WIDTH * PIECE_HEIGHT] = {
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

void
tetris_getPeiceSize(uint8_t piece, Size *size)
{
    size->w = 0;
    size->h = 1;

    for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
        for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
            uint8_t i = y * PIECE_WIDTH + x;
            if (tetris_tetrominos[piece][i]) {
                size->w++;
                break;
            }
        }
    }

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t i = y * PIECE_WIDTH + x;
            if (tetris_tetrominos[piece][i]) {
                size->h++;
                break;
            }
        }
    }
}

void tetris_getXY(uint8_t i, int *x, int *y) {
    *x = i % PIECE_WIDTH;
    *y = floor((float)i / PIECE_WIDTH);
}

uint8_t
tetris_drawTetromino(SDL_Renderer *renderer, uint8_t piece, SDL_Point position,
                     uint8_t color)
{
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!tetris_tetrominos[piece][i]) continue;
        int x, y; tetris_getXY(i, &x, &y);
        /* int x = i % PIECE_WIDTH; */
        /* int y = floor((float)i / PIECE_WIDTH); */

        SDL_Rect rect = {
            .x = (x + position.x) * PIECE_SIZE_PX, .y = (y + position.y) *
                 PIECE_SIZE_PX,
            .w = PIECE_SIZE_PX, .h = PIECE_SIZE_PX
        };
        tetris_setColor(color);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    return color;
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
tetris_addToPlaced(uint8_t piece, SDL_Point position, uint8_t color)
{

    uint8_t pos = tetris_getPlacedPosition(position);

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t piece_i = y * PIECE_WIDTH + x;
            uint8_t placed_i = y * ARENA_WIDTH + x;
            if (tetris_tetrominos[piece][piece_i]) {
                placed[pos + placed_i] = 1;
            }
        }
        
    }

    uint8_t i = placed_pieces_count++;
    placed_pieces = realloc(placed_pieces, sizeof(PlacedPeice) *
                           placed_pieces_count);
    memcpy(&placed_pieces[i].position, &position, sizeof(SDL_Point));
    memcpy(&placed_pieces[i].color, &color, sizeof(uint8_t));
    memcpy(&placed_pieces[i].piece, &piece, sizeof(uint8_t));
    tetris_printPlaced();
    printf("\n");
}

bool
tetris_collisionCheck(SDL_Point position, uint8_t piece)
{

    Size size; tetris_getPeiceSize(piece, &size);
    uint8_t placed_pos = tetris_getPlacedPosition(position);

    if (position.x + size.w > ARENA_WIDTH  ||
        position.y + size.h > ARENA_HEIGHT ||
        position.x < 0) {
        return true;
    }

    /* for (uint8_t i = 0; i < PIECE_WIDTH; ++i) { */
    /*     SDL_Point offset = {.x = i + position.x, .y = position.y}; */
    /*     uint8_t top = tetris_getPlacedPosition(offset); */
    /*     uint8_t bottom = top + ARENA_WIDTH; */
    /*     if ((tetris_tetrominos[piece][i] && placed[top]) || */
    /*        (tetris_tetrominos[piece][i + PIECE_WIDTH] && placed[bottom])) { */
    /*         return true; */
    /*     } */
    /* } */
    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            /* SDL_Point offset = {.x = x + position.x, */
            /*                     .y = position.y}; */
            uint8_t piece_i = y * PIECE_WIDTH + x;
            uint8_t placed_i = placed_pos + (y * ARENA_WIDTH + x);
            if (tetris_tetrominos[piece][piece_i] && placed[placed_i]) {
                return true;
            }

        }
    }
    return false;
}

void
tetris_pickPeice(uint8_t *piece, int *color)
{
    *piece = (float)((float)rand() / (float)RAND_MAX) * PIECE_COUNT;
    *color = ((*color) + 1) % COLOR_SIZE;
}

void
tetris_init()
{
    srand(time(NULL));
    memset(&placed, 0, sizeof(uint8_t) * ARENA_SIZE);

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
        .x  = rect.x - padding,
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
        tetris_drawTetromino(renderer, placed_pieces[i].piece,
        placed_pieces[i].position, placed_pieces[i].color);
    }
}

static void
update_loose(uint64_t frame, SDL_KeyCode key)
{
    tetris_drawPlaced();
    tetris_drawLooseText();
}

static void
update_main(uint64_t frame, SDL_KeyCode key)
{
    static SDL_Point piece_position = {.x = 0, .y = -1};
    static uint8_t piece = PIECE_L;
    static uint8_t fall_speed = 50;
    static int color = COLOR_RED;

    if (piece_position.y == -1) {
        Size size;
        tetris_getPeiceSize(piece, &size);
        piece_position.x = (ARENA_WIDTH / 2) - (size.w / 2);
    }

    tetris_drawTetromino(renderer, piece, piece_position, color);

    switch(key) {
        case SDLK_d: {
            SDL_Point check = {.x = piece_position.x + 1,
                               .y = piece_position.y};
            if (!tetris_collisionCheck(check, piece)) {
                piece_position.x++;
            }
            break;
        }
        case SDLK_a: {
            SDL_Point check = {.x = piece_position.x - 1,
                               .y = piece_position.y};

            if (!tetris_collisionCheck(check, piece)) {
                piece_position.x--;
            }
            break;
        }
        case SDLK_s: {
            fall_speed = 1;
            break;
        }
        default: {
            fall_speed = 30;
        }
    }

    if (frame % fall_speed == 0) {
        SDL_Point check = {.x = piece_position.x,
                           .y = piece_position.y + 1};

        if (!tetris_collisionCheck(check, piece))  {
            piece_position.y++;
        } else{
            if (piece_position.y < 1) {
                tetris_addToPlaced(piece, piece_position, color);
                update = update_loose;
            } else {
                tetris_addToPlaced(piece, piece_position, color);
                tetris_pickPeice(&piece, &color);
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
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_Event event;
        uint32_t start = SDL_GetTicks();
        SDL_KeyCode key = 0;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                   key =  event.key.keysym.sym;
                   break;
                case SDL_QUIT: {
                    quit = true;
                    break;
                }
            }
        }

        update(frame, key);

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < 16) {
            elapsed_time = 16 - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

        SDL_RenderPresent(renderer);
        frame++;
    }
}

void
tetris_quit()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(placed_pieces);
    TTF_CloseFont(font);
    SDL_DestroyTexture(texture_lost_text);
}

int
main(void)
{
    tetris_init();
    tetris_update();
    tetris_quit();
    return 0;
}
