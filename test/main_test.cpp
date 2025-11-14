#include <gtest/gtest.h>
#include <string>
#include <memory_resource>

#include "../include/pmr_vector.h"

struct Record {
    int         id;
    double      value;
    std::string label;
};

using IntAlloc = std::pmr::polymorphic_allocator<int>;
using RecordAlloc = std::pmr::polymorphic_allocator<Record>;

using IntVector = PmrVector<int, IntAlloc>;
using RecordVector = PmrVector<Record, RecordAlloc>;

//----------------------------------------------------------
// Тест 1: базовые операции с PmrVector<int>
//----------------------------------------------------------
TEST(PmrVectorIntTest, PushBackAndAccess) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };

    IntVector vec(0, alloc);

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.get_size(), 0u);

    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.get_size(), 3u);

    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 30);

    EXPECT_EQ(vec.front(), 10);
    EXPECT_EQ(vec.back(), 30);
}

//----------------------------------------------------------
// Тест 2: emplace_back и работа со сложным типом
//----------------------------------------------------------
TEST(PmrVectorRecordTest, EmplaceBackAndIterate) {
    CustomMemoryResource mr;
    RecordAlloc alloc{ &mr };

    RecordVector vec(1, alloc); // маленькая capacity, чтобы проверить рост

    vec.emplace_back(1, 3.14,  "one");
    vec.emplace_back(2, 2.71,  "two");
    vec.emplace_back(3, 1.41,  "three");

    EXPECT_EQ(vec.get_size(), 3u);

    // Проверяем значения
    EXPECT_EQ(vec[0].id, 1);
    EXPECT_DOUBLE_EQ(vec[0].value, 3.14);
    EXPECT_EQ(vec[0].label, "one");

    EXPECT_EQ(vec[1].id, 2);
    EXPECT_DOUBLE_EQ(vec[1].value, 2.71);
    EXPECT_EQ(vec[1].label, "two");

    EXPECT_EQ(vec[2].id, 3);
    EXPECT_DOUBLE_EQ(vec[2].value, 1.41);
    EXPECT_EQ(vec[2].label, "three");

    // Обход через итератор (ArrayIterator)
    int sum_ids = 0;
    for (RecordVector::iterator it = vec.begin(); it != vec.end(); ++it) {
        sum_ids += it->id;
    }
    EXPECT_EQ(sum_ids, 1 + 2 + 3);
}

//----------------------------------------------------------
// Тест 3: clear() сбрасывает size и позволяет дальше использовать вектор
//----------------------------------------------------------
TEST(PmrVectorIntTest, ClearAndReuse) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };
    IntVector vec(4, alloc);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_EQ(vec.get_size(), 3u);
    EXPECT_FALSE(vec.empty());

    vec.clear();
    EXPECT_EQ(vec.get_size(), 0u);
    EXPECT_TRUE(vec.empty());

    // После clear можно снова добавлять элементы
    vec.push_back(42);
    vec.push_back(17);

    EXPECT_EQ(vec.get_size(), 2u);
    EXPECT_EQ(vec.front(), 42);
    EXPECT_EQ(vec.back(), 17);
}

//----------------------------------------------------------
// Тест 4: итератор проходит по всем элементам в правильном порядке
//----------------------------------------------------------
TEST(PmrVectorIntTest, IteratorTraversal) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };
    IntVector vec(0, alloc);

    for (int i = 0; i < 5; ++i) {
        vec.push_back(i * 10);
    }

    int expected = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        EXPECT_EQ(*it, expected);
        expected += 10;
    }
    EXPECT_EQ(expected, 50); // проверим, что было 5 элементов
}

//----------------------------------------------------------
// Тест 5: operator[] и front/back на пустом векторе бросают исключение
//----------------------------------------------------------
TEST(PmrVectorIntTest, OutOfRangeAndEmptyFrontBack) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };
    IntVector vec(0, alloc);

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.get_size(), 0u);

    EXPECT_THROW(vec.front(), std::out_of_range);
    EXPECT_THROW(vec.back(),  std::out_of_range);
    EXPECT_THROW(vec[0],      std::out_of_range);

    vec.push_back(10);
    EXPECT_NO_THROW(vec.front());
    EXPECT_NO_THROW(vec.back());
    EXPECT_NO_THROW(vec[0]);

    EXPECT_THROW(vec[1], std::out_of_range);
}

//----------------------------------------------------------
// Тест 6: рост capacity и сохранность значений при перераспределении
//----------------------------------------------------------
TEST(PmrVectorIntTest, ReallocationKeepsValues) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };
    IntVector vec(1, alloc);  // начальная capacity = 1

    for (int i = 0; i < 10; ++i) {
        vec.push_back(i);
    }

    EXPECT_EQ(vec.get_size(), 10u);

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(vec[static_cast<std::size_t>(i)], i);
    }
}

//----------------------------------------------------------
// Тест 7: базовая проверка работы CustomMemoryResource через allocator
//----------------------------------------------------------
TEST(CustomMemoryResourceTest, AllocateAndDeallocateThroughPolymorphicAllocator) {
    CustomMemoryResource mr;
    IntAlloc alloc{ &mr };

    int* p = alloc.allocate(5);
    alloc.construct(p + 0, 10);
    alloc.construct(p + 1, 20);

    EXPECT_EQ(p[0], 10);
    EXPECT_EQ(p[1], 20);

    std::allocator_traits<IntAlloc>::destroy(alloc, p + 0);
    std::allocator_traits<IntAlloc>::destroy(alloc, p + 1);
    alloc.deallocate(p, 5);
}

//----------------------------------------------------------
// main для запуска gtest (если не используешь gtest_main)
//----------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
