// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file undo_stack.cpp
 * @brief Command-pattern undo/redo stack implementation.
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "undo_stack.hpp"

namespace koilo {

// ====================================================================
// PropertyChangeCommand
// ====================================================================

// Construct and snapshot the current field value.
PropertyChangeCommand::PropertyChangeCommand(void* instance,
                                             const FieldDecl& field,
                                             const void* newValue)
    : instance_(instance)
    , access_(field.access)
    , size_(field.size)
    , oldValue_(field.size)
    , newValue_(field.size) {
    // Snapshot current value
    const void* current = access_.get_cptr(instance_);
    std::memcpy(oldValue_.data(), current, size_);
    std::memcpy(newValue_.data(), newValue, size_);

    name_ = "Change ";
    name_ += field.name;
}

// Apply the new value to the target field.
void PropertyChangeCommand::Execute() {
    void* dst = access_.get_ptr(instance_);
    std::memcpy(dst, newValue_.data(), size_);
}

// Restore the old value to the target field.
void PropertyChangeCommand::Undo() {
    void* dst = access_.get_ptr(instance_);
    std::memcpy(dst, oldValue_.data(), size_);
}

// ====================================================================
// UndoStack
// ====================================================================

// Execute a command, trim redo tail, and enforce max depth.
void UndoStack::Push(std::unique_ptr<Command> cmd) {
    // Execute the command
    cmd->Execute();

    // Trim redo tail
    if (cursor_ < commands_.size()) {
        commands_.erase(commands_.begin() + static_cast<ptrdiff_t>(cursor_),
                        commands_.end());
    }

    commands_.push_back(std::move(cmd));
    cursor_ = commands_.size();

    // Enforce max depth
    if (commands_.size() > maxDepth_) {
        size_t excess = commands_.size() - maxDepth_;
        commands_.erase(commands_.begin(),
                        commands_.begin() + static_cast<ptrdiff_t>(excess));
        cursor_ -= excess;
    }

    if (onChange_) onChange_();
}

// Step the cursor back and undo the command.
bool UndoStack::Undo() {
    if (cursor_ == 0) return false;
    --cursor_;
    commands_[cursor_]->Undo();
    if (onChange_) onChange_();
    return true;
}

// Re-execute the command at the cursor and advance.
bool UndoStack::Redo() {
    if (cursor_ >= commands_.size()) return false;
    commands_[cursor_]->Execute();
    ++cursor_;
    if (onChange_) onChange_();
    return true;
}

// Return the name of the command that would be undone.
const char* UndoStack::UndoName() const {
    if (cursor_ == 0) return "";
    return commands_[cursor_ - 1]->Name();
}

// Return the name of the command that would be redone.
const char* UndoStack::RedoName() const {
    if (cursor_ >= commands_.size()) return "";
    return commands_[cursor_]->Name();
}

// Discard all commands and reset the cursor.
void UndoStack::Clear() {
    commands_.clear();
    cursor_ = 0;
    if (onChange_) onChange_();
}

} // namespace koilo
