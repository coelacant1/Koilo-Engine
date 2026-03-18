#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <algorithm>

namespace koilo {

MessageBus::MessageBus(size_t ringCapacity)
    : ringCapacity_(ringCapacity) {
    ringBuffer_.resize(ringCapacity);
}

MessageBus::SubscriptionId MessageBus::Subscribe(MessageType type, Handler handler) {
    SubscriptionId id = nextSubId_++;
    subscriptions_.push_back({id, type, false, std::move(handler)});
    return id;
}

MessageBus::SubscriptionId MessageBus::SubscribeAll(Handler handler) {
    SubscriptionId id = nextSubId_++;
    subscriptions_.push_back({id, MSG_NONE, true, std::move(handler)});
    return id;
}

void MessageBus::Unsubscribe(SubscriptionId id) {
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
            [id](const Subscription& s) { return s.id == id; }),
        subscriptions_.end());
}

void MessageBus::Send(const Message& msg) {
    if (ringCount_ >= ringCapacity_) {
        // Overflow: drop oldest message
        ringTail_ = (ringTail_ + 1) % ringCapacity_;
        --ringCount_;
        ++totalDropped_;
        // Rate-limited warning: only on first drop and then every 100th
        if (totalDropped_ == 1 || totalDropped_ % 100 == 0) {
            KL_WARN("MessageBus", "Ring buffer overflow - %zu message(s) dropped (capacity=%zu)",
                     totalDropped_, ringCapacity_);
        }
    }
    ringBuffer_[ringHead_] = msg;
    ringHead_ = (ringHead_ + 1) % ringCapacity_;
    ++ringCount_;
}

void MessageBus::SendImmediate(const Message& msg) {
    DeliverToSubscribers(msg);
}

void MessageBus::Dispatch() {
    // Process all pending messages. Note: handlers may enqueue new messages,
    // so we snapshot the count and process only what was pending at call time.
    size_t toProcess = ringCount_;
    for (size_t i = 0; i < toProcess; ++i) {
        const Message& msg = ringBuffer_[ringTail_];
        DeliverToSubscribers(msg);
        ringTail_ = (ringTail_ + 1) % ringCapacity_;
        --ringCount_;
    }
}

void MessageBus::FlushPending() {
    ringHead_ = 0;
    ringTail_ = 0;
    ringCount_ = 0;
}

size_t MessageBus::PendingCount() const {
    return ringCount_;
}

size_t MessageBus::SubscriberCount() const {
    return subscriptions_.size();
}

void MessageBus::DeliverToSubscribers(const Message& msg) {
    ++totalDispatched_;
    for (auto& sub : subscriptions_) {
        if (sub.wildcard || sub.type == msg.type) {
            sub.handler(msg);
        }
    }
}

} // namespace koilo
