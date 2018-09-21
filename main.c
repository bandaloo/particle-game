#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720
#define SPRING_SCALAR 1700
#define PI 3.14159265
#define SIM_FPS 300
#define TIME_PER_STEP (double) (1000.0 / SIM_FPS)
#define MAX_STEPS 10
#define NUM_MASSES 20
#define WATER_DEPTH 500
#define WORD_INTERVAL 20
#define WORD_AMOUNT 15

#define T_TEAR 0
#define T_WATER 1
#define T_LETTER 2
#define T_BUBBLE 3

int bgcolor[] = {30, 30, 30};

int clickx, clicky;
int helddown;
int gameover = 0;
int onstep, ondraw;
long unsigned int timediff, currtime, prevtime = 0;
double extratime;
double deltatime;
double leftovertime = 0;
double prevmody, nextmody;
struct node * head;
struct node * tail;
SDL_Window * window;
SDL_Renderer * renderer;

struct sprite * sadsprite, * alphabetsprite, * tearsprite, * bubblesprite;

int rswidth, rsheight, sr;
double waterdepth = WATER_DEPTH;

char * messages[WORD_AMOUNT] = {
    "hello world!",
    "keep going",
    "how are you?",
    "you are doing great",
    "the water is rising...",
    "one hundred point bonus!",
    "...let it out...",
    "youre a natural",
    "feel better?",
    "i hope so regardless",
    "?",
    "closing in",
    "are you having fun?",
    "...",
    "made by cole"
};

int messagecounter = 0;

struct particle {
    struct sprite * sprite;
    void (* step)(struct particle *);
    double x;
    double y;
    double angle;
    double size;
    double velx;
    double vely;
    double accx;
    double accy;
    double drag;
    double lifetime;
    int alpha;
    int r;
    int g;
    int b;
    int type;
    int clip;
};

struct node {
    struct node * next;
    struct particle * particle;
};

struct sprite {
    SDL_Texture ** images;
    int width;
    int height;
    int counter;
};

struct particle springmasses[NUM_MASSES];

double randdouble() {
    return (double) rand() / RAND_MAX;
}

double doublemod(double a, double b) {
    return a - ((int) (a / b)) * b;
}

int getwaterindex(double x) {
    double pos = x + SCREEN_WIDTH / NUM_MASSES / 2;
    return (int) (pos / (SCREEN_WIDTH / (NUM_MASSES - 1)));
}

// TODO figure out if particles are getting destroyed fast enough, not pushing water twice

int isunderwater(double x, double y) {
    int index = getwaterindex(x);
    return index >= 0 && index <= NUM_MASSES - 1 && springmasses[index].y < y;
}

void stepalphabet(struct particle * particle) {
    if (ondraw) {
        particle->alpha = (double) (particle->lifetime / 10000.0 * 255);
    }
}

void setspringmasses() { 
    for (int i = 0; i < NUM_MASSES; i++) {
        springmasses[i] = (struct particle) {
            .sprite = tearsprite,
            .step = NULL,
            .x = SCREEN_WIDTH / (NUM_MASSES - 1) * i,
            .y = WATER_DEPTH,
            .angle = 0,
            .size = 1.0,
            .velx = 0,
            .vely = 0,
            .accx = 0,
            .accy = 0,
            .drag = 0.005,
            .lifetime = 1000000,
            .alpha = 200,
            .r = 255,
            .g = 0,
            .b = 0,
            .type = T_WATER,
            .clip = 0
        };
    }
}

void updatespringaccs() {
    for (int i = 1; i < NUM_MASSES - 1; i++) {
        double yleft = springmasses[i - 1].y;
        double ymiddle = springmasses[i].y;
        double yright = springmasses[i + 1].y;
        springmasses[i].accy = ((yleft - ymiddle) + (yright - ymiddle)) / SPRING_SCALAR;
    }
}

void addparticle(struct particle * particle) {
    struct node * newnode = malloc(sizeof(struct node));
    newnode->particle = particle;
    newnode->next = head->next;
    head->next = newnode;
}

void addspringparticles() {
    for (int i = 0; i < NUM_MASSES; i++) {
        addparticle(springmasses + i);
    }
}


struct particle * makeletterparticle(double x, double y, char c) {
    int clip = c - 'a';
    struct particle * particle = malloc(sizeof(struct particle));
    *particle = (struct particle) {
        .sprite = alphabetsprite,
        .step = stepalphabet,
        .x = x,
        .y = y,
        .angle = 0,
        .velx = randdouble() / 50,
        .vely = randdouble() / 50,
        .accx = 0,
        .accy = 0,
        .drag = 0.001,
        .lifetime = 10000,
        .alpha = 200,
        .r = 0,
        .g = 255,
        .b = 0,
        .type = T_LETTER,
        .clip = clip
    };
    return particle;
}

void drawword(double x, double y, char * str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == '?') c = 'a' + 28;
        else if (c == '!') c = 'a' + 27;
        else if (c == '.') c = 'a' + 26;
        if (c != ' ') {
            addparticle(makeletterparticle(x + i * 20, y, c));
        }
    }
}

void raisewater() {
    springmasses[0].y -= 0.01;
    // TODO moving the water level should probably be in its own function
    double prevmod = doublemod(springmasses[NUM_MASSES - 1].y, WORD_INTERVAL);
    springmasses[NUM_MASSES - 1].y -= 0.01;
    double nextmod = doublemod(springmasses[NUM_MASSES - 1].y, WORD_INTERVAL);
    if (nextmod > prevmod) {
        if (!gameover)
            drawword(clickx - 100, clicky - 50, messages[messagecounter]);
        if (messagecounter < WORD_AMOUNT - 1)
            messagecounter++;
        else
            gameover = 1;
    }
}

void steptear(struct particle * particle) {
    if (ondraw) {
        particle->angle = atan2(particle->vely, particle->velx);
    }
    if (onstep) {
        int index = getwaterindex(particle->x);
        if (isunderwater(particle->x, particle->y)) {
            particle->lifetime = -1;
            if (index != 0 && index != NUM_MASSES - 1)
                springmasses[index].vely += 0.05;
            raisewater();
        }
    }
}

struct particle * maketearparticle(double x, double y) {
    struct particle * particle = malloc(sizeof(struct particle));
    double randdir = randdouble() * 2 * PI;
    double randspeed = randdouble();
    *particle = (struct particle) {
        .sprite = tearsprite,
        .step = steptear,
        .x = x,
        .y = y,
        .angle = 0,
        .size = 1.0,
        .velx = cos(randdir) * randspeed,
        .vely = sin(randdir) * randspeed,
        .accx = 0,
        .accy = 0.01,
        .drag = 0.005,
        .lifetime = 1000 * randdouble() * 1000,
        .alpha = 100,
        .r = 91,
        .g = 187,
        .b = 255,
        .type = T_TEAR,
        .clip = 0
    };
    return particle;
}

struct particle * makebubbleparticle(double x, double y) {
    struct particle * particle = malloc(sizeof(struct particle));
    double randdir = randdouble() * 2 * PI;
    double randspeed = randdouble();
    *particle = (struct particle) {
        .sprite = bubblesprite,
        .step = NULL,
        .x = x,
        .y = y,
        .angle = 0,
        .size = 1.0,
        .velx = cos(randdir) * randspeed * 4,
        .vely = sin(randdir) * randspeed * 2,
        .accx = 0,
        .accy = -0.01,
        .drag = 0.05,
        .lifetime = 300,
        .alpha = 100,
        .r = 255,
        .g = 255,
        .b = 255,
        .type = T_BUBBLE,
        .clip = 0
    };
    return particle;
}

void drawimage(double x, double y, SDL_Texture * image, int imgwidth, int imgheight) {
    SDL_Rect trect = {(x - imgwidth / 4) * sr, (y - imgheight / 4) * sr, imgwidth / 2 * sr, imgheight / 2 * sr};
    SDL_RenderCopy(renderer, image, NULL, &trect);
}

void setcolors(struct particle * particle) {
    SDL_SetTextureColorMod(particle->sprite->images[particle->sprite->counter], particle->r, particle->g, particle->b);
    SDL_SetTextureAlphaMod(particle->sprite->images[particle->sprite->counter], particle->alpha);
}

void makesprite(struct sprite ** sprite, SDL_Texture ** images) {
    (*sprite) = malloc(sizeof(struct sprite));
    (*sprite)->images = images;    
//    sprite->images = malloc(2 * sizeof(SDL_Texture *));
    (*sprite)->counter = 0;
    SDL_QueryTexture(images[0], NULL, NULL, &(*sprite)->width, &(*sprite)->height);
}

void drawsprite(double x, double y, struct sprite * sprite, double angle, double size, int clip) {
    SDL_Rect trect = {(x - sprite->width / 4) * sr, (y - sprite->height / 4) * sr, sprite->width / 2 * sr, sprite->height / 2 * sr};
    SDL_Rect crect = {sprite->width * clip, 0, sprite->width, sprite->height};
    double degrees = angle / (2 * PI) * 360;
    SDL_RenderCopyEx(renderer, sprite->images[sprite->counter], &crect, &trect, degrees, NULL, SDL_FLIP_NONE);
}

void drawparticle(struct particle * particle) {
    drawsprite(particle->x, particle->y, particle->sprite, particle->angle, 1.0, particle->clip);
}


void moveparticle(struct particle * particle, double rdeltatime) {
    particle->x += particle->velx * rdeltatime;
    particle->y += particle->vely * rdeltatime;
    particle->velx += particle->accx - particle->drag * particle->velx * rdeltatime;
    particle->vely += particle->accy - particle->drag * particle->vely * rdeltatime;
    particle->lifetime -= rdeltatime;
}
        
int main(int argc, char *argv[]) {
    head = malloc(sizeof(struct node));
    tail = malloc(sizeof(struct node));
    tail->next = NULL;
    tail->particle = NULL;
    head->next = tail;
    head->particle = NULL;
//    int imgwidth, imgheight;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return EXIT_FAILURE;
    window = SDL_CreateWindow("particles", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL) return EXIT_FAILURE;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) return EXIT_FAILURE;
    SDL_Texture * sadface0 = IMG_LoadTexture(renderer, "sad1.png");
    SDL_Texture * sadface1 = IMG_LoadTexture(renderer, "sad2.png");
    SDL_Texture * tear = IMG_LoadTexture(renderer, "tear.png");
    SDL_Texture * alphabet = IMG_LoadTexture(renderer, "alphabet.png");
    SDL_Texture * bubble = IMG_LoadTexture(renderer, "bubble.png");
    
    SDL_GetRendererOutputSize(renderer, &rswidth, &rsheight);
    sr = rswidth / SCREEN_WIDTH;

    // make sad face sprite
    SDL_Texture ** sadimages = malloc(2 * sizeof(SDL_Texture *));
    sadimages[0] = sadface0;
    sadimages[1] = sadface1;
    makesprite(&sadsprite, sadimages);

    // make alphabet sprite
    SDL_Texture ** alphabetimages = malloc(sizeof(SDL_Texture *));
    alphabetimages[0] = alphabet;
    makesprite(&alphabetsprite, alphabetimages);
    alphabetsprite->width = alphabetsprite->height;

    // make tear sprite
    SDL_Texture ** tearimages = malloc(sizeof(SDL_Texture *));
    tearimages[0] = tear;
    makesprite(&tearsprite, tearimages);
    
    // make bubble sprite
    SDL_Texture ** bubbleimages = malloc(sizeof(SDL_Texture *));
    bubbleimages[0] = bubble;
    makesprite(&bubblesprite, bubbleimages);

    setspringmasses();
    addspringparticles();
    drawword(100, 100, "click to cry");
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);    
    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { quit = 1;
            } else if (event.type == SDL_MOUSEMOTION) {
                SDL_GetMouseState(&clickx, &clicky);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                helddown = 1;
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                helddown = 0;
            }
        }
        // step
        currtime = SDL_GetPerformanceCounter();
        timediff = currtime - prevtime;
        prevtime = currtime;
        deltatime = (double) (timediff * 1000 / (double) SDL_GetPerformanceFrequency());
        int steps = (int) ((deltatime - leftovertime) / TIME_PER_STEP);
        extratime = deltatime - leftovertime - steps * TIME_PER_STEP;
        if (steps > MAX_STEPS) steps = MAX_STEPS;
        for (int s = 0; s < steps + 2; s++) {
            double rdeltatime;
            onstep = 0;
            ondraw = 0;
            if (s == 0) {
                rdeltatime = leftovertime;
                onstep = 1;
            } else if (s == steps + 1) {
                rdeltatime = extratime;
                ondraw = 1;
            } else {
                rdeltatime = TIME_PER_STEP;
                onstep = 1;
            }
            SDL_SetRenderDrawColor(renderer, bgcolor[0], bgcolor[1], bgcolor[2], 255);
            SDL_RenderClear(renderer);
            struct node * prevnode = NULL;
            struct node * currnode = head;
//            printf("rdeltatime %f\n", rdeltatime);
// TODO check if this should be on step
            updatespringaccs();
            prevmody = 0;
            nextmody = 0;
            while (currnode->next != NULL) {
                int destroyed = 0;
                if (currnode->particle != NULL) {
                    // update particle
                    moveparticle(currnode->particle, rdeltatime);
                    if (currnode->particle->step != NULL) {
                        currnode->particle->step(currnode->particle);
                    }
                    if (currnode->particle->type == T_TEAR) {
                    }
                    if (currnode->particle->type != T_WATER && currnode->particle->lifetime < 0) {
                        prevnode->next = currnode->next;
                        free(currnode->particle);
                        free(currnode);
                        currnode = prevnode->next;
                        destroyed = 1;
                    } else {
                        // draw particle
                        if (ondraw) {
//                            SDL_SetRenderDrawColor(renderer, pcolor[0], pcolor[1], pcolor[2], 255);
                            struct sprite * sprite;
                            if (currnode->particle->type == T_TEAR) {
                                sprite = tearsprite;
                            } else if (currnode->particle->type == T_WATER) {
                                sprite = tearsprite;
                            } else if (currnode->particle->type == T_LETTER) {
                                sprite = alphabetsprite;
                            } else {
                                sprite = tearsprite;
                            }
                            setcolors(currnode->particle);
                            drawparticle(currnode->particle);
                        }
                    }
                }
                if (!destroyed) {
                    prevnode = currnode;
                    currnode = currnode->next;
                }
            }

            // draw face on cursor
            if (ondraw) {
                drawsprite(clickx, clicky, sadsprite, 0.0, 1.0, 0);
            }

            // cry
            if (helddown && onstep && !gameover) {
                int particlesperhold = 1;
                for (int i = 0; i < particlesperhold; i++) {
                    // prevnode is now the last node
                    if (isunderwater(clickx, clicky))
                        addparticle(makebubbleparticle(clickx, clicky));
                    else
                        addparticle(maketearparticle(clickx, clicky));
               }
            }
            
        }
        leftovertime = TIME_PER_STEP - extratime;
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}








