#include <chrono>
#include <cstdlib>
#include <cstring>
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include "Quadtree.hpp"
#include <cstdarg>

#define NUM_POINTS_PER_COLOR 300

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
            vec2 operator*(float scale) {
                return (vec2){x*scale, y*scale};
            }
            vec2& operator *=(float scale) {
                x *= scale;
                y *= scale;
                return *this;
            }
            bool operator==(const vec2& rhs) const {
                return x == rhs.x && y == rhs.y;
            }
            bool operator!=(const vec2& rhs) const {
                return x != rhs.x || y != rhs.y;
            }
            friend std::ostream& operator<<(std::ostream& cout, const vec2& self) {
                cout << "{x: " << self.x << ", y: " << self.y << "}";
                return cout;
            }
            vec2& normalize() {
                assert(x!=0 || y!=0);
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
            YELLOW,
            END
        } type;
        bool operator==(const Point& rhs) const {
            return rhs.type == type && rhs.x == x && rhs.y == y;
        }
        friend std::ostream& operator<<(std::ostream& cout, const Point& self) {
            cout << "{x: " << self.x << ", y: " << self.y << ", vx: " << self.vx << ", vy: " << self.vy << ", type: " << (int)self.type << "}";
            return cout;
        }
        void print() const {
            fprintf(stdout, "{%f, %f}\n", x, y);
        }
    };
    Quadtree<Point> trees[2];
    Quadtree<Point> *A = &trees[0], *B = &trees[1]; // for swapping without copying
    float attracionMatrix[Point::PointType::END][Point::PointType::END];
    SDL_Color colors[Point::PointType::END];
};

App::Point::vec2 calcForce(float attracionMatrix[App::Point::PointType::END][App::Point::PointType::END], const App::Point& p, const App::Point& other) {
    App::Point::vec2 dir = p.pos - other.pos;
    const float dist2 = dir.x*dir.x+dir.y*dir.y;
    dir.normalize();
    dir *= 1./dist2;
    float mag = attracionMatrix[p.type][other.type];
    if (dist2 < 10*10) { // PUSH AWAY, TOO CLOSE
        mag = -1;
    } else if (dist2 > 300*300) { // NO INTERACTION, TOO FAR
        return {0, 0};
    }
    dir *= -mag;
    return dir;
}

void applyForce(float attracionMatrix[App::Point::PointType::END][App::Point::PointType::END], const std::vector<App::Point>& hood, App::Point& p) {
    for (const App::Point& other : hood) {
        if (other == p) {
            continue;
        }
        const auto force = calcForce(attracionMatrix, p, other);
        p.vel += force;
    }
}

void debugger(const char* info = "", ...) {
    std::va_list list;
    va_start(list, info);
    va_end(list);
}

void updateParticles(App& particles, long delta) {
    int w, h;
    SDL_GetWindowSize(particles.window, &w, &h);
    *particles.B = Quadtree<App::Point>(0, 0, w, h);
    size_t count = 0;

    for (App::Point p : *particles.A) {
        count++;
        debugger("hood");
        const auto hood = particles.A->get(p.x-50, p.y-50, 300);
        applyForce(particles.attracionMatrix, hood, p);
        p.vel *= std::pow(0.99, delta/10e6);
        int width, height;
        SDL_GetWindowSize(particles.window, &width, &height);
        const float edgeRepulsion = 0.1;
        if (p.pos.x < 5) {
            p.vel.x += edgeRepulsion;
        } else if (p.pos.x > width-5) {
            p.vel.x -= edgeRepulsion;
        }
        if (p.pos.y < 5) {
            p.vel.y += edgeRepulsion;
        } else if (p.pos.y > height-5) {
            p.vel.y -= edgeRepulsion;
        }
        p.pos += p.vel*(delta/10e6);
        if (p.x < 0) {
            p.x = 0;
            p.vx = std::abs(p.vx);
        }
        if (p.y < 0) {
            p.y = 0;
            p.vy = std::abs(p.vy);
        }
        if (p.x >= w) {
            p.x = w-1;
            p.vx = -std::abs(p.vx);
        }
        if (p.y >= h) {
            p.y = h-1;
            p.vy = -std::abs(p.vy);
        }

        // auto size_before = particles.B.getSize();
        // p.print();
        particles.B->push(p);
        // auto size_after = particles.B.getSize();
        // if (size_before+1 != size_after) {
        //     p.print();
        // }
    }
    if (count != particles.A->getSize()) {
        fprintf(stdout, "STOP\n");
        auto it = particles.A->begin();
        particles.A->printIterators();
        debugger("didnt iterate over all elements of A", &particles.A);
    }
    if (particles.A->getSize() != particles.B->getSize()) {
        debugger("A != B", &particles.A, &particles.B);
    }
    auto tmp = particles.A;
    particles.A = particles.B;
    particles.B = tmp;
}

bool update() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EventType::SDL_EVENT_QUIT:
                (void)event.quit;
                return false;
                break;
            case SDL_EventType::SDL_EVENT_KEY_DOWN:
                // if (event.key.key == SDLK_U) {
                //     updateParticles(particles);
                // }
                break;
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

void drawRect(SDL_Renderer* renderer, const Quadtree<App::Point>& a) {
    const auto [x,y,w,h] = a.getRect();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, x    , y    , x+w-1, y    );
    SDL_RenderLine(renderer, x+w-1, y    , x+w-1, y+h-1);
    SDL_RenderLine(renderer, x+w-1, y+h-1, x    , y+h-1);
    SDL_RenderLine(renderer, x    , y+h-1, x    , y    );
    if (a.getSize() > a.threshold) {
        for (int i = 0; i < 4; i++) {
            drawRect(renderer, *a.getSubtrees()[i]);
        }
    }
}

void draw(const App& a) {
    // drawRect(a.renderer, a.A);
    SDL_SetRenderScale(a.renderer, 4, 4);
    for (const App::Point& p : *a.A) {
        const auto color = a.colors[p.type];
        SDL_SetRenderDrawColor(a.renderer, color.r, color.g, color.b, color.a);
        SDL_RenderPoint(a.renderer, p.x/4, p.y/4);
    }
    SDL_SetRenderScale(a.renderer, 1, 1);
}

static size_t frameCount = 0;

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
    {
        float tmp[App::Point::PointType::END][App::Point::PointType::END] = {
            {+0.5, +0.0, +1.00, -1.0},
            {+1.0, +1.0, -1.50, +0.0},
            {-0.5, -0.5, -0.25, +0.0},
            {+0.5, +0.0, +1.00, +1.0}
        };
        std::memcpy(particles.attracionMatrix, tmp, sizeof(tmp));
    }
    {
        SDL_Color tmp[App::Point::PointType::END] = {
            {255, 0  , 0  , 255},
            {0  , 255, 0  , 255},
            {0  , 0  , 255, 255},
            {255, 255, 0  , 255},
        };
        std::memcpy(particles.colors, tmp, sizeof(tmp));
    }
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
    *particles.A = Quadtree<App::Point>(0, 0, width, height);
    for (uint8_t type = 0; type < App::Point::PointType::END; type++) {
        for (int i = 0; i < NUM_POINTS_PER_COLOR; i++) {
            particles.A->push({
                {{
                    SDL_randf()*width,
                    SDL_randf()*height,
                    SDL_randf()-1.0f,
                    SDL_randf()-1.0f
                }},
                static_cast<App::Point::PointType>(type)
            });
        }
    }
    bool isRunning = true;
    std::chrono::high_resolution_clock clock;
    auto begin = clock.now();
    long delta = 0;
    std::array<long, 100> last100deltas;
    int index = 0;
    while (isRunning) {
        frameCount++;
        updateParticles(particles, delta);
        SDL_SetRenderDrawColor(particles.renderer, 0, 0, 0, 255);
        SDL_RenderClear(particles.renderer);
        {
            draw(particles);
        }
        SDL_RenderPresent(particles.renderer);
        isRunning = update();
        const auto end = clock.now();
        while(index >= 100) {
            float avg = 0;
            for (int i = 0; i < 100; i++) {
                avg += last100deltas[i]/100.;
            }
            std::cout << "fps: " << 1e9/avg << std::endl;
            index-=100;
        }
        assert(particles.A->getSize() == NUM_POINTS_PER_COLOR*App::Point::PointType::END);
        delta = (end-begin).count();
        last100deltas[index++] = delta;
        begin = end;
    }

    SDL_DestroyRenderer(particles.renderer);
    particles.renderer = nullptr;
    SDL_DestroyWindow(particles.window);
    particles.window = nullptr;
    SDL_Quit();

    return EXIT_SUCCESS;
}
