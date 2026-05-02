// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file message_bus.hpp
 * @brief Zero-copy in-process message bus with ring-buffered delivery.
 *
 * @date 11/08/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/message_types.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include "../registry/reflect_macros.hpp"

namespace koilo {

/// Zero-copy in-process message bus with ring-buffered deferred delivery.
/// Supports broadcast (by type) and synchronous request-response.
class MessageBus {
public:
    using Handler = std::function<void(const Message&)>;
    using SubscriptionId = uint32_t;

    explicit MessageBus(size_t ringCapacity = 1024);
    ~MessageBus() = default;

    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;

    /// Subscribe to all messages of a given type.
    SubscriptionId Subscribe(MessageType type, Handler handler);

    /// Subscribe to ALL message types (wildcard).
    SubscriptionId SubscribeAll(Handler handler);

    /// Remove a subscription.
    void Unsubscribe(SubscriptionId id);

    /// Queue a message for deferred delivery on next Dispatch().
    void Send(const Message& msg);

    /// Deliver a message immediately to all matching subscribers (synchronous).
    /// Use sparingly - mainly for console request-response.
    void SendImmediate(const Message& msg);

    /// Deliver all queued messages to subscribers. Call once per frame.
    void Dispatch();

    /// Discard all pending messages without delivering.
    void FlushPending();

    size_t PendingCount() const;
    size_t SubscriberCount() const;
    size_t TotalDispatched() const { return totalDispatched_; }
    size_t TotalDropped() const    { return totalDropped_; }

    /// Fast O(1) check for whether any handler would receive a message of
    /// this type (counts both type-specific and wildcard subscribers).
    /// Use this before constructing/sending broadcast messages on the hot
    /// path, e.g. MSG_FRAME_BEGIN/END (F1).
    bool HasSubscribers(MessageType type) const {
        if (!wildcardSubs_.empty()) return true;
        auto it = subsByType_.find(type);
        return it != subsByType_.end() && !it->second.empty();
    }

private:
    struct Subscription {
        SubscriptionId id;
        MessageType    type;
        bool           wildcard;
        Handler        handler;

        KL_BEGIN_FIELDS(Subscription)
            KL_FIELD(Subscription, id, "Id", 0, 0),
            KL_FIELD(Subscription, type, "Type", 0, 0),
            KL_FIELD(Subscription, wildcard, "Wildcard", 0, 1),
            KL_FIELD(Subscription, handler, "Handler", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Subscription)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Subscription)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Subscription)

    };

    void DeliverToSubscribers(const Message& msg);

    // D1: bucketed subscription storage. Dispatch looks up a single
    // bucket per message type instead of scanning every subscription
    // and type-comparing on each one. typeSubCounts_ is folded into
    // bucket sizes; HasSubscribers becomes one map lookup + a vector
    // size check.
    std::unordered_map<MessageType, std::vector<Subscription>> subsByType_;
    std::vector<Subscription> wildcardSubs_;
    std::vector<Message>      ringBuffer_;
    size_t                    ringHead_ = 0;
    size_t                    ringTail_ = 0;
    size_t                    ringCount_ = 0;
    size_t                    ringCapacity_;
    SubscriptionId            nextSubId_ = 1;
    size_t                    totalDispatched_ = 0;
    size_t                    totalDropped_ = 0;
    size_t                    totalSubscribers_ = 0;

    KL_BEGIN_FIELDS(MessageBus)
        KL_FIELD(MessageBus, nextSubId_, "Next subscription id", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MessageBus)
        KL_METHOD_AUTO(MessageBus, PendingCount, "Pending count"),
        KL_METHOD_AUTO(MessageBus, SubscriberCount, "Subscriber count"),
        KL_METHOD_AUTO(MessageBus, TotalDispatched, "Total dispatched"),
        KL_METHOD_AUTO(MessageBus, TotalDropped, "Total dropped")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MessageBus)
        KL_CTOR0(MessageBus),
        KL_CTOR(MessageBus, size_t)
    KL_END_DESCRIBE(MessageBus)

};

} // namespace koilo

namespace koilo {
/// Global accessor for MessageBus (set by kernel on init, cleared on shutdown).
MessageBus* GetMessageBus();
void SetMessageBus(MessageBus* bus);
} // namespace koilo
