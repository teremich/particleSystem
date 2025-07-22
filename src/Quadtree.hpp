#pragma once
#include <type_traits>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>
#include <array>

#ifdef DEBUG
#else
#endif

template <class T>
concept hasPos = requires(T a) {
    a.x, a.y;
    std::is_trivially_copyable<T>::value;
};

template <hasPos T, size_t threshhold = 32>
class Quadtree{
    public:
    typedef T pt;
    struct Iterator{
        const Quadtree* parent;
        Iterator* child = NULL;
        size_t index = 0;
        
        Iterator() : parent(NULL) {
        }
        
        ~Iterator() {
            if (!parent) {
                return;
            }
            if (child && parent->size > threshhold) {
                delete child;
                child = NULL;
            }
        }
        
        void initChild() {
            for (; index < 4; index++) {
                assert(child == NULL);
                child = new Iterator(std::move(parent->subtrees[index].begin()));
                if (*child != parent->subtrees[index].end()) {
                    break;
                }
                delete child;
                child = NULL;
            }
        }
        
        explicit Iterator(const Quadtree* self) : parent(self) {
            if (parent->size > threshhold) {
                initChild();
            }
        }
        
        Iterator(const Quadtree* parentTree, Iterator* childIterator, size_t idx)
        : parent(parentTree), child(childIterator), index(idx) {
        }
        
        Iterator& operator=(Iterator&& moveFrom) {
            this->~Iterator();
            parent = moveFrom.parent;
            child = moveFrom.child;
            index = moveFrom.index;
            moveFrom.parent = 0;
            moveFrom.child = 0;
            moveFrom.index = 0;
            return *this;
        }
        
        Iterator(Iterator&& moveFrom) :
        parent(moveFrom.parent), child(moveFrom.child), index(moveFrom.index) {
            moveFrom.child = 0;
            moveFrom.index = 0;
            moveFrom.parent = 0;
        }

        Iterator& operator++() {
            if(next()) {
                // assert(*this == parent->end());
            }
            return *this;
        }

        bool next() {
            if (parent->size > threshhold) {
                if (child->next()) {
                    delete child;
                    child = NULL;
                    index++;
                    initChild();
                }
            } else {
                index++;
            }
            
            // fprintf(stdout, "index far enough: %d, this == end: %d\n", parent->size > threshhold ? index == 4 : index == parent->size, (*this == parent->end()));
            
            assert((parent->size > threshhold ? index == 4 : index == parent->size) == (*this == parent->end()));
            return parent->size > threshhold ? index == 4 : index == parent->size;
        }
        
        const T* operator->() const {
            return &**this;
        }
        
        const T& operator*() const {
            if (parent->size > threshhold) {
                if (!child) {
                    exit(1);
                }
                return **child;
            } else {
                return parent->items[index];
            }
        }
        bool operator==(const Iterator& rhs) const {
            return parent == rhs.parent && index == rhs.index;
        }
        bool operator!=(const Iterator& rhs) const {
            return ! (*this == rhs);
        }
        static void* operator new(size_t size) {
            void* ptr = malloc(size);
            return ptr;
        }
        static void operator delete(void* ptr) {
            free(ptr);
        }
        void print() {
            std::cout << "{" << parent << ", " << child << ", " << index << "}\n";
        }
    };
    public:
    Quadtree() = default;
    Quadtree(float x, float y, float w, float h)
    : X(x), Y(y), W(w), H(h) {
    }
    ~Quadtree() {
        if (size > threshhold) {
            delete[] subtrees;
        }
    };
    Iterator begin() const {
        return Iterator(this);
    }
    Iterator end() const {
        return {
            this, NULL, size > threshhold ? 4 : size
        };
    }
    bool test();
    void printIterators(int depth = 0) {
        for (int i = 0; i < depth; i++) {
            std::cout << "\t";
        }
        std::cout << "begin: ";
        begin().print();
        if (size > threshhold) {
            for (int i = 0; i < 4; i++) {
                subtrees[i].printIterators(depth+1);
            }
        }
        for (int i = 0; i < depth; i++) {
            std::cout << "\t";
        }
        std::cout << "end: ";
        end().print();
        for (int i = 0; i < depth+1; i++) {
            std::cout << "\t";
        }
        std::cout << "size: " << size << "\n";
    }
    void pushToSubtree(T item) {
        bool south = item.y >= Y+H/2;
        bool east = item.x >= X+W/2;
        subtrees[east + (south << 1)].push(item);
    }
    void push(T item) {
        if (item.x < X || item.x > X+W || item.y < Y || item.y > Y+H) {
            return;
        }
        if (size > threshhold) {
            pushToSubtree(item);
            size++;
        } else if (size == threshhold) {
            T tmp[threshhold];
            std::memcpy(tmp, items, sizeof(tmp));
            subtrees = new Quadtree<T, threshhold>[4];
            subtrees[0] = Quadtree<T, threshhold>(X    , Y    , W/2, H/2); // NW
            subtrees[1] = Quadtree<T, threshhold>(X+W/2, Y    , W/2, H/2); // NE
            subtrees[2] = Quadtree<T, threshhold>(X    , Y+H/2, W/2, H/2); // SW
            subtrees[3] = Quadtree<T, threshhold>(X+W/2, Y+H/2, W/2, H/2); // SE
            size++;
            for (size_t i = 0; i < threshhold; i++) {
                pushToSubtree(tmp[i]);
            }
            pushToSubtree(item);
            auto sum = subtrees[0].size + subtrees[1].size + subtrees[2].size + subtrees[3].size;
            assert(size == sum);
        } else {
            items[size++] = item;
        }
    }
    std::vector<T> get(float x, float y, float w, float h) const {
        std::vector<T> ret;
        if (x+w < X || x > X+W || y+h < Y || y > Y+H) {
            return ret;
        }
        if (size > threshhold) {
            for (int i = 0; i < 4; i++) {
                const std::vector<T> newElements = subtrees[i].get(x, y, w, h);
                ret.insert(
                    ret.end(),
                    newElements.begin(),
                    newElements.end()
                );
            }
            return ret;
        }
        for (size_t i = 0; i < threshhold; i++) {
            if (items[i].x < x || items[i].x > x+w || items[i].y < y || items[i].y > y+h) {
                continue;
            }
            ret.push_back(items[i]);
        }
        return ret;
    }
    std::vector<T> get(float x, float y, float r) const {
        std::vector<T> ret;
        if (x+r < X || x > X+W || y+r < Y || y > Y+H) {
            return ret;
        }
        if (size > threshhold) {
            for (int i = 0; i < 4; i++) {
                const std::vector<T> newElements = subtrees[i].get(x, y, r);
                ret.insert(
                    ret.end(),
                    newElements.begin(),
                    newElements.end()
                );
            }
            return ret;
        }
        for (size_t i = 0; i < threshhold; i++) {
            if (dist2(items[i].x, items[i].y, X, Y) <= r*r) {
                ret.push_back(items[i]);
            }
        }
        return ret;
    }
    struct rect{
        float x,y,w,h;
    };
    rect getRect() const {
        return {X,Y,W,H};
    }
    size_t getSize() const {
        return size;
    }
    static constexpr auto threshold = threshhold;
    std::array<const Quadtree*, 4> getSubtrees() const {
        if (size > threshhold) {
            return {&subtrees[0], &subtrees[1], &subtrees[2], &subtrees[3]};
        }
        return {};
    }
    private:
    std::size_t size = 0;
    float X, Y, W, H;
    union{
        Quadtree<T, threshhold>* subtrees = nullptr;
        // NW, NE, SW, SE
        T items[threshhold];
    };
    private:
        static float dist2(float x1, float y1, float x2, float y2) {
            const float dx = x1-x2;
            const float dy = y1-y2;
            return dx*dx+dy*dy;
        }
};
