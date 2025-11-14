#pragma once

#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <new>
#include <stdexcept>
#include <utility>
#include <list>
#include <memory>
#include <algorithm>

class CustomMemoryResource : public std::pmr::memory_resource {
    struct MemoryBlock {
        void * ptr;
        size_t size{0};
        size_t alignment;
        bool free;
    };

    std::list<MemoryBlock> used_blocks;

public:

    CustomMemoryResource() = default;
    ~CustomMemoryResource() override {
        for (auto &b : used_blocks) {
            std::cout << "  [MR] delete ptr=" << b.ptr
                          << " size=" << b.size
                          << " align=" << b.alignment
                          << (b.free ? " (was free)\n" : " (was used)\n");
            ::operator delete(b.ptr, std::align_val_t(b.alignment));
            b.ptr = nullptr;
        }
    }

    void* do_allocate(size_t bytes, size_t alignment) override {
        for (auto &b : used_blocks) {
            if (b.free && b.size >= bytes && b.alignment >= alignment) {
                b.free = false;
                std::cout << "   Повторное выделение: адрес " << b.ptr << ", размер " << bytes << " байт" << std::endl;
                return b.ptr;
            }
        }
        
        void *p = ::operator new(bytes, std::align_val_t(alignment));
        std::cout << "   Выделение: адрес " << p << ", размер " << bytes << " байт" << std::endl;
        used_blocks.push_back(MemoryBlock{p, bytes, alignment, false});
        return p;
    }

    void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
        for (auto &b : used_blocks) {
            if (b.ptr == ptr) {
                b.free = true;
                std::cout << "   Освобождение: адрес " << ptr << ", размер " << bytes << " байт" << std::endl;
                return;
            }
        }

        throw std::logic_error("Попытка освобождения не выделенного блока");
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

template <class ArrayType>
class ArrayIterator {
    ArrayType* array_pointer;
    size_t current_index; 
    size_t array_size;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename ArrayType::item_type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    ArrayIterator() : array_pointer(nullptr), current_index(0), array_size(0) {}

    ArrayIterator(ArrayType* array_pointer, size_t start_index, size_t array_size) :
        array_pointer(array_pointer), current_index(start_index), array_size(array_size) {}


    reference operator*() {
        if (current_index >= array_size) {
            throw std::out_of_range("Выход за границы массива");
        }
        return (*array_pointer)[current_index];
    }

    pointer operator->() {
        if (current_index >= array_size) {
            throw std::out_of_range("Выход за границы массива");
        }
        return &(*array_pointer)[current_index];
    }

    bool operator==(const ArrayIterator<ArrayType> &other) const {
        return (other.current_index == current_index) && (other.array_pointer == array_pointer);
    }

    bool operator!=(const ArrayIterator<ArrayType> &other) const {
        return (other.current_index != current_index) || (other.array_pointer != array_pointer);
    }

    ArrayIterator<ArrayType> &operator++() {
        ++current_index;
        return *this;
    }

    ArrayIterator<ArrayType> operator++(int) {
        ArrayIterator<ArrayType> temp = *this;
        ++current_index;
        return temp;
    }
};

template <class T, class allocator_type>
    requires std::is_default_constructible_v<T> && 
             std::is_same_v<allocator_type, std::pmr::polymorphic_allocator<T>>
class PmrVector {
private:
    struct PolymorphicDeleter {
        void operator()(T* ptr) const {
        }
    };

    using LimitedUniquePtr = std::unique_ptr<T, PolymorphicDeleter>;

    allocator_type polymorphic_allocator;
    LimitedUniquePtr data_pointer;
    std::size_t size;
    std::size_t capacity;

    void ensure_capacity(size_t new_size) {
        if (new_size <= capacity) {
            return;
        }

        size_t new_capacity = std::max(new_size, capacity * 2);
        T* new_data = polymorphic_allocator.allocate(new_capacity);

        for (size_t i = 0; i < size; i++) {
            polymorphic_allocator.construct(new_data + i, std::move_if_noexcept(data_pointer.get()[i]));
            std::allocator_traits<allocator_type>::destroy(polymorphic_allocator, data_pointer.get() + i);
        }

        if (data_pointer) {
            polymorphic_allocator.deallocate(data_pointer.get(), capacity);
        }
        data_pointer = LimitedUniquePtr(new_data, PolymorphicDeleter{});
        capacity = new_capacity;
    }
public:
    using item_type = T;
    using iterator = ArrayIterator<PmrVector<T, allocator_type>>;

    PmrVector(std::size_t capacity, allocator_type alloc = {}) 
            : polymorphic_allocator(alloc), data_pointer(nullptr), capacity(capacity), size(0) {
        if (capacity > 0) {
            T* raw_pointer = polymorphic_allocator.allocate(capacity);
            data_pointer = std::unique_ptr<T, PolymorphicDeleter>(raw_pointer, PolymorphicDeleter{});
        }
    }

    void push_back(const T& value) {
        ensure_capacity(size + 1);
        polymorphic_allocator.construct(data_pointer.get() + size, value);
        ++size;
    }

    void push_back(T&& value) {
        ensure_capacity(size + 1);
        polymorphic_allocator.construct(data_pointer.get() + size, std::move(value));
        ++size;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        ensure_capacity(size + 1);
        polymorphic_allocator.construct(data_pointer.get() + size, std::forward<Args>(args)...);
        ++size;
    }

    void clear() noexcept {
        for (size_t element_index = 0; element_index < size; ++element_index) {
            std::allocator_traits<allocator_type>::destroy(polymorphic_allocator, data_pointer.get() + element_index);
            
        }
        size = 0;
    }

    T& operator[](std::size_t index) {
        if (index < size) {
            return data_pointer.get()[index];
        } else {
            throw std::out_of_range("Выход за границы массива");
        }
    }

    const T& operator[](std::size_t index) const {
        if (index < size) {
            return data_pointer.get()[index];
        } else {
            throw std::out_of_range("Выход за границы массива");
        }
    }

    iterator begin() {
        return iterator(this, 0, size);
    }

    iterator end() {
        return iterator(this, size, size);
    }

    std::size_t get_size() const {
        return size;
    }

    bool empty() const {
        return size == 0;
    }

    T& front() {
        if (size == 0) {
            throw std::out_of_range("Массив пуст");
        }
        return data_pointer.get()[0];
    }

    const T& front() const {
        if (size == 0) {
            throw std::out_of_range("Массив пуст");
        }
        return data_pointer.get()[0];
    }

    T& back() {
        if (size == 0) {
            throw std::out_of_range("Массив пуст");
        }
        return data_pointer.get()[size - 1];
    }

    const T& back() const {
        if (size == 0) {
            throw std::out_of_range("Массив пуст");
        }
        return data_pointer.get()[size - 1];
    }


    ~PmrVector() {
        if (!data_pointer) {
            return;
        }
        if constexpr (std::is_destructible_v<T>) {
            for (size_t element_index = 0; element_index < size; ++element_index) {
                std::allocator_traits<allocator_type>::destroy(polymorphic_allocator, 
                                                              data_pointer.get() + element_index);
            }
        }
        polymorphic_allocator.deallocate(data_pointer.get(), capacity);
    }
};

