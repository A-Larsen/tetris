#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH_PX 400U
#define SCREEN_HEIGHT_PX 800U
#define ARENA_WIDTH 8
#define ARENA_HEIGHT 16
#define ARENA_SIZE 128
#define PEICE_WIDTH 4
#define PEICE_HEIGHT 2
#define TETROMINOS_DATA_SIZE 8
#define GET_PLACED_POSITION(pos) pos.y * ARENA_WIDTH + pos.x;
#define PEICE_I 0
#define PEICE_J 1
#define PEICE_L 2
#define PEICE_O 3
#define PEICE_S 4
#define PEICE_T 5
#define PEICE_Z 6
#define PEICE_COUNT 7
#define PEICE_SIZE 50
#define COLOR_RED 0
#define COLOR_GREEN 1
#define COLOR_BLUE 2
#define COLOR_SIZE 3

typedef struct _Size {
    int w;
    int h;
} Size;

typedef struct _PlacedPeice {
    uint8_t peice;
    uint8_t color;
    SDL_Point position;
} PlacedPeice;

static uint8_t placed[ARENA_SIZE]; // 16 x 8
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Event event;
static bool quit = false;
static uint8_t placedPeicesCount = 0;
static PlacedPeice *placedPeices = NULL;
static TTF_Font *font = NULL;

static SDL_Color colors[] = {
    [COLOR_RED] = {.r = 255, .g = 0, .b = 0, .a = 255},
    [COLOR_GREEN] = {.r = 0, .g = 255, .b = 0, .a = 255},
    [COLOR_BLUE] = {.r = 0, .g = 0, .b = 255, .a = 255},
};

// can adventually figure out the mazimum number of peices that could be
// placed

uint8_t tetris_tetrominos[7][8] = {
    [PEICE_I] = {0,0,0,0,
                 1,1,1,1},

    [PEICE_J] = {1,0,0,0,
                 1,1,1,0},

    [PEICE_L] = {0,0,1,0,
                 1,1,1,0},

    [PEICE_O] = {1,1,0,0,
                 1,1,0,0},

    [PEICE_S] = {0,1,1,0,
                 1,1,0,0},

    [PEICE_T] = {0,1,0,0,
                 1,1,1,0},

    [PEICE_Z] = {1,1,0,0,
                 0,1,1,0},
};

void tetris_setColor(uint8_t color) {
    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

void tetris_getPeiceSize(uint8_t peice, Size *size) {
    size->w = 0;
    size->h = 1;

    for (uint8_t i = 0; i < PEICE_WIDTH; ++i) {
        bool top = tetris_tetrominos[peice][i];
        bool bottom = tetris_tetrominos[peice][i + PEICE_WIDTH];

        if (top && bottom) {
            size->h = 2;
        }

        if (top || bottom) {
            size->w++;
        }
    }
}

void tetris_drawTetromino(SDL_Renderer *renderer, uint8_t peice,
                   SDL_Point position)
{
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!tetris_tetrominos[peice][i]) continue;
        uint8_t x = i % PEICE_WIDTH;
        uint8_t y = floor((float)i / PEICE_WIDTH);
        SDL_Rect rect = {
            .x = (x + position.x) * PEICE_SIZE, .y = (y + position.y) *
                 PEICE_SIZE,
            .w = PEICE_SIZE, .h = PEICE_SIZE
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void tetris_addToPlaced(uint8_t peice, SDL_Point position, uint8_t color) {

    uint8_t pos = GET_PLACED_POSITION(position);

    for (int i = 0; i < PEICE_WIDTH; ++i) {
        if (tetris_tetrominos[peice][i]) {
            placed[pos + i] = 1;
        }
        if (tetris_tetrominos[peice][i + PEICE_WIDTH]) {
            placed[pos + ARENA_WIDTH + i] = 1;
        }
    }

    uint8_t i = placedPeicesCount++;
    placedPeices = realloc(placedPeices, sizeof(PlacedPeice) *
                           placedPeicesCount);
    memcpy(&placedPeices[i].position, &position, sizeof(SDL_Point));
    memcpy(&placedPeices[i].color, &color, sizeof(uint8_t));
    memcpy(&placedPeices[i].peice, &peice, sizeof(uint8_t));
}

void tetris_printPlaced() {
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

bool tetris_collisionCheck(SDL_Point position, uint8_t peice) {

    Size size; tetris_getPeiceSize(peice, &size);
    uint8_t i = GET_PLACED_POSITION(position);

    if (position.x + size.w > ARENA_WIDTH  ||
        position.y + size.h > ARENA_HEIGHT ||
        position.x < 0) {
        return true;
    }

    for (uint8_t i = 0; i < PEICE_WIDTH; ++i) {
        SDL_Point offset = {.x = i + position.x, .y = position.y};
        uint8_t top = GET_PLACED_POSITION(offset);
        uint8_t bottom = top + ARENA_WIDTH;
        if ((tetris_tetrominos[peice][i] && placed[top]) ||
           (tetris_tetrominos[peice][i + PEICE_WIDTH] && placed[bottom])) {
            return true;
        }
    }

    return false;
}
void tetris_pickPeice(uint8_t *peice, int *color) {
    *peice = (float)((float)rand() / (float)RAND_MAX) * PEICE_COUNT;
    *color = ((*color) + 1) % COLOR_SIZE;
}

void tetris_init() {
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
    font = TTF_OpenFont("./fonts/NotoSansMono-Regular.ttf", 8);
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
}


int tetris_getInput() {
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                return event.key.keysym.sym;
            case SDL_QUIT: {
                quit = true;
                break;
            }
        }
    }
    return 0;
}

void tetris_callback(uint64_t frame) {

    static SDL_Point point = {.x = 0, .y = 0};
    static int color = COLOR_RED;
    static uint8_t peice = PEICE_L;

    if (point.y == 0) {
        Size size;
        tetris_getPeiceSize(peice, &size);
        point.x = (ARENA_WIDTH / 2) - (size.w / 2);
    }

    static uint8_t fall_speed = 50;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    tetris_setColor(color);
    tetris_drawTetromino(renderer, peice, point);

    switch(tetris_getInput()) {
        case SDLK_d: {
            SDL_Point check = {.x = point.x + 1, .y = point.y};
            if (!tetris_collisionCheck(check, peice)) {
                point.x++;
            }
            break;
        }
        case SDLK_a: {
            SDL_Point check = {.x = point.x - 1, .y = point.y};
            if (!tetris_collisionCheck(check, peice)) {
                point.x--;
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
        SDL_Point check = {.x = point.x, .y = point.y + 1};

        if (!tetris_collisionCheck(check, peice))  {
            point.y++;
        } else{
            tetris_addToPlaced(peice, point, color);
            tetris_printPlaced();
            tetris_pickPeice(&peice, &color);
            point.y = 0;
        } 
    }

    for (uint8_t i = 0; i < placedPeicesCount; ++i) {
        tetris_setColor(placedPeices[i].color);
        tetris_drawTetromino(renderer, placedPeices[i].peice,
        placedPeices[i].position);
    }
}

void tetris_update(void (*callback)(uint64_t frame)) {
    uint64_t frame = 0;
    while (!quit) {
        uint32_t start = SDL_GetTicks();

        callback(frame);

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

void tetris_quit() {
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(placedPeices);
}

int main(void)
{
    tetris_init();
    tetris_update(tetris_callback);
    tetris_quit();
    return 0;
}
