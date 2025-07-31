#pragma once
#include "Oxidize/core/OptRes.hpp"
#include "Oxidize/core/Fn.hpp"
#include "Oxidize/string/String.hpp"
#include "Oxidize/panic/Unwind.hpp"

namespace ox::panic {

template <typename F>
    requires Callable<F, void>
Result<Void, String> catch_unwind(F &&f) noexcept {
    try {
        std::invoke(std::forward<F>(f));
        return Ok();
    } catch(PanicUnwind &unwind) {
        unwind.supress();
        return Err(String(unwind.what()));
    }
}

inline void resume_unwind(String payload) {
    panic("{}", payload);
}

} // namespace ox::panic