// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file undo_stack.hpp
 * @brief Command-pattern undo/redo stack for editor operations.
 *
 * Commands are polymorphic objects that know how to Execute() and Undo().
 * The UndoStack owns commands and manages the undo/redo cursor.
 * PropertyChangeCommand is a reflection-aware command that modifies
 * any registered field via FieldDecl accessors.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <koilo/registry/registry.hpp>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

namespace koilo {

/// Callback invoked after any undo/redo/execute.
using UndoRedoCallback = std::function<void()>;

// -- Command base ----------------------------------------------------

/**
 * @class Command
 * @brief Abstract undoable operation.
 */
class Command {
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* Name() const = 0;
};

// -- PropertyChangeCommand -------------------------------------------

/**
 * @class PropertyChangeCommand
 * @brief Changes a single reflected field value, storing old/new for undo.
 *
 * Works with any FieldDecl by memcpy-ing the raw bytes.
 */
class PropertyChangeCommand : public Command {
public:
    /// @param instance   Live object pointer
    /// @param field      FieldDecl describing the field
    /// @param newValue   Pointer to the new value (field.size bytes)
    PropertyChangeCommand(void* instance, const FieldDecl& field,
                          const void* newValue);

    ~PropertyChangeCommand() override;

    void Execute() override;
    void Undo() override;
    const char* Name() const override { return name_.c_str(); }

private:
    void* instance_;
    FieldAccess access_;
    size_t size_;
    uint8_t* oldValue_;
    uint8_t* newValue_;
    std::string name_;
};

// -- UndoStack -------------------------------------------------------

/**
 * @class UndoStack
 * @brief Manages a linear command history with undo/redo cursor.
 */
class UndoStack {
public:
    static constexpr size_t DEFAULT_DEPTH = 256;

    explicit UndoStack(size_t maxDepth = DEFAULT_DEPTH)
        : maxDepth_(maxDepth) {}

    /// Execute a command and push it onto the stack.
    /// Clears any redo history beyond the current cursor.
    void Push(std::unique_ptr<Command> cmd);

    /// Undo the last command.  Returns false if nothing to undo.
    bool Undo();

    /// Redo the next command.  Returns false if nothing to redo.
    bool Redo();

    /// True if there is at least one command to undo.
    bool CanUndo() const { return cursor_ > 0; }

    /// True if there is at least one command to redo.
    bool CanRedo() const { return cursor_ < commands_.size(); }

    /// Name of the command that would be undone, or "" if none.
    const char* UndoName() const;

    /// Name of the command that would be redone, or "" if none.
    const char* RedoName() const;

    /// Number of commands in the stack (including redoable ones).
    size_t Count() const { return commands_.size(); }

    /// Current cursor position (number of executed commands).
    size_t Cursor() const { return cursor_; }

    /// Clear all history.
    void Clear();

    /// Set callback invoked after any modification.
    void SetOnChange(UndoRedoCallback cb) { onChange_ = std::move(cb); }

private:
    std::vector<std::unique_ptr<Command>> commands_;
    size_t cursor_ = 0;
    size_t maxDepth_;
    UndoRedoCallback onChange_;
};

} // namespace koilo
