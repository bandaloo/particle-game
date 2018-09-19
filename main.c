#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720
#define VSCALAR 20000000
#define PI 3.14159265
#define SIM_FPS 300
#define TIME_PER_STEP (double) (1000.0 / SIM_FPS)
#define MAX_STEPS 10
#define NUM_MASSES 20
#define WATER_DEPTH 500

#define T_TEAR 0
#define T_WATER 1

int bgcolor[] = {30, 30, 30};
int pcolor[] = {91, 187, 255};

struct particle {
    double x;
    double y;
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

double randdouble() {
    return (double) rand() / RAND_MAX;
}


int clickx, clicky;
int helddown;
long unsigned int timediff, currtime, prevtime = 0;
double extratime;
double deltatime;
double leftovertime = 0;
struct node * head;
struct node * tail;
SDL_Window * window;
SDL_Renderer * renderer;

int rswidth, rsheight, sr;
double waterdepth = WATER_DEPTH;

struct particle springmasses[NUM_MASSES];

void setspringmasses() { 
    for (int i = 0; i < NUM_MASSES; i++) {
        springmasses[i] = (struct particle) {
            .x = SCREEN_WIDTH / (NUM_MASSES - 1) * i,
            .y = WATER_DEPTH,
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
            .type = T_WATER
        };
    }
}

void updatespringaccs() {
    printf("teststart\n");
    for (int i = 1; i < NUM_MASSES - 1; i++) {
        double yleft = springmasses[i - 1].y;
        double ymiddle = springmasses[i].y;
        double yright = springmasses[i + 1].y;
        springmasses[i].accy = ((yleft - ymiddle) + (yright - ymiddle)) / 1000;
        printf("%f\n", ymiddle);
    }
    printf("testend\n");
}

void addspringparticles() {
    struct node * currnode = head;
    for (int i = 0; i < NUM_MASSES; i++) {
        struct node * newnode = malloc(sizeof(struct node));
        newnode->particle = springmasses + i;
        newnode->next = head->next;
        head->next = newnode;
    }
}

void drawimage(double x, double y, SDL_Texture * image, int imgwidth, int imgheight) {
    SDL_Rect trect = {(x - imgwidth / 4) * sr, (y - imgheight / 4) * sr, imgwidth / 2 * sr, imgheight / 2 * sr};
    SDL_RenderCopy(renderer, image, NULL, &trect);
}

void makesprite(struct sprite ** sprite, SDL_Texture ** images) {
    (*sprite) = malloc(sizeof(struct sprite));
    (*sprite)->images = images;    
//    sprite->images = malloc(2 * sizeof(SDL_Texture *));
    (*sprite)->counter = 0;
    SDL_QueryTexture(images[0], NULL, NULL, &(*sprite)->width, &(*sprite)->height);
}

void drawsprite(double x, double y, struct sprite * sprite, double angle, double size) {
    SDL_Rect trect = {(x - sprite->width / 4) * sr, (y - sprite->height / 4) * sr, sprite->width / 2 * sr, sprite->height / 2 * sr};
    double degrees = angle / (2 * PI) * 360;
    SDL_RenderCopyEx(renderer, sprite->images[sprite->counter], NULL, &trect, degrees, NULL, SDL_FLIP_NONE);
}

//void drawimageex(double x, double y, SDL_T

int main(int argc, char *argv[]) {
    setspringmasses();
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
    
//    SDL_QueryTexture(sadface, NULL, NULL, &imgwidth, &imgheight);
    SDL_GetRendererOutputSize(renderer, &rswidth, &rsheight);
    sr = rswidth / SCREEN_WIDTH;

    // make sad face sprite
    struct sprite * sadsprite;
    SDL_Texture ** sadimages = malloc(2 * sizeof(SDL_Texture *));
    sadimages[0] = sadface0;
    sadimages[1] = sadface1;
    makesprite(&sadsprite, sadimages);

    struct sprite * tearsprite;
    SDL_Texture ** tearimages = malloc(sizeof(SDL_Texture *));
    tearimages[0] = tear;
    makesprite(&tearsprite, tearimages);

    addspringparticles();
    
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
        printf("deltatime %f\n", deltatime);
        printf("timediff %d\n", timediff);
        printf("time per step %d\n", TIME_PER_STEP);
        int steps = (int) ((deltatime - leftovertime) / TIME_PER_STEP);
        extratime = deltatime - leftovertime - steps * TIME_PER_STEP;
        printf("steps %d\n", steps);
        printf("extratime %f\n", extratime);
//        printf("%l timediff", timediff);
        if (steps > MAX_STEPS) steps = MAX_STEPS;
        for (int s = 0; s < steps + 2; s++) {
            double rdeltatime;
            int onstep = 0;
            int ondraw = 0;
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
            // temp
            int counter = 0;
            printf("rdeltatime %f\n", rdeltatime);
            updatespringaccs();
            while (currnode->next != NULL) {
            
                int destroyed = 0;
                if (currnode->particle != NULL) {
                    // update particle
                    currnode->particle->x += currnode->particle->velx * rdeltatime;
                    currnode->particle->y += currnode->particle->vely * rdeltatime;
                    currnode->particle->velx += currnode->particle->accx - currnode->particle->drag * currnode->particle->velx * rdeltatime;
                    currnode->particle->vely += currnode->particle->accy - currnode->particle->drag * currnode->particle->vely * rdeltatime;
                    currnode->particle->lifetime -= rdeltatime;
                    if (currnode->particle->type == T_TEAR) {
                        // figure out which water node to push
                        double pos = currnode->particle->x;
                        pos += SCREEN_WIDTH / NUM_MASSES / 2;
                        int index = (int) (pos / (SCREEN_WIDTH / (NUM_MASSES - 1)));
                        printf("index%d\n", index);
                        if (index >= 0 && index <= NUM_MASSES - 1) {
                            if (springmasses[index].y < currnode->particle->y) {
                                currnode->particle->lifetime = -1;
                                if (index != 0 && index != NUM_MASSES - 1)
                                    springmasses[index].y += 10;
                                springmasses[0].y -= 0.01;
                                springmasses[NUM_MASSES - 1].y -= 0.01;
                            }
                        }
                    }
                    if (currnode->particle->lifetime < 0) {
                        prevnode->next = currnode->next;
                        free(currnode->particle);
                        free(currnode);
                        currnode = prevnode->next;
                        destroyed = 1;
                    } else {
                        // draw particle
                        if (ondraw) {
                            SDL_SetRenderDrawColor(renderer, pcolor[0], pcolor[1], pcolor[2], 255);
                            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                            SDL_SetTextureColorMod(tearsprite->images[tearsprite->counter], currnode->particle->r, currnode->particle->g, currnode->particle->b);
                            SDL_SetTextureAlphaMod(tearsprite->images[tearsprite->counter], currnode->particle->alpha);
                            double angle = atan2(currnode->particle->vely, currnode->particle->velx);
                            drawsprite(currnode->particle->x, currnode->particle->y, tearsprite, angle, 1.0);
                        }
                        /*
                        SDL_Rect rect = {(currnode->particle->x - 4) * sr, (currnode->particle->y - 4) * sr, 4 * sr, 4 * sr};
                        SDL_SetRenderDrawColor(renderer, pcolor[0], pcolor[1], pcolor[2], 255);
                        SDL_RenderFillRect(renderer, &rect);
                        */
                    }
                }
                if (!destroyed) {
                    prevnode = currnode;
                    currnode = currnode->next;
                }
                counter++;
            }
    //        printf("counter %d\n", counter);
    //        printf("%d %d\n", clickx, clicky);
            // draw face on cursor
            if (ondraw) {
                drawsprite(clickx, clicky, sadsprite, 0.0, 1.0);
            }
            if (helddown && onstep) {
                int particlesperhold = 3;
                for (int i = 0; i < particlesperhold; i++) {
                    // prevnode is now the last node
                    struct node * newnode = malloc(sizeof(struct node));
                    struct particle * particle = malloc(sizeof(struct particle));
                    // TODO these randomizations should be determined by the particle type (or spawner?)
                    double randdir = randdouble() * 2 * PI;
                    double randspeed = randdouble() * 10;
                    particle->x = (double) clickx;
                    particle->y = (double) clicky;
                    particle->velx = cos(randdir) * (double) randspeed / 10;
                    particle->vely = sin(randdir) * (double) randspeed / 10;
                    particle->accx = 0;
                    particle->accy = 0.01;
                    particle->drag = 0.005;
                    particle->lifetime = 1000 + randdouble() * 2000;
                    particle->alpha = 100;
                    particle->r = pcolor[0];
                    particle->g = pcolor[1];
                    particle->b = pcolor[2];
                    particle->type = T_TEAR;
                    newnode->next = tail;
                    newnode->particle = particle;
                    prevnode->next = newnode;
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








