#pragma once
#include <unity.h>

class TestKernelConsole {
public:
    // Tokenizer
    static void TestTokenizeBasic();
    static void TestTokenizeQuoted();
    static void TestTokenizeEmpty();

    // Command registry
    static void TestCommandRegister();
    static void TestCommandExecute();
    static void TestCommandNotFound();
    static void TestCommandComplete();

    // Console session
    static void TestSessionExecute();
    static void TestSessionHistory();
    static void TestSessionAlias();
    static void TestSessionOutputFormat();

    // Built-in commands
    static void TestHelpCommand();
    static void TestEchoCommand();
    static void TestVersionCommand();
    static void TestModulesCommand();
    static void TestServicesCommand();
    static void TestMemCommand();
    static void TestMessagesCommand();
    static void TestCapsCommand();

    // Console module
    static void TestKernelConsoleModuleLifecycle();

    static void RunAllTests();
};
