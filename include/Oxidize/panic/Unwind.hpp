#pragma once
#include "Oxidize/core/Types.hpp"
#include <exception>
#include <iostream>
#include <fmt/format.h>
#include <source_location>

namespace ox {

namespace panic {

struct PanicInfo {
    RawString message;
    RawString backtrace;
    RawString file;
    RawString func;
    usize line;
    usize column;
};

using PanicHookFn = void (*)(const PanicInfo &);

static void default_hook(const PanicInfo &info) {
    std::cout << fmt::format("\033[1;31mPanic at {}:{}:{} in {}:\033[0m", info.file, info.line, info.column, info.func) << std::endl;
    std::cout << info.message << std::endl;
    if (std::getenv("BACKTRACE")) {
        std::cout << "note: nacktraces are unsupported in the current version of Oxidize" << std::endl;
    } else {
        std::cout << "note: nacktraces are unsupported in the current version of Oxidize" << std::endl;
    }
}

class PanicHook {
  public:
    PanicHookFn hook = default_hook;

    static PanicHook &get_instance() {
        static PanicHook instance;
        return instance;
    }
};

inline void set_hook(PanicHookFn hook) {
    PanicHook::get_instance().hook = hook;
}

struct PanicUnwind;

struct Panic {
  private:
    bool panicking = false;
    Panic() {}

  public:
    static Panic &get_instance() {
        thread_local Panic inst;
        return inst;
    }

    static void panic_now(PanicInfo info);

    static void not_anymore() { get_instance().panicking = false; }

    static bool is_panicking() { return get_instance().panicking; }
};
struct PanicUnwind : public std::exception {
    PanicInfo info;
    bool suppressed = false;

  public:
    explicit PanicUnwind(PanicInfo info) : info(info) {}
    ~PanicUnwind() {
        if (!suppressed) {
            panic::PanicHook::get_instance().hook(info);
        }
        Panic::not_anymore();
    }

    void supress() noexcept { suppressed = true; }

    const char *what() const noexcept override { return info.message.c_str(); }
};

inline void Panic::panic_now(PanicInfo info) {
    auto &inst = get_instance();
    if (inst.panicking) {
        std::cerr << "\033[1;31mdouble panic! aborting.\033[0m\n";
        std::abort();
    }
    inst.panicking = true;
    throw PanicUnwind(info);
}

inline void panic_helper(
    RawString message, const std::source_location &loc = std::source_location::current()) {
    PanicInfo info = {.message = message,
        .backtrace = "",
        .file = loc.file_name(),
        .func = loc.function_name(),
        .line = loc.line(),
        .column = loc.column()};

    Panic::panic_now(info);
}

} // namespace panic

#define panic(text, ...) ox::panic::panic_helper(fmt::format("" text, ##__VA_ARGS__))

} // namespace ox

namespace ox::thread {

inline bool panicking() {
    return ox::panic::Panic::is_panicking();
}

} // namespace ox::thread