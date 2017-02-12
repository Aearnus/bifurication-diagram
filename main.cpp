#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>

const long double MAX_ERROR = 0.00001;
const int MAX_ITERATIONS = 1000;
const int X_RES = 1366;
const int Y_RES = 768;
const long double X_MAX = 4;
const long double X_MIN = 0;
const long double Y_MAX = 1;
const long double Y_MIN = 0;
const long double STARTING_VALUE = 0.5;

long double f(long double x, long double r) {
    //return (r*x*(1 - x));
    return std::pow(r, x);
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
    for (long double r = X_MIN; r < X_MAX; r += ((X_MAX-X_MIN)/X_RES)) {
        CycleData cycle = fixedCycle(STARTING_VALUE, r);
        printCycle(cycle);
        cycleList->push_back(cycle);
    }
}

int main() {
    bool go = true;
    SDL_Init(SDL_INIT_VIDEO);
    std::stringstream windowTitle;
    windowTitle << "Bifurication of f("<< STARTING_VALUE << "), " << X_MIN << " < r < " << X_MAX << " " << Y_MIN << " < y < " << Y_MAX;
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
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        for (auto cycle : cycleList) {
            if (cycle.converging) {
                for (auto cs : cycle.cycle) {
                    SDL_RenderDrawPoint(ren,
                        cycle.rValue * (X_RES / (X_MAX-X_MIN)),
                        Y_RES - (cs * (Y_RES / (Y_MAX-Y_MIN)))
                    );
                }
            }
        }
        SDL_RenderPresent(ren);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {
                    go = false;
                }
            }
        }
    }
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(ren);
	SDL_Quit();
	return 0;
}
