#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/console/command_provider.hpp>
#include <koilo/kernel/console/event_bridge.hpp>
#include <koilo/systems/ui/console_widget.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace koilo {

std::unique_ptr<ConsoleModule> ConsoleModule::instance_;

ConsoleModule::~ConsoleModule() = default;

const ModuleDesc& ConsoleModule::GetModuleDesc() {
    static ModuleDesc desc{};
    desc.name = "koilo.console";
    desc.version = KL_VERSION(0, 1, 0);
    desc.requiredCaps = Cap::Debug | Cap::ConsoleAdmin;
    desc.Init = &ConsoleModule::Init;
    desc.Tick = &ConsoleModule::Tick;
    desc.OnMessage = &ConsoleModule::OnMessage;
    desc.Shutdown = &ConsoleModule::Shutdown;
    return desc;
}

bool ConsoleModule::Init(KoiloKernel& kernel) {
    instance_ = std::make_unique<ConsoleModule>();
    instance_->kernel_ = &kernel;
    instance_->session_ = std::make_unique<ConsoleSession>(kernel, instance_->commands_);
    instance_->RegisterBuiltinCommands();

    // Create event bridge for TCP event subscription
    instance_->eventBridge_ = std::make_unique<EventBridge>();
    instance_->eventBridge_->ConnectToMessageBus(kernel.Messages());
    kernel.Services().Register("events", instance_->eventBridge_.get());

    kernel.Services().Register("console", instance_.get());
    return true;
}

void ConsoleModule::Tick(float /*dt*/) {
    if (instance_ && !instance_->providersDiscovered_) {
        instance_->DiscoverProviders();
    }
    // Update the console widget if it has been built
    if (instance_ && instance_->widget_ && instance_->widget_->IsBuilt()) {
        instance_->widget_->Update();
    }
}

void ConsoleModule::OnMessage(const Message& /*msg*/) {
    // Future: log messages, support tapping
}

void ConsoleModule::Shutdown() {
    if (instance_) {
        if (instance_->socket_) {
            instance_->socket_->Stop();
        }
        if (instance_->eventBridge_ && instance_->kernel_) {
            instance_->eventBridge_->DisconnectFromMessageBus(instance_->kernel_->Messages());
            instance_->kernel_->Services().Unregister("events");
        }
        if (instance_->kernel_) {
            instance_->kernel_->Services().Unregister("console");
        }
        instance_.reset();
    }
}

ConsoleResult ConsoleModule::Execute(const std::string& input) {
    return session_->Execute(input);
}

void ConsoleModule::DiscoverProviders() {
    providersDiscovered_ = true;
    if (!kernel_) return;

    auto names = kernel_->Services().ListByPrefix("commands.");
    for (const auto& name : names) {
        auto* provider = kernel_->Services().Get<ICommandProvider>(name);
        if (!provider) continue;
        auto cmds = provider->GetCommands();
        for (auto& def : cmds) {
            commands_.Register(std::move(def));
        }
        KL_DBG("ConsoleModule", "Loaded commands from provider: %s", name.c_str());
    }
}

IConsoleWidget* ConsoleModule::Widget() {
    return widget_.get();
}

bool ConsoleModule::StartSocket(uint16_t port) {
    if (!socket_) {
        socket_ = std::make_unique<ConsoleSocket>(*kernel_, commands_);
        socket_->SetPool(&kernel_->Pool());
    }
    return socket_->Start(port);
}

void ConsoleModule::StopSocket() {
    if (socket_) {
        socket_->Stop();
    }
}

void ConsoleModule::BuildWidget(UI& ui, float x, float y, float w, float h) {
    if (!kernel_) return;
    if (!widget_) widget_ = std::make_unique<ConsoleWidget>();
    widget_->Build(ui, *kernel_, commands_, x, y, w, h);
}

bool ConsoleModule::HandleKey(bool isReturn, bool isToggle) {
    if (isToggle && widget_ && widget_->IsBuilt()) {
        widget_->Toggle();
        return true;
    }
    if (isReturn && widget_ && widget_->IsBuilt() && widget_->IsVisible()) {
        widget_->SubmitInput();
        return true;
    }
    return false;
}

// --- Built-in Commands -------------------------------------------------------

static std::string FormatBytes(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        return ss.str();
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
    return ss.str();
}

void ConsoleModule::RegisterBuiltinCommands() {
    // -- help --
    commands_.Register({"help", "help [command]", "Show available commands or help for a specific command",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (!args.empty()) {
                auto* cmd = commands_.Find(args[0]);
                if (!cmd) return ConsoleResult::NotFound("Unknown command: " + args[0]);
                return ConsoleResult::Ok(cmd->name + " - " + cmd->description + "\nUsage: " + cmd->usage);
            }
            std::ostringstream ss;
            ss << "Available commands:\n";
            auto names = commands_.ListCommands();
            size_t maxLen = 0;
            for (auto& n : names) maxLen = std::max(maxLen, n.size());
            for (auto& n : names) {
                auto* cmd = commands_.Find(n);
                ss << "  " << std::left << std::setw(static_cast<int>(maxLen + 2)) << n;
                if (cmd) ss << cmd->description;
                ss << "\n";
            }
            ss << "\nType 'help <command>' for detailed usage.";
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });

    // -- echo --
    commands_.Register({"echo", "echo <text...>", "Echo text back (useful for testing)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            std::string out;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) out += ' ';
                out += args[i];
            }
            return ConsoleResult::Ok(out);
        }, nullptr
    });

    // -- version --
    commands_.Register({"version", "version", "Show kernel version info",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            return ConsoleResult::Ok("Koilo Engine v0.3.2\nKernel v0.1.0");
        }, nullptr
    });

    // -- modules --
    commands_.Register({"modules", "modules", "List loaded kernel modules",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto mods = kernel.Modules().ListModules();
            if (mods.empty()) return ConsoleResult::Ok("No modules loaded.");
            std::ostringstream ss;
            ss << "Loaded modules (" << mods.size() << "):\n";
            for (auto& m : mods) {
                const char* stateStr = "?";
                switch (m.state) {
                    case ModuleState::Registered:   stateStr = "registered"; break;
                    case ModuleState::Resolving:     stateStr = "resolving"; break;
                    case ModuleState::Initialized:   stateStr = "initialized"; break;
                    case ModuleState::Running:       stateStr = "running"; break;
                    case ModuleState::ShuttingDown:  stateStr = "shutting-down"; break;
                    case ModuleState::Unloaded:      stateStr = "unloaded"; break;
                    case ModuleState::Error:         stateStr = "error"; break;
                }
                ss << "  " << m.name << " v"
                   << (int)KL_VERSION_MAJOR(m.version) << "."
                   << (int)KL_VERSION_MINOR(m.version) << "."
                   << (int)KL_VERSION_PATCH(m.version)
                   << " [" << stateStr << "]\n";
            }

            // JSON
            std::ostringstream js;
            js << "[";
            for (size_t i = 0; i < mods.size(); ++i) {
                if (i > 0) js << ",";
                js << "{\"name\":\"" << mods[i].name
                   << "\",\"id\":" << mods[i].id
                   << ",\"version\":" << mods[i].version
                   << "}";
            }
            js << "]";
            return ConsoleResult::Ok(ss.str(), js.str());
        }, nullptr
    });

    // -- services --
    commands_.Register({"services", "services", "List registered kernel services",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto list = kernel.Services().List();
            if (list.empty()) return ConsoleResult::Ok("No services registered.");
            std::sort(list.begin(), list.end());
            std::ostringstream ss;
            ss << "Registered services (" << list.size() << "):\n";
            for (auto& name : list) {
                ss << "  " << name << "\n";
            }
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });

    // -- mem --
    commands_.Register({"mem", "mem", "Show kernel memory allocator status",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto& arena = kernel.FrameArena();
            auto& scratch = kernel.Scratch();

            std::ostringstream ss;
            ss << "Kernel Memory:\n";
            ss << "  Frame Arena:  " << FormatBytes(arena.Used()) << " / "
               << FormatBytes(arena.Capacity()) << " used\n";
            ss << "  Scratch:      " << FormatBytes(scratch.Used()) << " / "
               << FormatBytes(scratch.Capacity()) << " used\n";

            std::ostringstream js;
            js << "{\"frame_arena\":{\"used\":" << arena.Used()
               << ",\"capacity\":" << arena.Capacity()
               << "},\"scratch\":{\"used\":" << scratch.Used()
               << ",\"capacity\":" << scratch.Capacity() << "}}";
            return ConsoleResult::Ok(ss.str(), js.str());
        }, nullptr
    });

    // -- messages --
    commands_.Register({"messages", "messages", "Show message bus statistics",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto& bus = kernel.Messages();
            std::ostringstream ss;
            ss << "Message Bus:\n";
            ss << "  Pending:      " << bus.PendingCount() << "\n";
            ss << "  Subscribers:  " << bus.SubscriberCount() << "\n";
            ss << "  Dispatched:   " << bus.TotalDispatched() << "\n";
            ss << "  Dropped:      " << bus.TotalDropped() << "\n";

            std::ostringstream js;
            js << "{\"pending\":" << bus.PendingCount()
               << ",\"subscribers\":" << bus.SubscriberCount()
               << ",\"dispatched\":" << bus.TotalDispatched()
               << ",\"dropped\":" << bus.TotalDropped() << "}";
            return ConsoleResult::Ok(ss.str(), js.str());
        }, nullptr
    });

    // -- history --
    commands_.Register({"history", "history [n]", "Show command history (last n entries)",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto& hist = session_->History();
            size_t count = hist.size();
            if (!args.empty()) {
                try { count = std::min(count, static_cast<size_t>(std::stoul(args[0]))); }
                catch (...) {}
            }
            std::ostringstream ss;
            size_t start = hist.size() > count ? hist.size() - count : 0;
            for (size_t i = start; i < hist.size(); ++i) {
                ss << "  " << (i + 1) << ". " << hist[i] << "\n";
            }
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });

    // -- alias --
    commands_.Register({"alias", "alias [name] [command...]", "Create/list/remove command aliases",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) {
                auto& aliases = commands_.Aliases();
                if (aliases.empty()) return ConsoleResult::Ok("No aliases defined.");
                std::ostringstream ss;
                for (auto& [name, expansion] : aliases) {
                    ss << "  " << name << " = " << expansion << "\n";
                }
                return ConsoleResult::Ok(ss.str());
            }
            if (args.size() == 1) {
                commands_.RemoveAlias(args[0]);
                return ConsoleResult::Ok("Alias '" + args[0] + "' removed.");
            }
            std::string expansion;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) expansion += ' ';
                expansion += args[i];
            }
            commands_.SetAlias(args[0], expansion);
            return ConsoleResult::Ok("Alias '" + args[0] + "' = '" + expansion + "'");
        }, nullptr
    });

    // -- output --
    commands_.Register({"output", "output [text|json]", "Get/set output format",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) {
                return ConsoleResult::Ok(
                    session_->GetOutputFormat() == ConsoleOutputFormat::Json ? "json" : "text");
            }
            if (args[0] == "json") {
                session_->SetOutputFormat(ConsoleOutputFormat::Json);
                return ConsoleResult::Ok("Output format: json");
            }
            if (args[0] == "text") {
                session_->SetOutputFormat(ConsoleOutputFormat::Text);
                return ConsoleResult::Ok("Output format: text");
            }
            return ConsoleResult::Error("Unknown format: " + args[0] + " (use 'text' or 'json')");
        }, [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> opts = {"text", "json"};
            if (partial.empty()) return opts;
            std::vector<std::string> matches;
            for (auto& o : opts) {
                if (o.find(partial) == 0) matches.push_back(o);
            }
            return matches;
        }
    });

    // -- caps --
    commands_.Register({"caps", "caps [module_id]", "Show capabilities for a module",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto mods = kernel.Modules().ListModules();
            if (args.empty()) {
                std::ostringstream ss;
                ss << "Module capabilities:\n";
                for (auto& m : mods) {
                    ss << "  " << m.name << ": ";
                    bool first = true;
                    for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                        if (HasCap(m.grantedCaps, ALL_CAPS[i])) {
                            if (!first) ss << ", ";
                            ss << CapName(ALL_CAPS[i]);
                            first = false;
                        }
                    }
                    ss << "\n";
                }
                return ConsoleResult::Ok(ss.str());
            }
            // Find module by name
            for (auto& m : mods) {
                if (m.name == args[0]) {
                    std::ostringstream ss;
                    ss << m.name << " capabilities:\n";
                    ss << "  Required: ";
                    bool first = true;
                    for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                        if (HasCap(m.requiredCaps, ALL_CAPS[i])) {
                            if (!first) ss << ", ";
                            ss << CapName(ALL_CAPS[i]);
                            first = false;
                        }
                    }
                    ss << "\n  Granted:  ";
                    first = true;
                    for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                        if (HasCap(m.grantedCaps, ALL_CAPS[i])) {
                            if (!first) ss << ", ";
                            ss << CapName(ALL_CAPS[i]);
                            first = false;
                        }
                    }
                    ss << "\n";
                    return ConsoleResult::Ok(ss.str());
                }
            }
            return ConsoleResult::NotFound("Module not found: " + args[0]);
        }, nullptr
    });

    // -- clear --
    commands_.Register({"clear", "clear", "Clear console output",
        [this](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            if (widget_ && widget_->IsBuilt()) {
                widget_->ClearOutput();
            }
            return ConsoleResult::Ok("\033[2J\033[H"); // ANSI clear for terminal frontends
        }, nullptr
    });

    // Register extended command sets
    RegisterProfilingCommands(commands_);
    RegisterReflectionCommands(commands_);
    RegisterMessageCommands(commands_);
    RegisterServiceCommands(commands_);
    RegisterMemoryCommands(commands_);
    RegisterGPUCommands(commands_);
    RegisterCVarCommands(commands_);
    RegisterLogCommands(commands_);
    RegisterUtilityCommands(commands_);
    RegisterConfigCommands(commands_);
    RegisterEntityCommands(commands_);
    RegisterRenderGraphCommands(commands_);
    RegisterHotReloadCommands(commands_);
    RegisterGPUTimingCommands(commands_);
    RegisterErrorCommands(commands_);
    RegisterTaskCommands(commands_);

    // -- listen --
    commands_.Register({"listen", "listen [port]", "Start TCP console server (default: 9090)",
        [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            uint16_t port = 9090;
            if (!args.empty()) {
                try { port = static_cast<uint16_t>(std::stoul(args[0])); }
                catch (...) { return ConsoleResult::Error("Invalid port: " + args[0]); }
            }
            if (StartSocket(port)) {
                return ConsoleResult::Ok("Console server started on 127.0.0.1:" + std::to_string(port));
            }
            return ConsoleResult::Error("Failed to start console server on port " + std::to_string(port));
        }, nullptr
    });

    // -- listen-stop --
    commands_.Register({"listen-stop", "listen-stop", "Stop TCP console server",
        [this](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            if (socket_ && socket_->IsRunning()) {
                StopSocket();
                return ConsoleResult::Ok("Console server stopped.");
            }
            return ConsoleResult::Ok("Console server is not running.");
        }, nullptr
    });
}

} // namespace koilo
