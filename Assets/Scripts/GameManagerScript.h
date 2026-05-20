#pragma once

#include <ScriptFramework/Marionette.h>

// === C++ includes ===
#include <cstdint>
#include <vector>

MARIONETTE_DECLARE_SCRIPT_TYPE(GameManager, "GameManager");

class GameManager final : public Marionette::Behaviour<GameManager>
{
public:
    using StateBlob = Marionette::StateBlob<GameManager>;
    using Marionette::Behaviour<GameManager>::update;
    MARIONETTE_FIELDS(
        CUE_FIELD_FLOAT_META(
            "Combat",
            "fireInterval",
            0.18f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Combat",
            "missileSpeed",
            54.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Combat",
            "missileLifeTime",
            2.4f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "enemySpawnInterval",
            1.15f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "salvageSpawnInterval",
            2.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "spawnLeadDistance",
            72.0f,
            Marionette::EditAnywhere | Marionette::Serialize)
    );
    MARIONETTE_NO_FUNCTIONS();

    void bind_fields(const Marionette::ScriptFieldReader& a_reader);
    void start();
    void update();

private:
    struct Missile final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        float age = 0.0f;
    };

    struct Enemy final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        float radius = 1.2f;
        int hp = 1;
    };

    struct Salvage final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        float radius = 0.9f;
    };

    void resolve_player();
    void configure_player_collider() const;
    void update_combat(float a_deltaTime);
    void update_spawning(float a_deltaTime);
    void update_missiles(float a_deltaTime);
    void update_enemies(float a_deltaTime);
    void update_salvage();
    void cleanup_behind_player();
    void spawn_missile(const Marionette::Transform& a_playerTransform);
    void spawn_enemy(float a_playerZ);
    void spawn_salvage(float a_playerZ);
    void destroy_entity_safe(CueEntityHandle a_entity) const;
    void log_progress();

    CueEntityHandle playerEntity{ k_cueInvalidHandleValue };
    std::vector<Missile> missiles{};
    std::vector<Enemy> enemies{};
    std::vector<Salvage> salvages{};
    bool hasLoggedStartup = false;
    float fireTimer = 0.0f;
    float enemySpawnTimer = 0.0f;
    float salvageSpawnTimer = 0.65f;
    float elapsedTime = 0.0f;
    float progressLogTimer = 0.0f;
    float fireInterval = 0.18f;
    float missileSpeed = 54.0f;
    float missileLifeTime = 2.4f;
    float enemySpawnInterval = 1.15f;
    float salvageSpawnInterval = 2.0f;
    float spawnLeadDistance = 72.0f;
    int score = 0;
    int salvageCount = 0;
    uint32_t spawnIndex = 0;
};

[[nodiscard]] Cue::Core::Native::ScriptClassDefinition
make_game_manager_script_definition() noexcept;
