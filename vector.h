#ifndef INCLUDE_KERNEL_UTIL_VECTOR_H_
#define INCLUDE_KERNEL_UTIL_VECTOR_H_

/**
 * A resizable array implementation.
 *
 * Important Notes:
 *    MAKES NO GUARANTEE ABOUT THREAD SAFETY.
 *
 *    Vector relies on move semantics, meaning the typename
 *    needs to have a move constructor and move assignment.
 *    If the typename implements any of these with "= delete",
 *    it cannot be used as the Vector typename.
 *
 * Feature flags:
 *   [bound-check]: prints a warning if an out-of-bounds access is made
 *
 * Public API:
 *
 * Constructors, Destructors, and Assignment:
 *    Vector()
 *      Creates a Vector with some default reserved capacity.
 *
 *    Vector(size_t capacity)
 *      @param capacity   the initial capacity to reserve
 *
 *    Vector(const Vector& other)
 *      @param other  the Vector to copy
 *
 *    Vector(Vector&& other)
 *      @param other  the Vector to move
 *
 *    ~Vector()
 *
 *    Vector& operator=(const Vector& other)
 *      @param other  the Vector to copy-assign
 *      @return a reference to this vector
 *
 *    Vector& operator=(Vector&& other)
 *      @param other  the Vector to move-assign
 *      @return a reference to this vector
 *
 * Element Access:
 *    [bound-check]
 *    T& at(size_t pos)
 *      Returns a reference to the element at specified location pos.
 *      @param pos  the position of the element to return
 *      @return a reference to the requested element
 *
 *    T& operator[](size_t pos)
 *      Returns a reference to the element at specified location pos.
 *      No bounds checking is performed.
 *      @param pos  the position of the element to return
 *      @return a reference to the requested element
 *
 *    [bound-check]
 *    T& front()
 *      @return a reference to the first element in the container
 *
 *    [bound-check]
 *    T& back()
 *      @return a reference to the last element in the container
 *
 *    T* data()
 *      @return the pointer to the underlying array
 *
 * Capacity:
 *    bool empty() const
 *      @return true if the container is empty, false otherwise
 *
 *    size_t size() const
 *      @return the number of elements in the container
 *
 *    size_t capacity() const
 *      @return the number of elements the container has currently
 *              allocated space for
 *
 * Modifiers:
 *
 *    void clear()
 *      Erases all elements from the container.
 *      After this call, size() returns zero.
 *
 *    [bound-check]
 *    void insert(const size_t pos, const T& value)
 *      Inserts an element at the specified position in the container.
 *      @param pos    the position to insert the element at
 *      @param value  the element to insert
 *
 *    [bound-check]
 *    T remove(size_t pos)
 *      Removes and returns the element in the container at the
 *      specified position.
 *      @param pos  the position of the element to remove
 *      @return the removed element
 *
 *    void pushBack(const T& value)
 *      Appends the given element to the end of the container.
 *      @param value  the element to append
 *
 *    void pushBack(T&& x)
 *      Appends the given element to the end of the container
 *      using move semantics.
 *      @param value the element to append
 *
 *    void popBack()
 *      Removes the last element of the container.
 *      Does nothing if the container is empty.
 *
 *
 * We do not have a standardized API format yet. If you have any
 * suggestions, please let the C++ STL team know.
 */

/**
 * TODO: memory optimization - overload insert with move semantics
 *
 * TODO: add conditional compilation for boundCheck
 */

/**
 * Compatibility macros for testing outside of the kernel.
 * boundCheck is #undef'd at the end of the header.
 */
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace util {

    template <typename T>
    class Vector {
    private:
        static constexpr size_t DEFAULT_CAP = 1;

        size_t c_size;
        size_t cap;
        T* arr;  // owning

        /**
         * Caller's responsibility to reset c_size if applicable.
         */
        void delete_arr() {
            if (arr != nullptr) {
                for (size_t i = 0; i < c_size; i++) {
                    arr[i].~T();
                }

                free(arr);
                arr = nullptr;
            }
        }

    public:
        Vector() : Vector(DEFAULT_CAP) {}

        explicit Vector(size_t capacity) {
            if (capacity < DEFAULT_CAP) {
                capacity = DEFAULT_CAP;
            }
            c_size = 0;
            cap = capacity;
            arr = static_cast<T*>(malloc(cap * sizeof(T)));
            bzero(arr, cap * sizeof(T));
        }

        Vector(const Vector& other) : c_size(other.c_size), cap(other.cap) {
            arr = static_cast<T*>(malloc(cap * sizeof(T)));
            bzero(arr, cap * sizeof(T));

            for (size_t i = 0; i < c_size; i++) {
                new (arr + i) T(other.arr[i]);
            }
        }

        Vector(Vector&& other)
                : c_size(other.c_size), cap(other.cap), arr(other.arr) {
            other.arr = nullptr;
            other.c_size = 0;
            other.cap = 0;
        }

        ~Vector() { delete_arr(); }

        Vector& operator=(const Vector& other) {
            if (this != &other) {
                delete_arr();

                c_size = other.c_size;
                cap = other.cap;
                arr = static_cast<T*>(malloc(cap * sizeof(T)));
                bzero(arr, cap * sizeof(T));

                for (size_t i = 0; i < c_size; i++) {
                    new (arr + i) T(other.arr[i]);
                }
            }

            return *this;
        }

        Vector& operator=(Vector&& other) {
            if (this != &other) {
                delete_arr();

                c_size = other.c_size;
                cap = other.cap;
                arr = other.arr;

                other.arr = nullptr;
                other.c_size = 0;
                other.cap = 0;
            }

            return *this;
        }

        T& at(size_t pos) {
            boundCheck(!(pos < size()));
            return arr[pos];
        }

        const T& at(size_t pos) const {
            boundCheck(!(pos < size()));
            return arr[pos];
        }

        T& operator[](size_t pos) { return arr[pos]; }

        const T& operator[](size_t pos) const { return arr[pos]; }

        T& front() {
            boundCheck(empty());
            return arr[0];
        }

        T& back() {
            boundCheck(empty());
            return arr[c_size - 1];
        }

        T* data() { return arr; }

        const T* data() const { return (const T*)arr; }

        bool empty() const { return c_size == 0; }

        size_t size() const { return c_size; }

        void setSize(const size_t new_size) {
            if (new_size <= c_size) {
                for (size_t i = new_size; i < c_size; i++) {
                    arr[i].~T();
                }
            } else if (new_size > cap) {
                reserve(new_size);
            }

            c_size = new_size;
        }

        void reserve(const size_t new_cap) {
            if (new_cap <= cap) {
                return;
            }

            T* new_arr = static_cast<T*>(malloc(new_cap * sizeof(T)));
            bzero(new_arr, new_cap * sizeof(T));

            for (size_t i = 0; i < c_size; i++) {
                new (new_arr + i) T(std::move(arr[i]));
            }

            delete_arr();  // call destructors in case move made copies
            cap = new_cap;
            arr = new_arr;
        }

        size_t capacity() const { return cap; }

        void clear() {
            for (size_t i = 0; i < c_size; i++) {
                arr[i].~T();
            }

            c_size = 0;
        }

        void insert(const size_t pos, const T& value) {
            boundCheck(pos > size());

            if (c_size == cap) {
                reserve(cap * 2 + 1);
            }

            for (size_t i = c_size; i > pos; i--) {
                new (arr + i) T(std::move(arr[i - 1]));
                arr[i - 1].~T();  // in case of a copy
            }
            new (arr + pos) T(value);
            c_size++;
        }

        T remove(size_t pos) {
            boundCheck(!(pos < size()));

            T ret = std::move(arr[pos]);

            c_size--;
            while (pos < c_size) {
                arr[pos] = std::move(arr[pos + 1]);
                pos++;
            }

            // clean up last element if move made a copy
            arr[c_size].~T();

            return ret;
        }

        void pushBack(const T& value) {
            if (c_size == cap) {
                reserve(cap * 2 + 1);
            }

            new (arr + c_size) T(value);
            c_size++;
        }

        void pushBack(T&& value) {
            if (c_size == cap) {
                reserve(cap * 2 + 1);
            }

            new (arr + c_size) T(std::move(value));  // NOLINT
            c_size++;
        }

        void popBack() {
            if (empty()) return;
            arr[c_size - 1].~T();
            c_size--;
        }

        class Iterator {
        private:
            Vector& vec;
            size_t index;

        public:
            inline Iterator(Vector& vec, int index) : vec(vec), index(index) {}

            inline T& operator*() { return vec[index]; }
            inline Iterator& operator++() {
                index++;
                return *this;
            }
            inline Iterator& operator--() {
                index--;
                return *this;
            }
            inline const bool operator==(const Iterator& other) {
                return &this->vec == &other.vec && this->index == other.index;
            }
            inline const bool operator!=(const Iterator& other) {
                return !(*this == other);
            }
        };

        inline Vector::Iterator begin() { return Vector::Iterator(*this, 0); }

        inline Vector::Iterator end() { return Vector::Iterator(*this, size()); }
    };
}  // namespace util

#endif  // INCLUDE_KERNEL_UTIL_VECTOR_H_
