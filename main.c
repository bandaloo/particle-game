#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720
#define VSCALAR 20000000
#define PI 3.14159265

int bgcolor[] = {30, 30, 30};
int pcolor[] = {0, 0, 255};
// TODO make particles disappear and make delta time more accurate

struct particle {
    double x;
    double y;
    double velx;
    double vely;
    double accx;
    double accy;
    double drag;
    int lifetime;
};

struct node {
    struct node * next;
    struct particle * particle;
};

double randdouble() {
    return (double) rand() / (RAND_MAX + 1);
}

int clickx, clicky;
int helddown;
long unsigned int timediff, currtime, prevtime = 0;
struct node * head;
struct node * tail;

int rswidth, rsheight, sr;

int main(int argc, char *argv[]) {
    head = malloc(sizeof(struct node));
    tail = malloc(sizeof(struct node));
    tail->next = NULL;
    tail->particle = NULL;
    head->next = tail;
    head->particle = NULL;
    int imgwidth, imgheight;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return EXIT_FAILURE;
    SDL_Window * window = SDL_CreateWindow("particles", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL) return EXIT_FAILURE;
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) return EXIT_FAILURE;
    SDL_Texture * sadface = IMG_LoadTexture(renderer, "sad1.png");
    SDL_QueryTexture(sadface, NULL, NULL, &imgwidth, &imgheight);
    SDL_GetRendererOutputSize(renderer, &rswidth, &rsheight);
    sr = rswidth / SCREEN_WIDTH;

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
        SDL_SetRenderDrawColor(renderer, bgcolor[0], bgcolor[1], bgcolor[2], 255);
        SDL_RenderClear(renderer);
        // step
        currtime = SDL_GetPerformanceCounter();
        timediff = currtime - prevtime;
        prevtime = currtime;
//        printf("%l timediff", timediff);
        struct node * prevnode = NULL;
        struct node * currnode = head;
        // temp
        int counter = 0;
        while (currnode->next != NULL) {
            int destroyed = 0;
            if (currnode->particle != NULL) {
                // update particle
                currnode->particle->x += currnode->particle->velx * (double) timediff / VSCALAR;
                currnode->particle->y += currnode->particle->vely * (double) timediff / VSCALAR;
                // TODO correct drag
                currnode->particle->velx += currnode->particle->accx - currnode->particle->drag * currnode->particle->velx * (double) timediff / VSCALAR;
                currnode->particle->vely += currnode->particle->accy - currnode->particle->drag * currnode->particle->vely * (double) timediff / VSCALAR;
                currnode->particle->lifetime -= timediff;
                if (currnode->particle->lifetime < 0) {
                    prevnode->next = currnode->next;
                    free(currnode->particle);
                    free(currnode);
                    currnode = prevnode->next;
                    destroyed = 1;
                } else {
                    // draw particle
                    SDL_Rect rect = {(currnode->particle->x - 4) * sr, (currnode->particle->y - 4) * sr, 4 * sr, 4 * sr};
                    SDL_SetRenderDrawColor(renderer, pcolor[0], pcolor[1], pcolor[2], 255);
                    SDL_RenderFillRect(renderer, &rect);
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
        SDL_Rect trect = {(clickx - imgwidth / 4) * sr, (clicky - imgheight / 4) * sr, imgwidth / 2 * sr, imgheight / 2 * sr};
//        printf("SR %d\n", sr);
        SDL_RenderCopy(renderer, sadface, NULL, &trect);
        if (1) {
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
                particle->velx = cos(randdir) * (double) randspeed;
                particle->vely = sin(randdir) * (double) randspeed;
//                printf("%f\n", randspeed);
                particle->accx = 0;
                particle->accy = 1;
                particle->drag = 0.05;
                particle->lifetime = 2000000000 + randdouble() * 1000000000;
                newnode->next = tail;
                newnode->particle = particle;
                prevnode->next = newnode;
            }
        }
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}








