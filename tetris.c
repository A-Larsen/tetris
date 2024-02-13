#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>

#define ARENA_WIDTH_PX 400U
#define ARENA_HEIGHT_PX 800U
#define SCREEN_WIDTH_PX 1200U
#define SCREEN_HEIGHT_PX 800U
#define ARENA_PADDING_PX 400U
#define ARENA_WIDTH 8U
#define ARENA_HEIGHT 18U
#define ARENA_SIZE 144U
#define PIECE_WIDTH 4U
#define PIECE_HEIGHT 4U
#define PIECE_SIZE 16U
#define TETROMINOS_DATA_SIZE 16U
#define BLOCK_SIZE_PX 50U
#define TETROMINOS_COUNT 7U
#define PIECE_COLOR_SIZE 4
#define ARENA_PADDING_TOP 2

enum {PIECE_I, PIECE_J, PIECE_L, PIECE_O, PIECE_S, PIECE_T, PIECE_Z,
      PIECE_COUNT};

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_BLACK, COLOR_SIZE};

const uint8_t piece_colors[PIECE_COLOR_SIZE] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_ORANGE,
};
enum {COLLIDE_LEFT = 0b1000, COLLIDE_RIGHT = 0b0100, 
      COLLIDE_TOP = 0b0010, COLLIDE_BOTTOM = 0b0001, COLLIDE_PIECE = 0b1111};

typedef struct _Size {
    int w;
    int h;
    uint8_t start_x;
    uint8_t start_y;
} Size;

static uint8_t level = 0;
static uint16_t score = 0;
static uint8_t placed[ARENA_SIZE]; // 8 x 18
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *loose_font = NULL;
static TTF_Font *ui_font = NULL;
static SDL_Texture *texture_lost_text = NULL;
static const char * loose_text = "You Lost";
typedef uint8_t (*Update_callback)(uint64_t frame, SDL_KeyCode key,
                                   bool keydown);
/* static uint8_t current_piece[PIECE_SIZE]; */
static uint8_t current_piece_color = COLOR_RED;
static int old_repeat_delay;
static int old_repeat_interval;

static const SDL_Color colors[] = {
    [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
    [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
    [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
    [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
    [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
    [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
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

static void 
draw_text(TTF_Font *font, const char *text, SDL_Point point)
{
    int w = 0;
    int h = 0;
    TTF_SizeText(font, loose_text, &w, &h);

    SDL_Color font_color = {.r = 255, .g = 255, .b =255, .a = 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, font_color);

    if (surface == NULL) {
        fprintf(stderr, "Could not create text surface\n%s", SDL_GetError());
    }

    SDL_Rect rect = {
        .x = point.x - (w / 2),
        .y = point.y - (h / 2),
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

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &text_background);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &text_background);


    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

}

int
tetris_findPoints(uint8_t lines)
{
    switch(lines) {
        case 1: return 40 * (level + 1);
        case 2: return 100 * (level + 1);
        case 3: return 300 * (level + 1);
        case 4: return 1200 * (level + 1);
    }

    return 0;
}

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
tetris_rotatePiece(uint8_t *piece, uint8_t *rotated)
{
    memset(rotated, 0, sizeof(uint8_t) * PIECE_SIZE);

    uint8_t i = 0;

    // 90 degrees
    for (int y = 0; y < PIECE_HEIGHT; ++y) {
        for (int x = PIECE_WIDTH - 1; x >= 0; --x) {
            uint8_t j = x * PIECE_WIDTH + y;
            rotated[i] = piece[j];
            ++i;
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
            .x = ((x + position.x) * BLOCK_SIZE_PX) + ARENA_PADDING_PX,
            .y = (y + position.y - ARENA_PADDING_TOP) * BLOCK_SIZE_PX,
            .w = BLOCK_SIZE_PX, .h = BLOCK_SIZE_PX
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
tetris_clearRow(uint8_t c)
{
    uint8_t temp[ARENA_SIZE];
    memset(temp, 0, sizeof(uint8_t) * ARENA_SIZE);
    memcpy(temp, placed, sizeof(uint8_t) * ARENA_SIZE);

    for (int y = c; y > 0; --y) {
        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            uint8_t i = y * ARENA_WIDTH + x;
            if (i >= 0) {
                temp[i] = placed[i - ARENA_WIDTH];

            }
        }
    }

    memcpy(placed, temp, sizeof(uint8_t) * ARENA_SIZE);
}

uint8_t
tetris_checkForRowClearing()
{
    uint8_t row_count = 0;
    uint8_t lines = 0;
    for (uint8_t y = 0; y < ARENA_HEIGHT; ++y) {

        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            if (x == 0) row_count = 0;

            uint8_t i = y * ARENA_WIDTH + x;
            if (placed[i]) row_count++;
            if ((x == ARENA_WIDTH - 1) && (row_count == ARENA_WIDTH)) {
                tetris_clearRow(y);
                lines++;
            }

        }
    }
    return lines;
}

void
tetris_addToPlaced(uint8_t *piece, SDL_Point position)
{

    uint8_t pos = tetris_getPlacedPosition(position);

    for (uint8_t i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        int x, y; tetris_getXY(i, &x, &y);
        uint8_t piece_i = y * PIECE_WIDTH + x;
        uint8_t placed_i = y * ARENA_WIDTH + x;
        if (piece[piece_i])
            tetris_addToArena(pos + placed_i);
    }
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
tetris_pickPeice(uint8_t *piece)
{
    uint8_t id = (float)((float)rand() / (float)RAND_MAX) * PIECE_COUNT;

    memcpy(piece, &tetris_tetrominos[id], sizeof(uint8_t) *
           PIECE_SIZE);
    current_piece_color = piece_colors[((current_piece_color) + 1) % 
                          PIECE_COLOR_SIZE];
}

void
tetris_init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "could not initialize SDL2\n%s", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "could not initialize TTF\n%s", SDL_GetError());
        exit(1);
    }
    loose_font = TTF_OpenFont("./fonts/NotoSansMono-Regular.ttf", 50);

    if (loose_font == NULL) {
        fprintf(stderr, "could not open font %s\n", SDL_GetError());
    }

    ui_font = TTF_OpenFont("./fonts/NotoSansMono-Regular.ttf", 30);

    if (ui_font == NULL) {
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

void
tetris_drawPlaced() {
    for (int x = 0; x < ARENA_WIDTH; ++x) {
        for (int y = 0; y < ARENA_HEIGHT; ++y) {
            uint8_t i = y * ARENA_WIDTH + x;
            if (!placed[i]) continue;
            SDL_Rect rect = {
                .x = (x * BLOCK_SIZE_PX) + ARENA_PADDING_PX,
                .y = (y - ARENA_PADDING_TOP) * BLOCK_SIZE_PX,
                .w = BLOCK_SIZE_PX, .h = BLOCK_SIZE_PX
            };
            tetris_setColor(COLOR_GREY);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
            
        }
        
    }
}

// id of 1
static uint8_t
update_loose(uint64_t frame, SDL_KeyCode key, bool keydown)
{
    SDL_Point point = {.x = ARENA_WIDTH_PX / 2 + ARENA_PADDING_PX,
                       .y = ARENA_HEIGHT_PX / 2};

    draw_text(loose_font, loose_text, point);
    return 1;
}

// id of 0
static uint8_t
update_main(uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static SDL_Point piece_position = {.x = 0, .y = -1};
    static uint8_t fall_speed = 30;
    static uint8_t current_piece[PIECE_SIZE];
    static bool init = true;

    if (init) {
        srand(time(NULL));
        memset(&placed, 0, sizeof(uint8_t) * ARENA_SIZE);
        tetris_pickPeice(current_piece);
        init = false;
    }

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
            tetris_rotatePiece(current_piece, rotated);
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
            Size size;
            tetris_getPieceSize(current_piece, &size);
            if (piece_position.y - size.h < 0) {
                Size size;
                tetris_getPieceSize(current_piece, &size);
                tetris_addToPlaced(current_piece, piece_position);
                return 1;
            } else {
                fall_speed = 30;
                tetris_addToPlaced(current_piece, piece_position);
                tetris_pickPeice(current_piece);
                piece_position.y = -1;
            }
        } 
    }

    uint8_t lines = tetris_checkForRowClearing();
    score += tetris_findPoints(lines);
    char score_string[255];
    sprintf(score_string, "score %d", score);
    SDL_Point point = {.x = ARENA_PADDING_PX / 2, .y = 50};
    draw_text(ui_font, score_string, point);
    return 0;
}

void
tetris_update(const uint8_t fps)
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update = update_main;

    while (!quit) {

        switch(update_id) {
            case 0: update = update_main; break;
            case 1: update = update_loose; break;
        }

        tetris_setColor(COLOR_GREY);
        SDL_RenderClear(renderer);
        tetris_setColor(COLOR_BLACK);

        SDL_Rect arena_background_rect = {
            .x = ARENA_PADDING_PX,
            .y = 0,
            .w = ARENA_WIDTH_PX,
            .h = ARENA_HEIGHT_PX
        };

        SDL_RenderFillRect(renderer, &arena_background_rect);

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

        tetris_drawPlaced();
        update_id = update(frame, key, keydown);


        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        float mspd = (1.0f / (float)fps) * 1000.0f;

        if (elapsed_time < mspd) {
            elapsed_time = mspd - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

        SDL_RenderPresent(renderer);
        frame++;
    }
}

void
tetris_quit()
{
    TTF_CloseFont(loose_font);
    TTF_CloseFont(ui_font);
    SDL_DestroyTexture(texture_lost_text);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int
main(void)
{
    tetris_init();
    tetris_update(60);
    tetris_quit();
    return 0;
}
