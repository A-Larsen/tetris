#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <math.h>

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
#define PEICE_SIZE 50

static uint8_t placed[ARENA_SIZE]; // 16 x 8
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Event event;
static bool quit = false;

typedef struct _Size {
    int w;
    int h;
} Size;

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

void getPeiceSize(uint8_t peice, Size *size) {
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

void tetris_addToPlaced(uint8_t peice, SDL_Point position) {

    uint8_t pos = GET_PLACED_POSITION(position);
    for (int i = 0; i < PEICE_WIDTH; ++i) {
        if (tetris_tetrominos[peice][i]) {
            placed[pos + i] = 1;
        }
        if (tetris_tetrominos[peice][i + PEICE_WIDTH]) {
            placed[pos + ARENA_WIDTH + i] = 1;
        }
    }
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

    Size size; getPeiceSize(peice, &size);
    uint8_t i = GET_PLACED_POSITION(position);

    if (position.x + size.w > ARENA_WIDTH  ||
        position.y + size.h > ARENA_HEIGHT ||
        position.x < 0) {
        return true;
    }

    for (uint8_t i = position.x; i < position.x + PEICE_WIDTH; ++i) {
        SDL_Point offset = {.x = i, .y = position.y};
        uint8_t i = GET_PLACED_POSITION(offset);
        return placed[i] || placed[i + ARENA_WIDTH];

    }

    return false;
}

void tetris_init() {
    memset(&placed, 0, sizeof(uint8_t) * ARENA_SIZE);
    for (int i = 120; i < 128; ++i) {
        placed[i] = 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "could not initialize SDL2\n%s", SDL_GetError());
        exit(1);
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
    static uint8_t peice = PEICE_T;
    static bool doOnce = true;
    static SDL_Point point = {.x = 0, .y = 0};

    static uint8_t fall_speed = 50;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
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
    /* bool pass = tetris_collisionCheck(point, peice); */
    /* printf("%d\n", pass); */

    if (frame % fall_speed == 0) {
        SDL_Point check = {.x = point.x, .y = point.y + 1};

        if (!tetris_collisionCheck(check, peice))  {
            point.y++;
        } else{
            printf("add to placed\n\n");
            tetris_addToPlaced(peice, point);
            tetris_printPlaced();
        } 
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
}

int main(void)
{
    tetris_init();
    tetris_update(tetris_callback);
    tetris_quit();
    return 0;
}
