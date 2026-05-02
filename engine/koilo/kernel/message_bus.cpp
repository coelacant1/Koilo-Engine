#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <algorithm>

namespace koilo {

static MessageBus* g_messageBus = nullptr;

MessageBus* GetMessageBus() { return g_messageBus; }
void SetMessageBus(MessageBus* bus) { g_messageBus = bus; }

MessageBus::MessageBus(size_t ringCapacity)
    : ringCapacity_(ringCapacity) {
    ringBuffer_.resize(ringCapacity);
}

MessageBus::SubscriptionId MessageBus::Subscribe(MessageType type, Handler handler) {
    SubscriptionId id = nextSubId_++;
    subsByType_[type].push_back({id, type, false, std::move(handler)});
    ++totalSubscribers_;
    return id;
}

MessageBus::SubscriptionId MessageBus::SubscribeAll(Handler handler) {
    SubscriptionId id = nextSubId_++;
    wildcardSubs_.push_back({id, MSG_NONE, true, std::move(handler)});
    ++totalSubscribers_;
    return id;
}

void MessageBus::Unsubscribe(SubscriptionId id) {
    // Wildcard bucket first (small, hot).
    {
        auto it = std::find_if(wildcardSubs_.begin(), wildcardSubs_.end(),
            [id](const Subscription& s) { return s.id == id; });
        if (it != wildcardSubs_.end()) {
            wildcardSubs_.erase(it);
            if (totalSubscribers_) --totalSubscribers_;
            return;
        }
    }
    // Otherwise scan type buckets. Subscriptions are infrequent; this
    // rare O(unique-types + matching-bucket) scan is acceptable.
    for (auto bucketIt = subsByType_.begin(); bucketIt != subsByType_.end(); ++bucketIt) {
        auto& bucket = bucketIt->second;
        auto it = std::find_if(bucket.begin(), bucket.end(),
            [id](const Subscription& s) { return s.id == id; });
        if (it != bucket.end()) {
            bucket.erase(it);
            if (totalSubscribers_) --totalSubscribers_;
            if (bucket.empty()) subsByType_.erase(bucketIt);
            return;
        }
    }
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
    return totalSubscribers_;
}

void MessageBus::DeliverToSubscribers(const Message& msg) {
    ++totalDispatched_;
    // D1: O(bucket + wildcard) instead of O(all subs); no per-handler
    // type compare. The bucket lookup itself is O(1) average on the
    // unordered_map. Bail early when the bucket is empty.
    auto it = subsByType_.find(msg.type);
    if (it != subsByType_.end()) {
        for (auto& sub : it->second) sub.handler(msg);
    }
    for (auto& sub : wildcardSubs_) sub.handler(msg);
}

} // namespace koilo
