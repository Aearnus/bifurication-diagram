#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>

const long double MAX_ERROR = 0.00001;
const int MAX_ITERATIONS = 1000;
const int X_RES = 1366;
const int Y_RES = 768;
long double R_MAX = 4;
long double R_MIN = 0;
long double Y_MAX = 1;
long double Y_MIN = 0;
bool DIRTY_UPDATE = false;
const long double INVERSE_MOVE_SPEED = 500;
const long double ZOOM_SPEED = 0.01;
//long double R_MAX = 4;
//long double R_MIN = 0;
//long double Y_MAX = 1;
//long double Y_MIN = 0;
long double CURRENT_R = 0;
long double CURRENT_Y = 0;
const long double STARTING_VALUE = 0.5;

long double f(long double x, long double r) {
    return (r*x*(1 - x));
    //return std::pow(r, x);
}

struct MatchData {
    bool found;
    int where;
};

MatchData findFloatInVector(long double x, std::vector<long double> vector) {
    MatchData out;
    for (int i = 0; i < vector.size(); i++) {
        if (std::abs(vector[i] - x) < MAX_ERROR) {
            out.found = true;
            out.where = i;
            return out;
        }
    }
    out.found = false;
    return out;
}

struct CycleData {
    bool converging;
    long double rValue;
    long double startingValue;
    std::vector<long double> cycle;
    long long iterationsUntilCycle;
};
CycleData fixedCycle(long double x, long double r) {
    CycleData out;
    out.converging = false;
    out.startingValue = x;
    out.rValue = r;
    std::vector<long double> outCycle;
    std::vector<long double> iterations;
    //iterate until fixed point or cycle
    x = f(x, r);
    for (long long i = 0; i < MAX_ITERATIONS; i++) {
        iterations.push_back(x);
        x = f(x, r);
        if (findFloatInVector(x, iterations).found) {
            out.converging = true;
            out.iterationsUntilCycle = i+1;
            break;
        }
    }
    if (out.converging == true) {
        //record fixed point or cycle
        for (int i = 0; i < MAX_ITERATIONS; i++) {
            outCycle.push_back(x);
            x = f(x, r);
            if (findFloatInVector(x, outCycle).found) {
                break;
            }
        }
        out.cycle = outCycle;
    }
    return out;
}

void printCycle(CycleData c) {
    if (c.converging) {
        if (c.cycle.size() == 1) {
            std::cout
            << "Fixed point found for f(" << c.startingValue << ")." << std::endl
            << "    Fixed point: " << c.cycle.at(0) << std::endl
            << "    Iterations to reach fixed point: " << c.iterationsUntilCycle << std::endl;
        } else {
            std::cout
            << "Cycle found for f(" << c.startingValue << ")." << std::endl
            << "    Cycle length: " << c.cycle.size() << std::endl
            << "    Iterations to reach cycle: " << c.iterationsUntilCycle << std::endl
            << "    Cycle: ";
            for (auto cs : c.cycle) {
                std::cout << cs << ", ";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "No cycle or fixed point found for f(" << c.startingValue << ")." << std::endl;
    }
}

void calcPoints(SDL_Renderer *ren, std::vector<CycleData> *cycleList) {
    //DIRTY_UPDATE is managed by the other thread
    for (;;) {
        for (long double r = R_MIN; r < R_MAX; r += ((R_MAX-R_MIN)/X_RES)) {
            if (DIRTY_UPDATE) {
                break;
            }
            CycleData cycle = fixedCycle(STARTING_VALUE, r);
            //printCycle(cycle);
            cycleList->push_back(cycle);
        }
        while (!DIRTY_UPDATE) {
            SDL_Delay(10);
        }
        *cycleList = std::vector<CycleData>();
    }
}

void drawCoords(TTF_Font *font, SDL_Renderer *ren, long double r, long double x, long double y) {
    SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
    std::stringstream coordsText;
    coordsText
    << "r = " << CURRENT_R << "\n"
    << "xn = " << CURRENT_Y << "\n";
    SDL_Surface *coordsSurface = TTF_RenderText_Blended_Wrapped(font, coordsText.str().c_str(), {255, 255, 255, 255}, X_RES);
    SDL_Texture *coords = SDL_CreateTextureFromSurface(ren, coordsSurface);
    SDL_Rect coordsRect;
    coordsRect.x = 10;
    coordsRect.y = 10;
    coordsRect.w = X_RES;
    coordsRect.h = 24*3;
    SDL_RenderCopy(ren, coords, NULL, &coordsRect);
    SDL_FreeSurface(coordsSurface);
    SDL_DestroyTexture(coords);
}

long double lerp(long double t, long double a, long double b){
      return (1-t)*a + t*b;
}

int main() {
    bool go = true;
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    TTF_Font *font = TTF_OpenFont("DejaVuSansMono.ttf", 24);
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);
    std::stringstream windowTitle;
    windowTitle << "Bifurication of f("<< STARTING_VALUE << "), " << R_MIN << " < r < " << R_MAX << " " << Y_MIN << " < y < " << Y_MAX;
    SDL_Window *win = SDL_CreateWindow(windowTitle.str().c_str(), 0,0, 1366,768, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    //create calculation thread and begin calculations
    std::vector<CycleData> cycleList;
    std::thread calculationThread(calcPoints, ren, &cycleList);
    //enter main loop
    while (go) {
        //begin draw loop
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        drawCoords(font, ren, 0, 0, 0);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        for (long long cycleI = 0; cycleI < cycleList.size(); cycleI++) {
            CycleData cycle;
            //only update if it's not dirty rendering
            //avoid a nasty race condition
            if (!DIRTY_UPDATE) {
                cycle = cycleList.at(cycleI);
                if (cycle.converging) {
                    for (auto cs : cycle.cycle) {
                        SDL_RenderDrawPoint(ren,
                            (cycle.rValue - R_MIN) * (X_RES / (R_MAX-R_MIN)),
                            Y_RES - ((cs - Y_MIN) * (Y_RES / (Y_MAX-Y_MIN)))
                        );
                    }
                }
            }
        }
        SDL_RenderPresent(ren);
        //handle controls
        SDL_PumpEvents(); //update keyboard array
        //movement
        DIRTY_UPDATE = false;
        long double moveAmountR = (R_MAX - R_MIN) / INVERSE_MOVE_SPEED;
        long double moveAmountY = (Y_MAX - Y_MIN) / INVERSE_MOVE_SPEED;
        if (keyboard[SDL_SCANCODE_RIGHT]) {
            R_MAX += moveAmountR;
            R_MIN += moveAmountR;
            DIRTY_UPDATE = true;
        }
        if (keyboard[SDL_SCANCODE_LEFT]) {
            R_MAX -= moveAmountR;
            R_MIN -= moveAmountR;
            DIRTY_UPDATE = true;
        }
        if (keyboard[SDL_SCANCODE_UP]) {
            Y_MAX += moveAmountY;
            Y_MIN += moveAmountY;
            DIRTY_UPDATE = true;
        }
        if (keyboard[SDL_SCANCODE_DOWN]) {
            Y_MAX -= moveAmountY;
            Y_MIN -= moveAmountY;
            DIRTY_UPDATE = true;
        }
        //zooming
        if (keyboard[SDL_SCANCODE_E]) {
            Y_MAX = lerp(ZOOM_SPEED, Y_MAX, (Y_MAX + Y_MIN) / 2);
            Y_MIN = lerp(ZOOM_SPEED, Y_MIN, (Y_MAX + Y_MIN) / 2);
            R_MAX = lerp(ZOOM_SPEED, R_MAX, (R_MAX + R_MIN) / 2);
            R_MIN = lerp(ZOOM_SPEED, R_MIN, (R_MAX + R_MIN) / 2);
            DIRTY_UPDATE = true;
        }
        if (keyboard[SDL_SCANCODE_Q]) {
            Y_MAX = lerp(-ZOOM_SPEED, Y_MAX, (Y_MAX + Y_MIN) / 2);
            Y_MIN = lerp(-ZOOM_SPEED, Y_MIN, (Y_MAX + Y_MIN) / 2);
            R_MAX = lerp(-ZOOM_SPEED, R_MAX, (R_MAX + R_MIN) / 2);
            R_MIN = lerp(-ZOOM_SPEED, R_MIN, (R_MAX + R_MIN) / 2);
            DIRTY_UPDATE = true;
        }
        //handle events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {
                    go = false;
                }
                case SDL_MOUSEMOTION: {
                    CURRENT_Y = ((Y_RES - (long double)e.motion.y) / Y_RES) * (Y_MAX - Y_MIN) + Y_MIN;
                    CURRENT_R = (((long double)e.motion.x / X_RES) * (R_MAX - R_MIN)) + R_MIN;
                }
            }
        }
        SDL_Delay(5);
    }
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(ren);
	SDL_Quit();
	return 0;
}
