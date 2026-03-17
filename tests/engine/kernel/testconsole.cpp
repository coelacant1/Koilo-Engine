#include "testconsole.hpp"
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/kernel/console/console_session.hpp>
#include <koilo/kernel/console/console_result.hpp>
#include <string>
#include <vector>

using namespace koilo;

// --- Tokenizer ---

void TestKernelConsole::TestTokenizeBasic() {
    KoiloKernel kernel;
    CommandRegistry reg;
    ConsoleSession session(kernel, reg);

    // Test via Execute (Tokenize is private, test through behavior)
    reg.Register({"test-args", "", "", [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
        return ConsoleResult::Ok(std::to_string(args.size()));
    }, nullptr});

    auto r = session.Execute("test-args a b c");
    TEST_ASSERT_EQUAL_STRING("3", r.text.c_str());
}

void TestKernelConsole::TestTokenizeQuoted() {
    KoiloKernel kernel;
    CommandRegistry reg;
    ConsoleSession session(kernel, reg);

    std::string capturedArg;
    reg.Register({"test-quote", "", "", [&capturedArg](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
        capturedArg = args.empty() ? "" : args[0];
        return ConsoleResult::Ok(std::to_string(args.size()));
    }, nullptr});

    auto r = session.Execute("test-quote \"hello world\" second");
    TEST_ASSERT_EQUAL_STRING("2", r.text.c_str());
    TEST_ASSERT_EQUAL_STRING("hello world", capturedArg.c_str());
}

void TestKernelConsole::TestTokenizeEmpty() {
    KoiloKernel kernel;
    CommandRegistry reg;
    ConsoleSession session(kernel, reg);

    auto r = session.Execute("");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_STRING("", r.text.c_str());
}

// --- Command Registry ---

void TestKernelConsole::TestCommandRegister() {
    CommandRegistry reg;
    reg.Register({"test", "test", "A test command",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            return ConsoleResult::Ok("ok");
        }, nullptr
    });

    TEST_ASSERT_NOT_NULL(reg.Find("test"));
    TEST_ASSERT_NULL(reg.Find("nonexistent"));
}

void TestKernelConsole::TestCommandExecute() {
    CommandRegistry reg;
    reg.Register({"greet", "greet <name>", "Greet someone",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            return ConsoleResult::Ok("Hello, " + (args.empty() ? "world" : args[0]) + "!");
        }, nullptr
    });

    KoiloKernel kernel;
    auto r = reg.Execute(kernel, "greet", {"Alice"});
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_STRING("Hello, Alice!", r.text.c_str());
}

void TestKernelConsole::TestCommandNotFound() {
    CommandRegistry reg;
    KoiloKernel kernel;
    auto r = reg.Execute(kernel, "nosuchcmd", {});
    TEST_ASSERT_EQUAL(ConsoleResult::Status::NotFound, r.status);
}

void TestKernelConsole::TestCommandComplete() {
    CommandRegistry reg;
    KoiloKernel kernel;
    reg.Register({"help", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("");
    }, nullptr});
    reg.Register({"history", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("");
    }, nullptr});
    reg.Register({"echo", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("");
    }, nullptr});

    auto matches = reg.Complete(kernel, "", {}, "h");
    TEST_ASSERT_EQUAL(2, matches.size());
}

// --- Console Session ---

void TestKernelConsole::TestSessionExecute() {
    KoiloKernel kernel;
    CommandRegistry reg;
    reg.Register({"ping", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("pong");
    }, nullptr});

    ConsoleSession session(kernel, reg);
    auto r = session.Execute("ping");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_STRING("pong", r.text.c_str());
}

void TestKernelConsole::TestSessionHistory() {
    KoiloKernel kernel;
    CommandRegistry reg;
    reg.Register({"noop", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("");
    }, nullptr});

    ConsoleSession session(kernel, reg);
    session.Execute("noop");
    session.Execute("noop arg1");
    session.Execute("noop arg2");

    TEST_ASSERT_EQUAL(3, session.History().size());
    TEST_ASSERT_EQUAL_STRING("noop", session.History()[0].c_str());
    TEST_ASSERT_EQUAL_STRING("noop arg2", session.History()[2].c_str());
}

void TestKernelConsole::TestSessionAlias() {
    KoiloKernel kernel;
    CommandRegistry reg;
    std::string lastCmd;
    reg.Register({"echo", "", "", [&lastCmd](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
        lastCmd = args.empty() ? "" : args[0];
        return ConsoleResult::Ok(lastCmd);
    }, nullptr});

    ConsoleSession session(kernel, reg);
    session.SetAlias("hi", "echo hello");
    auto r = session.Execute("hi");
    TEST_ASSERT_EQUAL_STRING("hello", lastCmd.c_str());
}

void TestKernelConsole::TestSessionOutputFormat() {
    KoiloKernel kernel;
    CommandRegistry reg;
    reg.Register({"data", "", "", [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
        return ConsoleResult::Ok("text output", "{\"key\":\"value\"}");
    }, nullptr});

    ConsoleSession session(kernel, reg);

    session.SetOutputFormat(ConsoleOutputFormat::Text);
    auto r = session.Execute("data");
    TEST_ASSERT_EQUAL_STRING("text output", session.FormatResult(r).c_str());

    session.SetOutputFormat(ConsoleOutputFormat::Json);
    r = session.Execute("data");
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", session.FormatResult(r).c_str());
}

// --- Built-in Commands ---

void TestKernelConsole::TestHelpCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("help");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("Available commands") != std::string::npos);

    kernel.Shutdown();
}

void TestKernelConsole::TestEchoCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("echo hello world");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_STRING("hello world", r.text.c_str());

    kernel.Shutdown();
}

void TestKernelConsole::TestVersionCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("version");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("Koilo") != std::string::npos);

    kernel.Shutdown();
}

void TestKernelConsole::TestModulesCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("modules");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("koilo.console") != std::string::npos);
    TEST_ASSERT(!r.json.empty());

    kernel.Shutdown();
}

void TestKernelConsole::TestServicesCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("services");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("console") != std::string::npos);

    kernel.Shutdown();
}

void TestKernelConsole::TestMemCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("mem");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("Frame Arena") != std::string::npos);
    TEST_ASSERT(!r.json.empty());

    kernel.Shutdown();
}

void TestKernelConsole::TestMessagesCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("messages");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("Dispatched") != std::string::npos);
    TEST_ASSERT(!r.json.empty());

    kernel.Shutdown();
}

void TestKernelConsole::TestCapsCommand() {
    KoiloKernel kernel;
    kernel.RegisterModule(ConsoleModule::GetModuleDesc());
    kernel.InitializeModules();

    auto r = ConsoleModule::Instance()->Execute("caps");
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT(r.text.find("koilo.console") != std::string::npos);

    kernel.Shutdown();
}

// --- Console Module Lifecycle ---

void TestKernelConsole::TestKernelConsoleModuleLifecycle() {
    {
        KoiloKernel kernel;
        kernel.RegisterModule(ConsoleModule::GetModuleDesc());
        TEST_ASSERT_TRUE(kernel.InitializeModules());
        TEST_ASSERT_NOT_NULL(ConsoleModule::Instance());
        TEST_ASSERT_TRUE(kernel.Services().Has("console"));

        // Execute a command
        auto r = ConsoleModule::Instance()->Execute("echo test");
        TEST_ASSERT_EQUAL_STRING("test", r.text.c_str());

        kernel.Shutdown();
    }
    TEST_ASSERT_NULL(ConsoleModule::Instance());
}

// --- RunAllTests ---

void TestKernelConsole::RunAllTests() {
    // Tokenizer
    RUN_TEST(TestKernelConsole::TestTokenizeBasic);
    RUN_TEST(TestKernelConsole::TestTokenizeQuoted);
    RUN_TEST(TestKernelConsole::TestTokenizeEmpty);

    // Command registry
    RUN_TEST(TestKernelConsole::TestCommandRegister);
    RUN_TEST(TestKernelConsole::TestCommandExecute);
    RUN_TEST(TestKernelConsole::TestCommandNotFound);
    RUN_TEST(TestKernelConsole::TestCommandComplete);

    // Console session
    RUN_TEST(TestKernelConsole::TestSessionExecute);
    RUN_TEST(TestKernelConsole::TestSessionHistory);
    RUN_TEST(TestKernelConsole::TestSessionAlias);
    RUN_TEST(TestKernelConsole::TestSessionOutputFormat);

    // Built-in commands
    RUN_TEST(TestKernelConsole::TestHelpCommand);
    RUN_TEST(TestKernelConsole::TestEchoCommand);
    RUN_TEST(TestKernelConsole::TestVersionCommand);
    RUN_TEST(TestKernelConsole::TestModulesCommand);
    RUN_TEST(TestKernelConsole::TestServicesCommand);
    RUN_TEST(TestKernelConsole::TestMemCommand);
    RUN_TEST(TestKernelConsole::TestMessagesCommand);
    RUN_TEST(TestKernelConsole::TestCapsCommand);

    // Module lifecycle
    RUN_TEST(TestKernelConsole::TestKernelConsoleModuleLifecycle);
}
