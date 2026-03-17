#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

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
    registry.Register({"tap", "tap <type_id>", "Subscribe console to a message type (live stream)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) {
                // Tap all
                kernel.Messages().SubscribeAll([](const Message& msg) {
                    std::fprintf(stderr, "[tap] %s (type=%u) src=%u size=%u\n",
                                 MessageTypeName(msg.type), msg.type, msg.source, msg.size);
                });
                return ConsoleResult::Ok("Tapping all message types (output to stderr).");
            }
            MessageType type;
            try { type = static_cast<MessageType>(std::stoul(args[0])); }
            catch (...) { return ConsoleResult::Error("Invalid type: " + args[0]); }

            kernel.Messages().Subscribe(type, [type](const Message& msg) {
                std::fprintf(stderr, "[tap:%s] src=%u size=%u\n",
                             MessageTypeName(type), msg.source, msg.size);
            });
            return ConsoleResult::Ok("Tapping message type " + std::to_string(type) + ".");
        }, nullptr
    });
}

} // namespace koilo
