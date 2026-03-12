// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file undo_stack.cpp
 * @brief Command-pattern undo/redo stack implementation.
 * @date 03/09/2026
 * @author Coela
 */

#include "undo_stack.hpp"

namespace koilo {

// -- PropertyChangeCommand -------------------------------------------

PropertyChangeCommand::PropertyChangeCommand(void* instance,
                                             const FieldDecl& field,
                                             const void* newValue)
    : instance_(instance)
    , access_(field.access)
    , size_(field.size)
    , oldValue_(new uint8_t[field.size])
    , newValue_(new uint8_t[field.size]) {
    // Snapshot current value
    const void* current = access_.get_cptr(instance_);
    std::memcpy(oldValue_, current, size_);
    std::memcpy(newValue_, newValue, size_);

    name_ = "Change ";
    name_ += field.name;
}

PropertyChangeCommand::~PropertyChangeCommand() {
    delete[] oldValue_;
    delete[] newValue_;
}

void PropertyChangeCommand::Execute() {
    void* dst = access_.get_ptr(instance_);
    std::memcpy(dst, newValue_, size_);
}

void PropertyChangeCommand::Undo() {
    void* dst = access_.get_ptr(instance_);
    std::memcpy(dst, oldValue_, size_);
}

// -- UndoStack -------------------------------------------------------

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

bool UndoStack::Undo() {
    if (cursor_ == 0) return false;
    --cursor_;
    commands_[cursor_]->Undo();
    if (onChange_) onChange_();
    return true;
}

bool UndoStack::Redo() {
    if (cursor_ >= commands_.size()) return false;
    commands_[cursor_]->Execute();
    ++cursor_;
    if (onChange_) onChange_();
    return true;
}

const char* UndoStack::UndoName() const {
    if (cursor_ == 0) return "";
    return commands_[cursor_ - 1]->Name();
}

const char* UndoStack::RedoName() const {
    if (cursor_ >= commands_.size()) return "";
    return commands_[cursor_]->Name();
}

void UndoStack::Clear() {
    commands_.clear();
    cursor_ = 0;
    if (onChange_) onChange_();
}

} // namespace koilo
