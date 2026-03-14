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
 * @author Coela Can't
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

    /** @brief Execute (or re-execute) this command. */
    virtual void Execute() = 0;

    /** @brief Reverse the effect of Execute(). */
    virtual void Undo() = 0;

    /** @brief Human-readable name for display in undo history. */
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
    /**
     * @brief Construct a property-change command.
     * @param instance   Live object pointer.
     * @param field      FieldDecl describing the field.
     * @param newValue   Pointer to the new value (field.size bytes).
     */
    PropertyChangeCommand(void* instance, const FieldDecl& field,
                          const void* newValue);

    ~PropertyChangeCommand() override;

    /** @brief Apply the new value to the field. */
    void Execute() override;

    /** @brief Restore the old value to the field. */
    void Undo() override;

    /** @brief Return the descriptive name of this command. */
    const char* Name() const override { return name_.c_str(); }

private:
    void* instance_;           ///< Live object whose field is modified
    FieldAccess access_;       ///< Accessor for the target field
    size_t size_;              ///< Byte size of the field
    uint8_t* oldValue_;        ///< Snapshot of value before Execute()
    uint8_t* newValue_;        ///< Value written by Execute()
    std::string name_;         ///< Descriptive name shown in undo history
};

// -- UndoStack -------------------------------------------------------

/**
 * @class UndoStack
 * @brief Manages a linear command history with undo/redo cursor.
 */
class UndoStack {
public:
    static constexpr size_t DEFAULT_DEPTH = 256;

    /** @brief Construct an undo stack with the given maximum depth. */
    explicit UndoStack(size_t maxDepth = DEFAULT_DEPTH)
        : maxDepth_(maxDepth) {}

    /**
     * @brief Execute a command and push it onto the stack.
     *
     * Clears any redo history beyond the current cursor.
     */
    void Push(std::unique_ptr<Command> cmd);

    /** @brief Undo the last command.  Returns false if nothing to undo. */
    bool Undo();

    /** @brief Redo the next command.  Returns false if nothing to redo. */
    bool Redo();

    /** @brief True if there is at least one command to undo. */
    bool CanUndo() const { return cursor_ > 0; }

    /** @brief True if there is at least one command to redo. */
    bool CanRedo() const { return cursor_ < commands_.size(); }

    /** @brief Name of the command that would be undone, or "" if none. */
    const char* UndoName() const;

    /** @brief Name of the command that would be redone, or "" if none. */
    const char* RedoName() const;

    /** @brief Number of commands in the stack (including redoable ones). */
    size_t Count() const { return commands_.size(); }

    /** @brief Current cursor position (number of executed commands). */
    size_t Cursor() const { return cursor_; }

    /** @brief Clear all history. */
    void Clear();

    /** @brief Set callback invoked after any modification. */
    void SetOnChange(UndoRedoCallback cb) { onChange_ = std::move(cb); }

private:
    std::vector<std::unique_ptr<Command>> commands_; ///< Linear command history
    size_t cursor_ = 0;       ///< Index of the next command to redo
    size_t maxDepth_;          ///< Maximum number of commands retained
    UndoRedoCallback onChange_; ///< Callback fired after any state change
};

} // namespace koilo
