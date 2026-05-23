#pragma once

#include <cstdint>

namespace BorroWing
{
    struct RunSummary final
    {
        int score = 0;
        int salvageCount = 0;
        int armorCount = 0;
        float elapsedTime = 0.0f;
        bool cleared = false;
    };

    [[nodiscard]] bool is_gameplay_active() noexcept;
    void set_gameplay_active(bool a_isActive) noexcept;
    [[nodiscard]] uint32_t player_reset_serial() noexcept;
    void bump_player_reset_serial() noexcept;
    void set_last_run_summary(const RunSummary& a_summary) noexcept;
    [[nodiscard]] RunSummary last_run_summary() noexcept;
}
