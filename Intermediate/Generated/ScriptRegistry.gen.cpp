#include "ScriptRegistry.h"

#include "Scripts/GameManagerScript.h"
#include "Scripts/PlayerFlightScript.h"

// === C++ includes ===
#include <array>
#include <span>

// *** This file is generated. Do not edit by hand.
// *** Why: script registration is derived from Assets/Scripts at build time.

namespace
{
    const std::array<Cue::Core::Native::ScriptClassDefinition, 2> k_scriptClasses = {
        make_game_manager_script_definition(),
        make_player_flight_script_definition(),
    };
}

std::span<const Cue::Core::Native::ScriptClassDefinition>
script_classes() noexcept
{
    return std::span<const Cue::Core::Native::ScriptClassDefinition>(
        k_scriptClasses.data(),
        k_scriptClasses.size());
}
