#pragma once

#include <iterator>
#include <type_traits>
#include <vector>
#include <cstdlib>
#include <concepts>
#include <array>
#include <cstring>
#include <cassert>
#include <memory>
#include <iostream>
#include <typeinfo>

template <class T>
concept hasPos = requires(T a) {
    a.x, a.y;
    std::is_trivially_copyable<T>::value;
};

template <hasPos T, size_t threshhold = 4>
class Quadtree{
    public:
    struct Iterator{
        const Quadtree* parent;
        Iterator* child = NULL;
        size_t index = 0;

        Iterator() : parent(NULL) {
            fprintf(stdout, "%p: new Iterator(NULL)\n", this);
        }

        ~Iterator() {
            if (child && parent->size > threshhold) {
                delete child;
                child = NULL;
            }
            fprintf(stdout, "delete %p\n", this);
        }

        void initChild() {
            fprintf(stdout, "%p.initChild()\n", this);
            for (; index < 4; index++) {
                assert(child == NULL);
                child = new Iterator();
                *child = parent->subtrees[index].begin();
                if (*child != parent->subtrees[index].end()) {
                    break;
                }
                delete child;
                child = NULL;
            }
            fprintf(stdout, "index is now: %zu, child: %p\n", index, child);
        }

        explicit Iterator(const Quadtree* self) : parent(self) {
            fprintf(stdout, "%p: new Iterator(%p)\n", this, self);
            if (parent->size > threshhold) {
                initChild();
            }
        }

        Iterator(const Quadtree* parentTree, Iterator* childIterator, size_t idx)
        : parent(parentTree), child(childIterator), index(idx) {
            fprintf(stdout, "%p: new Iterator(%p, %p, %zu)\n", this, parent, child, index);
        }

        Iterator& operator++() {
            fprintf(stdout, "%p.operator++() -> am I at the end? %d, %d\n", this, next(), *this == parent->end());
            return *this;
        }

        bool next() {
            fprintf(stdout, "%p.next()\n", this);
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
            fprintf(stdout, "index is now: %zu\n", index);
            return index == parent->size;
        }

        const T& operator*() const {
            if (parent->size > threshhold) {
                if (!child) {
                    fprintf(stderr, "tried dereferencing an invalid pointer, *this == parent->end(): %d\n", *this == parent->end());
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
            std::cout << "malloc(size: "<< size << ") -> ptr: " << ptr << "\n";
            return ptr;
        }
        static void operator delete(void* ptr) {
            std::cout << "free(ptr: " << ptr << ") -> void\n";
            free(ptr);
        }
        void print() {
            std::cout << "{" << parent << ", " << child << ", " << index << "}\n";
        }
    };
    public:
    Quadtree() = default;
    Quadtree(int x, int y, int w, int h)
        : X(x), Y(y), W(w), H(h) {
            fprintf(stdout, "%p = Quadtree(%d, %d, %d, %d)\n", this, x, y, w, h);
        }
    ~Quadtree() {
        fprintf(stdout, "delete %p\n", this);
        if (size > threshhold) {
            delete[] subtrees;
        }
    };
    Iterator begin() const {
        fprintf(stdout, "%p.begin()\n", this);
        return Iterator(this);
    }
    Iterator end() const {
        fprintf(stdout, "%p.end() -> {%p, NULL, %zu}\n", this, this, size > threshhold ? 4 : size);
        return {
            this, NULL, size > threshhold ? 4 : size
        };
    }
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
        std::cout << "size: " << size << "\n";
    }
    void push(T item) {
        if constexpr (std::is_floating_point_v<decltype(T::x)>) {
            fprintf(stdout, "%p.push({%f, %f})", this, item.x, item.y);
        } else if (std::is_integral_v<decltype(T::x)>) {
            fprintf(stdout, "%p.push({%d, %d})\n", this, item.x, item.y);
        }
        if (item.x < X || item.x > X+W || item.y < Y || item.y > Y+H) {
            fprintf(stdout, "\toutside of me\n");
            return;
        }
        if (size > threshhold) {
            bool south = item.y >= Y+H/2;
            bool east = item.x >= X+W/2;
            fprintf(stdout, "\tin child %d\n", east + (south << 1));
            subtrees[east + (south << 1)].push(item);
            size++;
        } else if (size == threshhold) {
            T tmp[threshhold];
            std::memmove(tmp, items, sizeof(tmp));
            subtrees = new Quadtree<T, threshhold>[4];
            fprintf(stdout, "\tgrowing: (%p %p %p %p)\n", subtrees+0, subtrees+1, subtrees+2, subtrees+3);
            subtrees[0] = Quadtree<T, threshhold>(X    , Y    , W/2, H/2); // NW
            subtrees[1] = Quadtree<T, threshhold>(X+W/2, Y    , W/2, H/2); // NE
            subtrees[2] = Quadtree<T, threshhold>(X    , Y+H/2, W/2, H/2); // SW
            subtrees[3] = Quadtree<T, threshhold>(X+W/2, Y+H/2, W/2, H/2); // SE
            size++;
            for (size_t i = 0; i < threshhold; i++) {
                push(tmp[i]);
            }
            push(item);
        } else {
            items[size++] = item;
        }
    }
    std::vector<T> get(int x, int y, int w, int h) const {
        std::vector<T> ret;
        fprintf(stdout, "%p.get(%d, %d, %d, %d) -> ", this, x, y, w, h);
        if (x+w < X || x > X+W || y+h < Y || y > Y+H) {
            fprintf(stdout, "0\n");
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
            fprintf(stdout, "%zu\n", ret.size());
            return ret;
        }
        for (size_t i = 0; i < threshhold; i++) {
            if (items[i].x < x || items[i].x > x+w || items[i].y < y || items[i].y > y+h) {
                continue;
            }
            ret.push_back(items[i]);
        }
        fprintf(stdout, "%zu\n", ret.size());
        return ret;
    }
    private:
    int X, Y, W, H;
    private:
    std::size_t size = 0;
    union{
        Quadtree<T, threshhold>* subtrees = nullptr;
        // NW, NE, SW, SE
        T items[threshhold];
    };
};
