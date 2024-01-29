#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <math.h>

#define ARENA_WIDTH 400U
#define ARENA_HEIGHT 800U

typedef struct _Tetrominos {
    const uint8_t I[8];
    const uint8_t J[8];
    const uint8_t L[8];
    const uint8_t O[8];
    const uint8_t Z[8];
    const uint8_t S[8];
    const uint8_t T[8];
    const uint8_t size;
} Tetrominos;

#define TETROMINOS_DATA_SIZE 8

enum TET_TYPES {
    TET_I,
    TET_J,
    TET_L,
    TET_O,
    TET_S,
    TET_T,
    TET_Z,
};
Tetrominos tetrominos = {

    .I = {0,0,0,0,
          1,1,1,1},

    .J = {1,0,0,0,
          1,1,1,0},

    .L = {0,0,1,0,
          1,1,1,0},

    .O = {1,1,0,0,
          1,1,0,0},

    .S = {0,1,1,0,
          1,1,0,0},

    .T = {0,1,0,0,
          1,1,1,0},

    .Z = {1,1,0,0,
          0,1,1,0},

    .size = 50
};

uint8_t placed[128]; // 16 x 8

void drawTetromino(SDL_Renderer *renderer, const uint8_t peice[8],
                   SDL_Point position)
{
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!peice[i]) continue;
        uint8_t x = i % 4;
        uint8_t y = floor((float)i / 4);
        SDL_Rect rect = {
            .x = (x + position.x) * tetrominos.size, .y = (y + position.y) * 
                 tetrominos.size,
            .w = tetrominos.size, .h = tetrominos.size
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void addToPlaced(const uint8_t peice[8], SDL_Point position) {

    uint8_t pos = position.y * 8 + position.x;
    for (int i = 0; i < 4; ++i) {
        if (peice[i]) {
            placed[pos + i] = 1;
        }
        if (peice[i + 4]) {
            placed[pos + 8 + i] = 1;
        }
    }
}

void printPlaced() {
    uint8_t i = 0;

    for (int y = 0; y < 16; ++y) {
        printf("%.3d: ", y * 8);
        for (int x = 0; x < 8; ++x) {
            i = (y * 8) + x;
            printf("%d", placed[i]);
        }
        printf("\n");
    }
}

void checkIfFits() {
}

int main(void)
{
    memset(&placed, 0, sizeof(uint8_t) * 128);
    /* placed[14] = 1; */
    /* printPlaced(); */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "could not initialize SDL2\n");
        exit(1);
    }
    SDL_Window *window = SDL_CreateWindow("tetris", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, ARENA_WIDTH, ARENA_HEIGHT,
                     SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, 
                                                SDL_RENDERER_SOFTWARE);
    SDL_Event event;

    bool quit = false;

    bool doOnce = true;
    uint8_t *peice; 
    memcpy(peice, &tetrominos.T, sizeof(uint8_t) * 8);
    SDL_Point point = {.x = 0, .y = 0};

    int i = 0;

    uint8_t fall_speed = 50;
    while(!quit) {
        uint32_t start = SDL_GetTicks();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
        drawTetromino(renderer, peice, point);

        while(SDL_PollEvent(&event)) {

            switch(event.type) {
                case SDL_KEYDOWN: {
                    switch(event.key.keysym.sym) {
                        case SDLK_d: {
                            if (point.x < 5) {
                                point.x++;
                            }
                            break;
                        }
                        case SDLK_a: {
                            if (point.x > 0) {
                                point.x--;
                            }
                            break;
                        }
                        case SDLK_s: {
                            fall_speed = 1;
                            break;
                        }
                        default: {
                            fall_speed = 50;
                        }
                    }
                    break;
                }

                case SDL_QUIT: {
                    quit = true;
                    break;
                }
            }
        }

        if (i % fall_speed == 0) {
            if (point.y + 2 < ARENA_HEIGHT / tetrominos.size)  {
                point.y++;
            } else if (doOnce) {
                printf("add to placed\n\n");
                addToPlaced(peice, point);
                printPlaced();
                doOnce = false;
            }
        }

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        uint8_t delay = elapsed_time;
        if (elapsed_time < 16) {
            delay = 16 - elapsed_time;
            SDL_Delay(delay);
        } 


        SDL_RenderPresent(renderer);
        i++;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
