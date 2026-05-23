#pragma once

#include <ScriptFramework/Marionette.h>

// === C++ includes ===
#include <cstdint>
#include <string>
#include <string_view>
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
            "machineGunInterval",
            0.055f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Combat",
            "machineGunSpeed",
            82.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Combat",
            "machineGunLifeTime",
            0.85f,
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
            "Combat",
            "reverseMissileStateDuration",
            8.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Combat",
            "reverseMissileTurnSpeed",
            16.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "SonicStream",
            "sonicStreamDuration",
            6.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "SonicStream",
            "sonicLockAcquireMultiplier",
            2.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "SonicStream",
            "sonicFireIntervalMultiplier",
            0.65f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "SonicStream",
            "sonicMissileTurnMultiplier",
            1.35f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Armor",
            "armorStateDuration",
            10.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "InfiniteMissile",
            "infiniteMissileDuration",
            7.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "InfiniteMissile",
            "infiniteFireIntervalMultiplier",
            0.35f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "InfiniteMissile",
            "infiniteLockRangeMultiplier",
            1.8f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "LockOn",
            "lockRange",
            90.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "LockOn",
            "lockWidth",
            4.5f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "LockOn",
            "lockHeight",
            3.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "LockOn",
            "lockAcquireTime",
            0.28f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "LockOn",
            "missileTurnSpeed",
            10.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldMaxGauge",
            30.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldDrainPerSecond",
            1.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldRechargePerSecond",
            0.65f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldHalfExtentX",
            3.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldHalfExtentY",
            2.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "ConvertField",
            "fieldHalfExtentZ",
            3.2f,
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
            "enemyMissileSpawnInterval",
            1.6f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "enemyMissileSpeed",
            26.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "largeMissileSpawnInterval",
            5.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "largeMissileSpeed",
            16.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "normalBulletSpawnInterval",
            1.1f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "normalBulletSpeed",
            34.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Spawn",
            "spawnLeadDistance",
            72.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "World",
            "worldScrollSpeed",
            18.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Flow",
            "stageDuration",
            120.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_INT32_META(
            "Flow",
            "playerHullMax",
            3,
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
        CueEntityHandle target{ k_cueInvalidHandleValue };
        CueFloat3 direction{ 0.0f, 0.0f, 1.0f };
        float radius = 0.35f;
        float turnSpeed = 10.0f;
        float speed = 54.0f;
        float lifeTime = 2.4f;
        int damage = 1;
        float age = 0.0f;
    };

    struct EnemyMissile final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        CueFloat3 direction{ 0.0f, 0.0f, -1.0f };
        float radius = 0.35f;
        float age = 0.0f;
    };

    struct LargeMissile final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        CueFloat3 direction{ 0.0f, 0.0f, -1.0f };
        float radius = 0.95f;
        float age = 0.0f;
    };

    enum class EnemyKind : uint8_t
    {
        Fighter,
        MissileCarrier,
        HeavyCarrier,
        BossPart,
        BossCore,
    };

    struct Enemy final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        float radius = 1.2f;
        int hp = 1;
        EnemyKind kind = EnemyKind::Fighter;
    };

    struct EnemyBullet final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        CueFloat3 direction{ 0.0f, 0.0f, -1.0f };
        float radius = 0.32f;
        float age = 0.0f;
    };

    struct Salvage final
    {
        CueEntityHandle entity{ k_cueInvalidHandleValue };
        float radius = 0.9f;
        bool inConvertField = false;
    };

    struct TerrainProxyDef final
    {
        std::string name{};
        CueFloat3 position{ 0.0f, 0.0f, 0.0f };
        CueFloat3 halfExtent{ 0.5f, 0.5f, 0.5f };
    };

    struct TerrainSegmentDef final
    {
        std::string id{};
        std::string modelName{};
        float length = 80.0f;
        bool hasObstacles = false;
        std::vector<TerrainProxyDef> proxies{};
    };

    struct ActiveTerrainSegment final
    {
        CueEntityHandle visualEntity{ k_cueInvalidHandleValue };
        std::vector<CueEntityHandle> proxyEntities{};
        uint32_t definitionIndex = 0;
        float centerZ = 0.0f;
        float length = 80.0f;
    };

    enum class FlowState : uint8_t
    {
        Title,
        Playing,
        GameOver,
        Result,
    };

    enum class StagePhase : uint8_t
    {
        Launch,
        EnemyFormation,
        SmallMissileIntro,
        SalvageIntro,
        LargeMissileIntro,
        CombineStates,
        MissileInfinity,
        Boss,
    };

    void enter_title_scene();
    void start_gameplay();
    void enter_game_over_scene();
    void enter_result_scene();
    void request_scene_transition(const char* a_sceneName, bool a_cleared);
    void destroy_play_scene_objects();
    void update_scene_flow(float a_deltaTime);
    void update_gameplay(float a_deltaTime);
    void update_stage_phase();
    [[nodiscard]] StagePhase phase_for_time(float a_elapsedTime) const noexcept;
    void reset_gameplay_state();
    void reset_player_transform() const;
    void clear_dynamic_entities();
    void ensure_flow_ui();
    void update_flow_ui();
    void destroy_flow_ui();
    CueEntityHandle spawn_ui_text(
        const char* a_name,
        float a_y,
        float a_height,
        uint32_t a_fontSize,
        uint32_t a_order,
        const Marionette::Color& a_color);
    void set_ui_text(
        CueEntityHandle a_entity,
        std::string_view a_text,
        uint32_t a_fontSize,
        uint32_t a_order,
        const Marionette::Color& a_color) const;
    void register_player_hit(int a_scorePenalty);
    void resolve_player();
    void configure_player_collider() const;
    void update_convert_field(float a_deltaTime);
    void activate_convert_field(const Marionette::Transform& a_playerTransform);
    void deactivate_convert_field();
    void update_convert_field_transform(
        const Marionette::Transform& a_playerTransform) const;
    void update_convert_field_targets(
        const Marionette::Transform& a_playerTransform);
    void absorb_enemy_missile(EnemyMissile& a_missile);
    void absorb_large_missile(LargeMissile& a_missile);
    void absorb_salvage(Salvage& a_salvage);
    [[nodiscard]] bool is_inside_convert_field(
        const CueFloat3& a_center,
        const CueFloat3& a_position,
        float a_radius) const;
    [[nodiscard]] bool is_sonic_stream_active() const noexcept;
    [[nodiscard]] bool is_armor_active() const noexcept;
    [[nodiscard]] bool is_infinite_missile_ready() const noexcept;
    [[nodiscard]] bool is_infinite_missile_active() const noexcept;
    void update_infinite_missile(float a_deltaTime);
    void update_player_status_visual();
    [[nodiscard]] bool consume_armor();
    void update_lock_on(float a_deltaTime);
    [[nodiscard]] bool is_enemy_in_lock_range(
        const Marionette::Transform& a_playerTransform,
        const Marionette::Transform& a_enemyTransform) const;
    [[nodiscard]] bool is_locked(CueEntityHandle a_entity) const;
    void update_lock_visuals();
    void clear_lock_visual(CueEntityHandle a_entity) const;
    void set_lock_visual(
        CueEntityHandle a_entity,
        const Marionette::Color& a_color) const;
    void update_combat(float a_deltaTime);
    void load_terrain_config();
    void unload_terrain_config();
    void update_terrain_segments(float a_deltaTime);
    void update_terrain_collisions(float a_deltaTime);
    void spawn_terrain_segment(
        const TerrainSegmentDef& a_definition,
        uint32_t a_definitionIndex,
        float a_centerZ);
    void destroy_terrain_segment(ActiveTerrainSegment& a_segment) const;
    [[nodiscard]] uint32_t choose_terrain_segment_index() const noexcept;
    [[nodiscard]] bool read_terrain_segment_definition(
        Marionette::JsonConfigHandle a_indexConfig,
        uint32_t a_index,
        TerrainSegmentDef& a_outDefinition);
    void translate_entity_z(CueEntityHandle a_entity, float a_deltaZ) const;
    void update_spawning(float a_deltaTime);
    void update_missiles(float a_deltaTime);
    void update_enemy_bullets(float a_deltaTime);
    void update_enemy_missiles(float a_deltaTime);
    void update_large_missiles(float a_deltaTime);
    void update_enemies(float a_deltaTime);
    void update_salvage();
    void cleanup_behind_player();
    void spawn_machine_gun_bullet(
        const Marionette::Transform& a_playerTransform);
    void spawn_missile(
        const Marionette::Transform& a_playerTransform,
        CueEntityHandle a_target,
        bool a_isReverse);
    void spawn_enemy(float a_playerZ);
    void spawn_enemy_kind(EnemyKind a_kind, float a_playerZ);
    void spawn_boss(float a_playerZ);
    void run_boss_attack_pattern(const Marionette::Transform& a_playerTransform);
    void spawn_enemy_bullet(const Marionette::Transform& a_playerTransform);
    void spawn_enemy_missile(const Marionette::Transform& a_playerTransform);
    void spawn_large_missile(const Marionette::Transform& a_playerTransform);
    void spawn_salvage(float a_playerZ);
    void spawn_salvage_at(const CueFloat3& a_position);
    void destroy_entity_safe(CueEntityHandle a_entity) const;
    void log_progress();

    CueEntityHandle playerEntity{ k_cueInvalidHandleValue };
    std::vector<Missile> missiles{};
    std::vector<EnemyBullet> enemyBullets{};
    std::vector<EnemyMissile> enemyMissiles{};
    std::vector<LargeMissile> largeMissiles{};
    std::vector<Enemy> enemies{};
    std::vector<Salvage> salvages{};
    std::vector<TerrainSegmentDef> terrainDefinitions{};
    std::vector<ActiveTerrainSegment> activeTerrainSegments{};
    CueEntityHandle convertFieldEntity{ k_cueInvalidHandleValue };
    std::vector<CueEntityHandle> lockedEnemies{};
    std::vector<CueEntityHandle> visualLockedEnemies{};
    CueEntityHandle uiCanvasEntity{ k_cueInvalidHandleValue };
    CueEntityHandle uiTitleEntity{ k_cueInvalidHandleValue };
    CueEntityHandle uiBodyEntity{ k_cueInvalidHandleValue };
    CueEntityHandle uiHudEntity{ k_cueInvalidHandleValue };
    FlowState flowState = FlowState::Title;
    bool hasLoggedStartup = false;
    bool isConvertFieldActive = false;
    bool isPlayerStatusVisualActive = false;
    bool hasLoggedInfiniteReady = false;
    bool hasLoadedTerrainConfig = false;
    bool pendingGameOver = false;
    bool pendingResult = false;
    bool hasRequestedSceneTransition = false;
    bool hasSpawnedBoss = false;
    bool bossCoreExposed = false;
    int bossPartsDestroyed = 0;
    uint32_t bossAttackStep = 0;
    StagePhase stagePhase = StagePhase::Launch;
    float machineGunTimer = 0.0f;
    float missileFireTimer = 0.0f;
    float terrainNextEntryZ = 0.0f;
    float terrainHitCooldown = 0.0f;
    float enemySpawnTimer = 0.0f;
    float normalBulletSpawnTimer = 1.0f;
    float bossAttackTimer = 1.0f;
    float enemyMissileSpawnTimer = 0.35f;
    float largeMissileSpawnTimer = 2.4f;
    float salvageSpawnTimer = 0.65f;
    float elapsedTime = 0.0f;
    float progressLogTimer = 0.0f;
    float fireInterval = 0.18f;
    float machineGunInterval = 0.055f;
    float machineGunSpeed = 82.0f;
    float machineGunLifeTime = 0.85f;
    float missileSpeed = 54.0f;
    float missileLifeTime = 2.4f;
    float reverseMissileStateTimer = 0.0f;
    float reverseMissileStateDuration = 8.0f;
    float reverseMissileTurnSpeed = 16.0f;
    float sonicStreamTimer = 0.0f;
    float sonicStreamDuration = 6.0f;
    float sonicLockAcquireMultiplier = 2.0f;
    float sonicFireIntervalMultiplier = 0.65f;
    float sonicMissileTurnMultiplier = 1.35f;
    float armorStateTimer = 0.0f;
    float armorStateDuration = 10.0f;
    float infiniteMissileTimer = 0.0f;
    float infiniteMissileDuration = 7.0f;
    float infiniteFireIntervalMultiplier = 0.35f;
    float infiniteLockRangeMultiplier = 1.8f;
    float lockRange = 90.0f;
    float lockWidth = 4.5f;
    float lockHeight = 3.0f;
    float lockAcquireTime = 0.28f;
    float missileTurnSpeed = 10.0f;
    float fieldGauge = 30.0f;
    float fieldMaxGauge = 30.0f;
    float fieldDrainPerSecond = 1.0f;
    float fieldRechargePerSecond = 0.65f;
    float fieldHalfExtentX = 3.0f;
    float fieldHalfExtentY = 2.0f;
    float fieldHalfExtentZ = 3.2f;
    float enemySpawnInterval = 1.15f;
    float salvageSpawnInterval = 2.0f;
    float enemyMissileSpawnInterval = 1.6f;
    float enemyMissileSpeed = 26.0f;
    float largeMissileSpawnInterval = 5.0f;
    float largeMissileSpeed = 16.0f;
    float normalBulletSpawnInterval = 1.1f;
    float normalBulletSpeed = 34.0f;
    float spawnLeadDistance = 72.0f;
    float worldScrollSpeed = 18.0f;
    float stageDuration = 120.0f;
    int score = 0;
    int salvageCount = 0;
    int reverseMissileAmmo = 0;
    int armorCount = 0;
    int playerHull = 3;
    int playerHullMax = 3;
    uint32_t spawnIndex = 0;
};

[[nodiscard]] Cue::Core::Native::ScriptClassDefinition
make_game_manager_script_definition() noexcept;
