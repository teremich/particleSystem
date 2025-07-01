#include <cstdlib>
#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include "Quadtree.hpp"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

struct App{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    // struct Point{
    //     vec2 pos, vel;
    // };
    struct Point{
        struct vec2{
            float x, y;
            vec2 operator-(const vec2& rhs) const {
                return {x-rhs.x, y-rhs.y};
            }
            vec2 operator+(const vec2& rhs) const {
                return {x+rhs.x, y+rhs.y};
            }
            vec2& operator+=(const vec2& rhs) {
                x += rhs.x;
                y += rhs.y;
                return *this;
            }
            vec2& operator *=(float scale) {
                x *= scale;
                y *= scale;
                return *this;
            }
            vec2& normalize() {
                float mag = std::sqrt(x*x + y*y);
                x /= mag;
                y /= mag;
                return *this;
            }
        };
        union {
            struct {
                float x,y,vx,vy;
            };
            struct {
                vec2 pos, vel;
            };
        };
        enum PointType : uint8_t {
            RED = 0,
            GREEN,
            BLUE,
            END
        } type;
        bool operator==(const Point& rhs) const {
            return rhs.type == type && rhs.x == x && rhs.y == y;
        }
    };
    Quadtree<Point> trees[2];
    Quadtree<Point>& A = trees[0], B = trees[1]; // for swapping without copying
    float attracionMatrix[Point::PointType::END][Point::PointType::END];
    SDL_Color colors[Point::PointType::END];
};

bool update() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EventType::SDL_EVENT_QUIT:
                (void)event.quit;
                return false;
                break;
            case SDL_EventType::SDL_EVENT_KEY_DOWN:
            case SDL_EventType::SDL_EVENT_KEY_UP:
                (void)event.key;
                break;
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP:
                (void)event.button;
            case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
                (void)event.wheel;
                break;
            case SDL_EventType::SDL_EVENT_MOUSE_MOTION:
                (void)event.motion;
                break;
        }
    }

    return true;
}

App::Point::vec2 calcForce(App& a, App::Point& p, const App::Point& other) {
    App::Point::vec2 dir = p.pos - other.pos;
    float dist2 = dir.x*dir.x+dir.y*dir.y;
    dir.normalize();
    dir *= 0.1;
    if (dist2 < 10) { // PUSH AWAY, TOO CLOSE
        float mag = 2;
        dir *= mag;
    } else if (dist2 < 100) { // INTERACTION
        float mag = a.attracionMatrix[p.type][other.type];
        dir *= -mag;
    } else { // NO INTERACTION, TOO FAR
        return {0, 0};
    }
    return dir;
}

void applyForce(App& a, const std::vector<App::Point>& hood, App::Point& p) {
    for (const App::Point& other : hood) {
        if (other == p) {
            continue;
        }
        p.vel += calcForce(a, p, other);
        p.pos += p.vel;
    }
}

void updateParticles(App& particles) {
    int w, h;
    SDL_GetWindowSize(particles.window, &w, &h);
    particles.B = Quadtree<App::Point>(0, 0, w, h);
    for (App::Point p : particles.A) {
        applyForce(particles, particles.A.get(p.x-50, p.y+50, 100, 100), p);
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        particles.B.push(p);
    }
    auto& tmp = particles.A;
    particles.A = particles.B;
    particles.B = tmp;
}

void draw(App a) {
    for (const App::Point& p : a.A) {
        const auto color = a.colors[p.type];
        SDL_SetRenderDrawColor(a.renderer, color.r, color.g, color.b, color.a);
        SDL_RenderPoint(a.renderer, p.x, p.y);
    }
}

int main() {
    App particles;
    {
        constexpr App::Point test{};
        static_assert(
            &test.x == &test.pos.x &&
            &test.y == &test.pos.y &&
            &test.vx == &test.vel.x &&
            &test.vy == &test.vel.y
        );
    }
    float tmp[App::Point::PointType::END][App::Point::PointType::END] = {
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
    };
    std::memcpy(particles.attracionMatrix, tmp, sizeof(tmp));
    SDL_Color tmpC[App::Point::PointType::END] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    std::memcpy(particles.colors, tmpC, sizeof(tmpC));
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return EXIT_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer(
        "const char* title",
        1920, 1080,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL,
        &particles.window,
        &particles.renderer
    )) {
        return EXIT_FAILURE;
    }
    int width, height;
    SDL_GetWindowSize(particles.window, &width, &height);
    particles.A = Quadtree<App::Point>(0, 0, width, height);
    for (uint8_t type = 0; type < App::Point::PointType::END; type++) {
        for (int i = 0; i < 50; i++) {
            particles.A.push({{{SDL_randf()*width, SDL_randf()*height, SDL_randf(), SDL_randf()}}, static_cast<App::Point::PointType>(type)});
        }
    }
    bool isRunning = true;
    while (isRunning) {
        updateParticles(particles);
        SDL_SetRenderDrawColor(particles.renderer, 0, 0, 0, 255);
        SDL_RenderClear(particles.renderer);
        {
            draw(particles);
        }
        SDL_RenderPresent(particles.renderer);
        isRunning = update();
    }

    SDL_DestroyRenderer(particles.renderer);
    particles.renderer = nullptr;
    SDL_DestroyWindow(particles.window);
    particles.window = nullptr;
    SDL_Quit();

    return EXIT_SUCCESS;
}
