#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <algorithm>

namespace koilo {

// Track active tap subscriptions so they can be removed.
static std::mutex s_tapMutex;
static std::vector<MessageBus::SubscriptionId> s_tapSubs;

void RegisterMessageCommands(CommandRegistry& registry) {
    // -- send --
    registry.Register({"send", "send <type_id>", "Send a message on the bus (type as integer)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) return ConsoleResult::Error("Usage: send <type_id>");

            MessageType type;
            try { type = static_cast<MessageType>(std::stoul(args[0])); }
            catch (...) { return ConsoleResult::Error("Invalid message type (must be integer): " + args[0]); }

            kernel.Messages().Send(MakeSignal(type, MODULE_KERNEL));
            return ConsoleResult::Ok("Sent message type=" + std::to_string(type));
        }, nullptr
    });

    // -- flush --
    registry.Register({"flush", "flush", "Flush pending messages without delivering",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            size_t count = kernel.Messages().PendingCount();
            kernel.Messages().FlushPending();
            return ConsoleResult::Ok("Flushed " + std::to_string(count) + " pending messages.");
        }, nullptr
    });

    // -- dispatch --
    registry.Register({"dispatch", "dispatch", "Dispatch all pending messages now",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            size_t count = kernel.Messages().PendingCount();
            kernel.Messages().Dispatch();
            return ConsoleResult::Ok("Dispatched " + std::to_string(count) + " messages.");
        }, nullptr
    });

    // -- tap --
    registry.Register({"tap", "tap [type_id]", "Subscribe console to a message type (live stream)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            MessageBus::SubscriptionId id;
            if (args.empty()) {
                id = kernel.Messages().SubscribeAll([](const Message& msg) {
                    KL_DBG("tap", "%s (type=%u) src=%u size=%u",
                           MessageTypeName(msg.type), msg.type, msg.source, msg.size);
                });
                std::lock_guard<std::mutex> lock(s_tapMutex);
                s_tapSubs.push_back(id);
                return ConsoleResult::Ok("Tapping all message types (id=" + std::to_string(id) + "). Use 'untap' to stop.");
            }
            MessageType type;
            try { type = static_cast<MessageType>(std::stoul(args[0])); }
            catch (...) { return ConsoleResult::Error("Invalid type: " + args[0]); }

            id = kernel.Messages().Subscribe(type, [type](const Message& msg) {
                KL_DBG("tap", "[%s] src=%u size=%u",
                       MessageTypeName(type), msg.source, msg.size);
            });
            std::lock_guard<std::mutex> lock(s_tapMutex);
            s_tapSubs.push_back(id);
            return ConsoleResult::Ok("Tapping type " + std::to_string(type) + " (id=" + std::to_string(id) + "). Use 'untap' to stop.");
        }, nullptr
    });

    // -- untap --
    registry.Register({"untap", "untap [id]", "Remove tap subscriptions (all or by id)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            std::lock_guard<std::mutex> lock(s_tapMutex);
            if (args.empty()) {
                size_t count = s_tapSubs.size();
                for (auto subId : s_tapSubs)
                    kernel.Messages().Unsubscribe(subId);
                s_tapSubs.clear();
                return ConsoleResult::Ok("Removed " + std::to_string(count) + " tap subscription(s).");
            }
            MessageBus::SubscriptionId target;
            try { target = static_cast<MessageBus::SubscriptionId>(std::stoul(args[0])); }
            catch (...) { return ConsoleResult::Error("Invalid id: " + args[0]); }

            auto it = std::find(s_tapSubs.begin(), s_tapSubs.end(), target);
            if (it == s_tapSubs.end())
                return ConsoleResult::Error("No tap with id=" + std::to_string(target));
            kernel.Messages().Unsubscribe(target);
            s_tapSubs.erase(it);
            return ConsoleResult::Ok("Removed tap id=" + std::to_string(target) + ".");
        }, nullptr
    });
}

} // namespace koilo
