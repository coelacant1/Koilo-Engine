// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testundostack.cpp
 * @brief Tests for the undo/redo command stack.
 * @date 03/09/2026
 * @author Coela
 */

#include "testundostack.hpp"
#include <cstring>

// -- Simple concrete command for testing -----------------------------

class IncrementCommand : public koilo::Command {
public:
    IncrementCommand(int& val) : val_(val) {}
    void Execute() override { val_++; }
    void Undo() override { val_--; }
    const char* Name() const override { return "Increment"; }
private:
    int& val_;
};

// -- Reflected test struct for PropertyChangeCommand -----------------

struct UndoTestObj {
    float health = 100.0f;
    int   score  = 0;

    static koilo::FieldList Fields() {
        static koilo::FieldDecl fields[] = {
            KL_FIELD(UndoTestObj, health, "Health", 0.0f, 200.0f),
            KL_FIELD(UndoTestObj, score,  "Score", 0, 1000)
        };
        return {fields, 2};
    }
    static koilo::MethodList Methods() { return {nullptr, 0}; }

    KL_BEGIN_DESCRIBE(UndoTestObj)
        KL_CTOR0(UndoTestObj)
    KL_END_DESCRIBE(UndoTestObj)
};

// -- Tests -----------------------------------------------------------

void TestUndoStack::TestBasicPushUndoRedo() {
    int val = 0;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_INT(1, val);
    stack.Undo();
    TEST_ASSERT_EQUAL_INT(0, val);
    stack.Redo();
    TEST_ASSERT_EQUAL_INT(1, val);
}

void TestUndoStack::TestCanUndoRedoStates() {
    int val = 0;
    koilo::UndoStack stack;
    TEST_ASSERT_FALSE(stack.CanUndo());
    TEST_ASSERT_FALSE(stack.CanRedo());
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_TRUE(stack.CanUndo());
    TEST_ASSERT_FALSE(stack.CanRedo());
    stack.Undo();
    TEST_ASSERT_FALSE(stack.CanUndo());
    TEST_ASSERT_TRUE(stack.CanRedo());
}

void TestUndoStack::TestRedoClearedOnPush() {
    int val = 0;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<IncrementCommand>(val));
    stack.Push(std::make_unique<IncrementCommand>(val));
    stack.Undo();
    TEST_ASSERT_TRUE(stack.CanRedo());
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_FALSE(stack.CanRedo());
}

void TestUndoStack::TestMultipleUndos() {
    int val = 0;
    koilo::UndoStack stack;
    for (int i = 0; i < 5; ++i)
        stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_INT(5, val);
    for (int i = 0; i < 5; ++i) stack.Undo();
    TEST_ASSERT_EQUAL_INT(0, val);
    TEST_ASSERT_FALSE(stack.Undo());
}

void TestUndoStack::TestCommandNames() {
    int val = 0;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_STRING("Increment", stack.UndoName());
    stack.Undo();
    TEST_ASSERT_EQUAL_STRING("Increment", stack.RedoName());
    TEST_ASSERT_EQUAL_STRING("", stack.UndoName());
}

void TestUndoStack::TestMaxDepth() {
    int val = 0;
    koilo::UndoStack stack(3);
    for (int i = 0; i < 5; ++i)
        stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_UINT(3, static_cast<unsigned>(stack.Count()));
    stack.Undo(); stack.Undo(); stack.Undo();
    TEST_ASSERT_EQUAL_INT(2, val);
}

void TestUndoStack::TestClear() {
    int val = 0;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<IncrementCommand>(val));
    stack.Push(std::make_unique<IncrementCommand>(val));
    stack.Clear();
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(stack.Count()));
    TEST_ASSERT_FALSE(stack.CanUndo());
}

void TestUndoStack::TestOnChangeCallback() {
    int val = 0;
    int changeCount = 0;
    koilo::UndoStack stack;
    stack.SetOnChange([&changeCount]() { changeCount++; });
    stack.Push(std::make_unique<IncrementCommand>(val));
    stack.Undo();
    stack.Redo();
    stack.Clear();
    TEST_ASSERT_EQUAL_INT(4, changeCount);
}

void TestUndoStack::TestPropertyChangeCommand() {
    UndoTestObj obj;
    auto fields = UndoTestObj::Fields();
    float newHealth = 75.0f;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<koilo::PropertyChangeCommand>(
        &obj, fields.data[0], &newHealth));
    TEST_ASSERT_EQUAL_FLOAT(75.0f, obj.health);
    stack.Undo();
    TEST_ASSERT_EQUAL_FLOAT(100.0f, obj.health);
    stack.Redo();
    TEST_ASSERT_EQUAL_FLOAT(75.0f, obj.health);
}

void TestUndoStack::TestPropertyChangeName() {
    UndoTestObj obj;
    auto fields = UndoTestObj::Fields();
    int newScore = 42;
    koilo::UndoStack stack;
    stack.Push(std::make_unique<koilo::PropertyChangeCommand>(
        &obj, fields.data[1], &newScore));
    TEST_ASSERT_NOT_NULL(strstr(stack.UndoName(), "score"));
}

void TestUndoStack::TestCursorTracking() {
    int val = 0;
    koilo::UndoStack stack;
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(stack.Cursor()));
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(stack.Cursor()));
    stack.Push(std::make_unique<IncrementCommand>(val));
    TEST_ASSERT_EQUAL_UINT(2, static_cast<unsigned>(stack.Cursor()));
    stack.Undo();
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(stack.Cursor()));
}

void TestUndoStack::RunAllTests() {
    RUN_TEST(TestUndoStack::TestBasicPushUndoRedo);
    RUN_TEST(TestUndoStack::TestCanUndoRedoStates);
    RUN_TEST(TestUndoStack::TestRedoClearedOnPush);
    RUN_TEST(TestUndoStack::TestMultipleUndos);
    RUN_TEST(TestUndoStack::TestCommandNames);
    RUN_TEST(TestUndoStack::TestMaxDepth);
    RUN_TEST(TestUndoStack::TestClear);
    RUN_TEST(TestUndoStack::TestOnChangeCallback);
    RUN_TEST(TestUndoStack::TestPropertyChangeCommand);
    RUN_TEST(TestUndoStack::TestPropertyChangeName);
    RUN_TEST(TestUndoStack::TestCursorTracking);
}
