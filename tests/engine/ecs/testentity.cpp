// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testentity.cpp
 * @brief Implementation of Entity unit tests.
 */

#include "testentity.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestEntity::TestDefaultConstructor() {
    Entity obj;
    TEST_ASSERT_TRUE(obj.IsNull());
    TEST_ASSERT_FALSE(obj.IsValid());
    TEST_ASSERT_EQUAL(0, (int)obj.GetID());
}

void TestEntity::TestParameterizedConstructor() {
    Entity e(Entity::MakeID(1, 0));
    TEST_ASSERT_FALSE(e.IsNull());
    TEST_ASSERT_TRUE(e.IsValid());
    TEST_ASSERT_EQUAL(1, (int)e.GetIndex());
    TEST_ASSERT_EQUAL(0, (int)e.GetGeneration());
}

// ========== Method Tests ==========

void TestEntity::TestGetID() {
    EntityID id = Entity::MakeID(5, 3);
    Entity e(id);
    TEST_ASSERT_EQUAL((int)id, (int)e.GetID());
}

void TestEntity::TestGetIndex() {
    Entity e(Entity::MakeID(42, 7));
    TEST_ASSERT_EQUAL(42, (int)e.GetIndex());
}

void TestEntity::TestGetGeneration() {
    Entity e(Entity::MakeID(1, 99));
    TEST_ASSERT_EQUAL(99, (int)e.GetGeneration());
}

void TestEntity::TestIsNull() {
    Entity null;
    TEST_ASSERT_TRUE(null.IsNull());

    Entity valid(Entity::MakeID(1, 0));
    TEST_ASSERT_FALSE(valid.IsNull());
}

void TestEntity::TestIsValid() {
    Entity null;
    TEST_ASSERT_FALSE(null.IsValid());

    Entity valid(Entity::MakeID(1, 0));
    TEST_ASSERT_TRUE(valid.IsValid());
}

// ========== Edge Cases ==========

void TestEntity::TestEdgeCases() {
    // Equality
    Entity a(Entity::MakeID(1, 0));
    Entity b(Entity::MakeID(1, 0));
    Entity c(Entity::MakeID(2, 0));
    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_TRUE(a != c);

    // Bool conversion
    Entity null;
    TEST_ASSERT_FALSE((bool)null);
    Entity valid(Entity::MakeID(1, 0));
    TEST_ASSERT_TRUE((bool)valid);

    // MakeID encoding
    EntityID id = Entity::MakeID(0xFFFFFFFF, 0xFFFFFFFF);
    Entity maxE(id);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, (unsigned)maxE.GetIndex());
    TEST_ASSERT_EQUAL(0xFFFFFFFF, (unsigned)maxE.GetGeneration());
}

// ========== Test Runner ==========

void TestEntity::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetID);
    RUN_TEST(TestGetIndex);
    RUN_TEST(TestGetGeneration);
    RUN_TEST(TestIsNull);
    RUN_TEST(TestIsValid);
    RUN_TEST(TestEdgeCases);
}
