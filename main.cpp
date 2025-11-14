#include <iostream>
#include <string>
#include <memory_resource>
#include "include/pmr_vector.h"

// Сложный тип для демонстрации
struct Record {
    int         id;
    double      value;
    std::string label;
};

using IntAlloc = std::pmr::polymorphic_allocator<int>;
using RecordAlloc = std::pmr::polymorphic_allocator<Record>;

using IntVector = PmrVector<int, IntAlloc>;
using RecordVector = PmrVector<Record, RecordAlloc>;

void demo_int_vector(CustomMemoryResource& mr) {
    std::cout << "\n=== Демо 1: PmrVector<int> ===\n";

    IntAlloc alloc{ &mr };
    IntVector vec(0, alloc);  // начинаем с capacity = 0

    std::cout << "[Демо 1] Добавляем элементы 10, 20, 30, 40\n";
    for (int i = 1; i <= 4; ++i) {
        std::cout << "[Демо 1] vec.push_back(" << 10 * i << ")\n";
        vec.push_back(10 * i);
    }
    std::cout << "[Демо 1] size = " << vec.get_size()
              << ", пустой? " << std::boolalpha << vec.empty() << "\n";

    std::cout << "[Демо 1] Первый элемент (front): " << vec.front() << "\n";
    std::cout << "[Демо 1] Последний элемент (back): " << vec.back() << "\n";

    std::cout << "[Демо 1] Обход через range-based for:\n  ";
    for (int x : vec) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    std::cout << "[Демо 1] Обход через итератор (ArrayIterator):\n  ";
    for (IntVector::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << "\n";

    std::cout << "[Демо 1] Очищаем контейнер методом clear()\n";
    vec.clear();
    std::cout << "[Демо 1] После clear(): size = " << vec.get_size()
              << ", пустой? " << vec.empty() << "\n";
}

void demo_record_vector(CustomMemoryResource& mr) {
    std::cout << "\n=== Демо 2: PmrVector<Record> (сложный тип) ===\n";

    std::cout << "[Демо 2] Создаём вектор записей с начальной capacity = 2\n";
    RecordAlloc alloc{ &mr };
    RecordVector vec(2, alloc);

    std::cout << "[Демо 2] Добавляем несколько записей через emplace_back...\n";
    std::cout << "[Демо 2] vec.emplace_back(1, 3.14, \"первый\")\n";
    vec.emplace_back(1, 3.14,  "первый");
    std::cout << "[Демо 2] vec.emplace_back(2, 2.71, \"второй\")\n";
    vec.emplace_back(2, 2.71,  "второй");
    std::cout << "[Демо 2] vec.emplace_back(3, 1.414, \"третий\")\n";
    vec.emplace_back(3, 1.414, "третий");

    std::cout << "[Демо 2] size = " << vec.get_size()
              << ", пустой? " << std::boolalpha << vec.empty() << "\n";

    std::cout << "[Демо 2] Содержимое вектора:\n";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        const Record& r = *it;
        std::cout << "  id = "    << r.id
                  << ", value = " << r.value
                  << ", label = " << r.label << "\n";
    }

    std::cout << "[Демо 2] Умножаем value на 10 через итератор...\n";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        it->value *= 10.0;
    }

    std::cout << "[Демо 2] После изменения:\n";
    for (const auto& r : vec) {
        std::cout << "  id = "    << r.id
                  << ", value = " << r.value
                  << ", label = " << r.label << "\n";
    }
}

void demo_reuse_memory(CustomMemoryResource& mr) {
    std::cout << "\n=== Демо 3: Переиспользование памяти между векторами ===\n";

    {
        std::cout << "[Демо 3] Фаза 1: создаём первый вектор v1\n";
        IntAlloc alloc{ &mr };
        IntVector v1(0, alloc);

        for (int i = 0; i < 5; ++i) {
            std::cout << "[Демо 3] v1.push_back(" << i * 10 << ")\n";
            v1.push_back(i * 10);
        }

        std::cout << "[Демо 3] v1: size = " << v1.get_size() << "\n";
        std::cout << "[Демо 3] v1 элементы: ";
        for (int x : v1) {
            std::cout << x << " ";
        }
        std::cout << "\n";

        std::cout << "[Демо 3] Фаза 1 заканчивается, v1 выходит из области видимости,\n"
                     "          его память возвращается в CustomMemoryResource.\n";
    }

    {
        std::cout << "\n[Демо 3] Фаза 2: создаём второй вектор v2 с capacity = 0\n";
        IntAlloc alloc{ &mr };
        IntVector v2(0, alloc);

        std::cout << "[Демо 3] Добавляем элемент 100 в v2\n";
        v2.push_back(100);
        std::cout << "[Демо 3] Добавляем элемент 200 в v2\n";
        v2.push_back(200);
        std::cout << "[Демо 3] Добавляем элемент 300 в v2\n";
        v2.push_back(300);

        std::cout << "[Демо 3] v2: size = " << v2.get_size() << "\n";
        std::cout << "[Демо 3] v2 элементы: ";
        for (int x : v2) {
            std::cout << x << " ";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "=== Демонстрация CustomMemoryResource и PmrVector ===\n";

    CustomMemoryResource my_resource;

    demo_int_vector(my_resource);
    demo_record_vector(my_resource);
    demo_reuse_memory(my_resource);

    std::cout << "\n=== Завершение main: будет вызван деструктор CustomMemoryResource,\n"
                 "    он освободит всю оставшуюся неосвобождённую память ===\n";
    return 0;
}
