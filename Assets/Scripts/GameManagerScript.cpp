#include "GameManagerScript.h"

// === C++ includes ===
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "BorroWingGameState.h"

namespace
{
    bool g_isGameplayActive = false;
    uint32_t g_playerResetSerial = 0u;
    BorroWing::RunSummary g_lastRunSummary{};

    constexpr Marionette::Color k_lockedEnemyColor{
        0.1f, 0.85f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_convertFieldColor{
        0.2f, 0.75f, 1.0f, 0.24f
    };
    constexpr Marionette::Color k_fieldTargetColor{
        0.35f, 1.0f, 0.45f, 1.0f
    };
    constexpr Marionette::Color k_salvageColor{
        0.15f, 0.95f, 0.28f, 1.0f
    };
    constexpr Marionette::Color k_playerMissileColor{
        0.78f, 0.92f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_machineGunColor{
        1.0f, 0.94f, 0.45f, 1.0f
    };
    constexpr Marionette::Color k_reverseMissileColor{
        0.15f, 1.0f, 0.72f, 1.0f
    };
    constexpr Marionette::Color k_enemyMissileColor{
        1.0f, 0.22f, 0.12f, 1.0f
    };
    constexpr Marionette::Color k_enemyBulletColor{
        1.0f, 0.05f, 0.08f, 1.0f
    };
    constexpr Marionette::Color k_largeMissileColor{
        0.78f, 0.18f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_bossColor{
        0.95f, 0.1f, 0.22f, 1.0f
    };
    constexpr Marionette::Color k_bossPartColor{
        0.95f, 0.42f, 0.16f, 1.0f
    };
    constexpr Marionette::Color k_bossCoreLockedColor{
        0.24f, 0.24f, 0.28f, 1.0f
    };
    constexpr Marionette::Color k_missileCarrierColor{
        0.22f, 0.75f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_heavyCarrierColor{
        0.86f, 0.22f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_sonicStreamColor{
        0.12f, 0.55f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_armorColor{
        1.0f, 0.78f, 0.18f, 1.0f
    };
    constexpr Marionette::Color k_infiniteMissileColor{
        1.0f, 0.25f, 0.9f, 1.0f
    };
    constexpr Marionette::Color k_uiTitleColor{
        0.82f, 0.95f, 1.0f, 1.0f
    };
    constexpr Marionette::Color k_uiBodyColor{
        0.94f, 0.94f, 0.9f, 1.0f
    };
    constexpr Marionette::Color k_uiWarningColor{
        1.0f, 0.32f, 0.22f, 1.0f
    };
    constexpr Marionette::Color k_uiResultColor{
        0.35f, 1.0f, 0.68f, 1.0f
    };
    constexpr Marionette::Color k_uiHudColor{
        0.84f, 0.92f, 1.0f, 1.0f
    };

    [[nodiscard]] CueStringView make_view(std::string_view a_value) noexcept
    {
        return CueStringView{
            a_value.data(),
            static_cast<uint32_t>(a_value.size())
        };
    }

    [[nodiscard]] float distance_sq(
        const CueFloat3& a_left,
        const CueFloat3& a_right) noexcept
    {
        const float x = a_left.x - a_right.x;
        const float y = a_left.y - a_right.y;
        const float z = a_left.z - a_right.z;
        return x * x + y * y + z * z;
    }

    [[nodiscard]] CueFloat3 subtract(
        const CueFloat3& a_left,
        const CueFloat3& a_right) noexcept
    {
        return {
            a_left.x - a_right.x,
            a_left.y - a_right.y,
            a_left.z - a_right.z
        };
    }

    [[nodiscard]] CueFloat3 scale(
        const CueFloat3& a_value,
        float a_scale) noexcept
    {
        return {
            a_value.x * a_scale,
            a_value.y * a_scale,
            a_value.z * a_scale
        };
    }

    [[nodiscard]] CueFloat3 add(
        const CueFloat3& a_left,
        const CueFloat3& a_right) noexcept
    {
        return {
            a_left.x + a_right.x,
            a_left.y + a_right.y,
            a_left.z + a_right.z
        };
    }

    [[nodiscard]] CueFloat3 normalize_or_forward(
        const CueFloat3& a_value) noexcept
    {
        const float lengthSq =
            a_value.x * a_value.x +
            a_value.y * a_value.y +
            a_value.z * a_value.z;
        if (lengthSq <= 0.0001f)
        {
            return { 0.0f, 0.0f, 1.0f };
        }

        const float invLength = 1.0f / std::sqrt(lengthSq);
        return scale(a_value, invLength);
    }

    [[nodiscard]] CueFloat3 blend_direction(
        const CueFloat3& a_current,
        const CueFloat3& a_target,
        float a_blend) noexcept
    {
        return normalize_or_forward(
            add(scale(a_current, 1.0f - a_blend), scale(a_target, a_blend)));
    }

    [[nodiscard]] Marionette::Transform make_transform(
        const CueFloat3& a_position,
        const CueFloat3& a_scale) noexcept
    {
        Marionette::Transform transform{};
        transform.position = a_position;
        transform.rotation = { 0.0f, 0.0f, 0.0f };
        transform.scale = a_scale;
        return transform;
    }

    [[nodiscard]] Marionette::SpawnObjectDesc make_spawn_desc(
        std::string_view a_name,
        std::string_view a_tag,
        const Marionette::Transform& a_transform) noexcept
    {
        Marionette::SpawnObjectDesc desc{};
        desc.kind = Marionette::SpawnObjectKindStaticMesh;
        desc.sceneId = Marionette::k_invalidSceneId;
        desc.name = make_view(a_name);
        desc.tag = make_view(a_tag);
        desc.transform = a_transform;
        desc.isActive = 1u;
        desc.isPersistent = 0u;
        return desc;
    }

    [[nodiscard]] Marionette::SpawnObjectDesc make_spawn_desc(
        Marionette::SpawnObjectKind a_kind,
        std::string_view a_name,
        std::string_view a_tag,
        const Marionette::Transform& a_transform) noexcept
    {
        Marionette::SpawnObjectDesc desc =
            make_spawn_desc(a_name, a_tag, a_transform);
        desc.kind = a_kind;
        return desc;
    }

    [[nodiscard]] Marionette::StaticMeshRendererComponentData
    make_renderer(uint8_t a_castsShadow, uint8_t a_receivesShadow) noexcept
    {
        Marionette::StaticMeshRendererComponentData renderer{};
        renderer.visible = 1u;
        renderer.castsShadow = a_castsShadow;
        renderer.receivesShadow = a_receivesShadow;
        return renderer;
    }

    [[nodiscard]] Marionette::MeshFilterComponentData make_mesh_filter(
        std::string_view a_modelName) noexcept
    {
        Marionette::MeshFilterComponentData meshFilter{};
        meshFilter.modelName = make_view(a_modelName);
        meshFilter.meshId = 0u;
        return meshFilter;
    }

    [[nodiscard]] Marionette::ColliderComponentData make_box_collider(
        const CueFloat3& a_halfExtent,
        bool a_isTrigger) noexcept
    {
        Marionette::ColliderComponentData collider{};
        collider.meshModelName = make_view("");
        collider.offset = { 0.0f, 0.0f, 0.0f };
        collider.halfExtent = a_halfExtent;
        collider.shapeType = Marionette::ColliderShapeTypeBox;
        collider.radius = 0.5f;
        collider.halfHeight = 0.5f;
        collider.friction = 0.4f;
        collider.restitution = 0.0f;
        collider.layer = 1u;
        collider.mask = 0xffffu;
        collider.isTrigger = a_isTrigger ? 1u : 0u;
        return collider;
    }

    [[nodiscard]] Marionette::ColliderComponentData make_box_trigger(
        const CueFloat3& a_halfExtent) noexcept
    {
        return make_box_collider(a_halfExtent, true);
    }

    [[nodiscard]] Marionette::CanvasComponentData make_canvas() noexcept
    {
        Marionette::CanvasComponentData canvas{};
        canvas.referenceSize = { 1920.0f, 1080.0f };
        canvas.scaleFactor = 1.0f;
        canvas.sortOrder = 100;
        canvas.matchesScreen = 1u;
        return canvas;
    }

    [[nodiscard]] Marionette::UiRectTransformComponentData make_ui_rect(
        const CueFloat2& a_anchorMin,
        const CueFloat2& a_anchorMax,
        const CueFloat2& a_pivot,
        const CueFloat2& a_anchoredPosition,
        const CueFloat2& a_sizeDelta) noexcept
    {
        Marionette::UiRectTransformComponentData rect{};
        rect.anchorMin = a_anchorMin;
        rect.anchorMax = a_anchorMax;
        rect.pivot = a_pivot;
        rect.anchoredPosition = a_anchoredPosition;
        rect.sizeDelta = a_sizeDelta;
        return rect;
    }

    [[nodiscard]] Marionette::TextRendererComponentData make_text_renderer(
        std::string_view a_text,
        uint32_t a_fontSize,
        uint32_t a_order,
        const Marionette::Color& a_color) noexcept
    {
        Marionette::TextRendererComponentData text{};
        text.text = make_view(a_text);
        text.fontPath = make_view("");
        text.color = a_color;
        text.fontSize = a_fontSize;
        text.layer = 100;
        text.order = a_order;
        text.horizontalAlign = Marionette::TextHorizontalAlignCenter;
        text.verticalAlign = Marionette::TextVerticalAlignMiddle;
        text.visible = 1u;
        return text;
    }

    [[nodiscard]] bool aabb_overlap(
        const CueFloat3& a_leftCenter,
        const CueFloat3& a_leftHalfExtent,
        const CueFloat3& a_rightCenter,
        const CueFloat3& a_rightHalfExtent) noexcept
    {
        return
            std::abs(a_leftCenter.x - a_rightCenter.x) <=
                a_leftHalfExtent.x + a_rightHalfExtent.x &&
            std::abs(a_leftCenter.y - a_rightCenter.y) <=
                a_leftHalfExtent.y + a_rightHalfExtent.y &&
            std::abs(a_leftCenter.z - a_rightCenter.z) <=
                a_leftHalfExtent.z + a_rightHalfExtent.z;
    }
}

namespace BorroWing
{
    bool is_gameplay_active() noexcept
    {
        return g_isGameplayActive;
    }

    void set_gameplay_active(bool a_isActive) noexcept
    {
        g_isGameplayActive = a_isActive;
    }

    uint32_t player_reset_serial() noexcept
    {
        return g_playerResetSerial;
    }

    void bump_player_reset_serial() noexcept
    {
        ++g_playerResetSerial;
    }

    void set_last_run_summary(const RunSummary& a_summary) noexcept
    {
        g_lastRunSummary = a_summary;
    }

    RunSummary last_run_summary() noexcept
    {
        return g_lastRunSummary;
    }
}

void GameManager::bind_fields(const Marionette::ScriptFieldReader& a_reader)
{
    (void)read_float(a_reader, "fireInterval", fireInterval);
    (void)read_float(a_reader, "machineGunInterval", machineGunInterval);
    (void)read_float(a_reader, "machineGunSpeed", machineGunSpeed);
    (void)read_float(a_reader, "machineGunLifeTime", machineGunLifeTime);
    (void)read_float(a_reader, "missileSpeed", missileSpeed);
    (void)read_float(a_reader, "missileLifeTime", missileLifeTime);
    (void)read_float(
        a_reader,
        "reverseMissileStateDuration",
        reverseMissileStateDuration);
    (void)read_float(
        a_reader,
        "reverseMissileTurnSpeed",
        reverseMissileTurnSpeed);
    (void)read_float(a_reader, "sonicStreamDuration", sonicStreamDuration);
    (void)read_float(
        a_reader,
        "sonicLockAcquireMultiplier",
        sonicLockAcquireMultiplier);
    (void)read_float(
        a_reader,
        "sonicFireIntervalMultiplier",
        sonicFireIntervalMultiplier);
    (void)read_float(
        a_reader,
        "sonicMissileTurnMultiplier",
        sonicMissileTurnMultiplier);
    (void)read_float(a_reader, "armorStateDuration", armorStateDuration);
    (void)read_float(
        a_reader,
        "infiniteMissileDuration",
        infiniteMissileDuration);
    (void)read_float(
        a_reader,
        "infiniteFireIntervalMultiplier",
        infiniteFireIntervalMultiplier);
    (void)read_float(
        a_reader,
        "infiniteLockRangeMultiplier",
        infiniteLockRangeMultiplier);
    (void)read_float(a_reader, "lockRange", lockRange);
    (void)read_float(a_reader, "lockWidth", lockWidth);
    (void)read_float(a_reader, "lockHeight", lockHeight);
    (void)read_float(a_reader, "lockAcquireTime", lockAcquireTime);
    (void)read_float(a_reader, "missileTurnSpeed", missileTurnSpeed);
    (void)read_float(a_reader, "fieldMaxGauge", fieldMaxGauge);
    (void)read_float(a_reader, "fieldDrainPerSecond", fieldDrainPerSecond);
    (void)read_float(
        a_reader,
        "fieldRechargePerSecond",
        fieldRechargePerSecond);
    (void)read_float(a_reader, "fieldHalfExtentX", fieldHalfExtentX);
    (void)read_float(a_reader, "fieldHalfExtentY", fieldHalfExtentY);
    (void)read_float(a_reader, "fieldHalfExtentZ", fieldHalfExtentZ);
    (void)read_float(a_reader, "enemySpawnInterval", enemySpawnInterval);
    (void)read_float(a_reader, "salvageSpawnInterval", salvageSpawnInterval);
    (void)read_float(
        a_reader,
        "enemyMissileSpawnInterval",
        enemyMissileSpawnInterval);
    (void)read_float(a_reader, "enemyMissileSpeed", enemyMissileSpeed);
    (void)read_float(
        a_reader,
        "largeMissileSpawnInterval",
        largeMissileSpawnInterval);
    (void)read_float(a_reader, "largeMissileSpeed", largeMissileSpeed);
    (void)read_float(
        a_reader,
        "normalBulletSpawnInterval",
        normalBulletSpawnInterval);
    (void)read_float(a_reader, "normalBulletSpeed", normalBulletSpeed);
    (void)read_float(a_reader, "spawnLeadDistance", spawnLeadDistance);
    (void)read_float(a_reader, "worldScrollSpeed", worldScrollSpeed);
    (void)read_float(a_reader, "stageDuration", stageDuration);
    int32_t hullMax = playerHullMax;
    if (read_int32(a_reader, "playerHullMax", hullMax))
    {
        playerHullMax = std::max(1, hullMax);
    }
    fieldGauge = std::max(0.0f, fieldMaxGauge);
}

void GameManager::start()
{
    if (!hasLoggedStartup)
    {
        log_info("BorroWing prototype initialized.");
        hasLoggedStartup = true;
    }

    resolve_player();
    configure_player_collider();
    load_terrain_config();
    start_gameplay();
}

void GameManager::update()
{
    const float dt = delta_time();
    resolve_player();
    ensure_flow_ui();
    update_scene_flow(dt);
    if (hasRequestedSceneTransition)
    {
        return;
    }
    update_flow_ui();
}

void GameManager::enter_title_scene()
{
    flowState = FlowState::Title;
    BorroWing::set_gameplay_active(false);
    pendingGameOver = false;
    clear_dynamic_entities();
    reset_gameplay_state();
    reset_player_transform();
    log_info("Title scene entered.");
}

void GameManager::start_gameplay()
{
    clear_dynamic_entities();
    reset_gameplay_state();
    reset_player_transform();
    configure_player_collider();
    flowState = FlowState::Playing;
    BorroWing::bump_player_reset_serial();
    BorroWing::set_gameplay_active(true);
    log_info("Gameplay started.");
}

void GameManager::enter_game_over_scene()
{
    request_scene_transition("GameOver", false);
}

void GameManager::enter_result_scene()
{
    request_scene_transition("Result", true);
}

void GameManager::request_scene_transition(
    const char* a_sceneName,
    bool a_cleared)
{
    if (hasRequestedSceneTransition)
    {
        return;
    }

    hasRequestedSceneTransition = true;
    BorroWing::set_gameplay_active(false);
    BorroWing::set_last_run_summary(BorroWing::RunSummary{
        score,
        salvageCount,
        armorCount,
        elapsedTime,
        a_cleared });

    const Marionette::SceneId loadedScene =
        Marionette::SceneManager.load_scene(a_sceneName);
    if (loadedScene == Marionette::k_invalidSceneId)
    {
        hasRequestedSceneTransition = false;
        BorroWing::set_gameplay_active(true);
        log_warning("Scene transition request failed.");
        return;
    }

    destroy_play_scene_objects();
    log_info(a_cleared ? "Result scene requested." : "Game over scene requested.");
}

void GameManager::destroy_play_scene_objects()
{
    deactivate_convert_field();
    clear_lock_visual(playerEntity);
    lockedEnemies.clear();
    visualLockedEnemies.clear();
    clear_dynamic_entities();
    destroy_flow_ui();

    for (CueEntityHandle camera : find_entities_by_tag("MainCamera"))
    {
        destroy_entity_safe(camera);
    }
    for (CueEntityHandle light : find_entities_by_tag("DirectionalLight"))
    {
        destroy_entity_safe(light);
    }
    destroy_entity_safe(playerEntity);
    playerEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    destroy_entity_safe(self());
}

void GameManager::update_scene_flow(float a_deltaTime)
{
    switch (flowState)
    {
    case FlowState::Title:
        if (push_key(Marionette::Key::Enter) ||
            push_key(Marionette::Key::Space))
        {
            start_gameplay();
        }
        return;
    case FlowState::Playing:
        update_gameplay(a_deltaTime);
        if (pendingGameOver)
        {
            enter_game_over_scene();
            return;
        }
        if (pendingResult)
        {
            enter_result_scene();
        }
        return;
    case FlowState::GameOver:
    case FlowState::Result:
        if (push_key(Marionette::Key::Enter) ||
            push_key(Marionette::Key::Space))
        {
            start_gameplay();
            return;
        }
        if (push_key(Marionette::Key::Escape))
        {
            enter_title_scene();
        }
        return;
    }
}

void GameManager::update_gameplay(float a_deltaTime)
{
    elapsedTime += a_deltaTime;
    machineGunTimer = std::max(0.0f, machineGunTimer - a_deltaTime);
    missileFireTimer = std::max(0.0f, missileFireTimer - a_deltaTime);
    reverseMissileStateTimer =
        std::max(0.0f, reverseMissileStateTimer - a_deltaTime);
    sonicStreamTimer = std::max(0.0f, sonicStreamTimer - a_deltaTime);
    armorStateTimer = std::max(0.0f, armorStateTimer - a_deltaTime);
    infiniteMissileTimer =
        std::max(0.0f, infiniteMissileTimer - a_deltaTime);
    terrainHitCooldown = std::max(0.0f, terrainHitCooldown - a_deltaTime);
    progressLogTimer = std::max(0.0f, progressLogTimer - a_deltaTime);

    resolve_player();
    update_stage_phase();
    update_terrain_segments(a_deltaTime);
    update_terrain_collisions(a_deltaTime);
    update_convert_field(a_deltaTime);
    update_infinite_missile(a_deltaTime);
    update_player_status_visual();
    update_lock_on(a_deltaTime);
    update_lock_visuals();
    update_combat(a_deltaTime);
    update_spawning(a_deltaTime);
    update_missiles(a_deltaTime);
    update_enemy_bullets(a_deltaTime);
    update_enemy_missiles(a_deltaTime);
    update_large_missiles(a_deltaTime);
    update_enemies(a_deltaTime);
    update_salvage();
    cleanup_behind_player();
    log_progress();
}

void GameManager::reset_gameplay_state()
{
    score = 0;
    salvageCount = 0;
    reverseMissileAmmo = 0;
    armorCount = 0;
    playerHull = std::max(1, playerHullMax);
    spawnIndex = 0u;
    elapsedTime = 0.0f;
    progressLogTimer = 0.0f;
    machineGunTimer = 0.0f;
    missileFireTimer = 0.0f;
    terrainHitCooldown = 0.0f;
    enemySpawnTimer = 0.0f;
    normalBulletSpawnTimer = 1.0f;
    enemyMissileSpawnTimer = 0.35f;
    largeMissileSpawnTimer = 2.4f;
    salvageSpawnTimer = 0.65f;
    reverseMissileStateTimer = 0.0f;
    sonicStreamTimer = 0.0f;
    armorStateTimer = 0.0f;
    infiniteMissileTimer = 0.0f;
    fieldGauge = std::max(0.0f, fieldMaxGauge);
    terrainNextEntryZ = 0.0f;
    pendingGameOver = false;
    pendingResult = false;
    hasLoggedInfiniteReady = false;
    isPlayerStatusVisualActive = false;
    hasSpawnedBoss = false;
    bossCoreExposed = false;
    bossPartsDestroyed = 0;
    bossAttackStep = 0;
    stagePhase = StagePhase::Launch;
    bossAttackTimer = 1.0f;
}

void GameManager::update_stage_phase()
{
    const StagePhase nextPhase = phase_for_time(elapsedTime);
    if (nextPhase == stagePhase)
    {
        return;
    }

    stagePhase = nextPhase;
    switch (stagePhase)
    {
    case StagePhase::Launch:
        log_info("Stage phase: Launch.");
        break;
    case StagePhase::EnemyFormation:
        log_info("Stage phase: Enemy formation.");
        break;
    case StagePhase::SmallMissileIntro:
        log_info("Stage phase: Small missile intro.");
        break;
    case StagePhase::SalvageIntro:
        log_info("Stage phase: Salvage intro.");
        break;
    case StagePhase::LargeMissileIntro:
        log_info("Stage phase: Large missile intro.");
        break;
    case StagePhase::CombineStates:
        log_info("Stage phase: Combine states.");
        break;
    case StagePhase::MissileInfinity:
        log_info("Stage phase: Missile infinity.");
        break;
    case StagePhase::Boss:
        log_info("Stage phase: Boss.");
        break;
    }
}

GameManager::StagePhase GameManager::phase_for_time(
    float a_elapsedTime) const noexcept
{
    const float bossStart = std::max(1.0f, stageDuration);
    if (a_elapsedTime >= bossStart)
    {
        return StagePhase::Boss;
    }
    if (a_elapsedTime >= bossStart * 0.78f)
    {
        return StagePhase::MissileInfinity;
    }
    if (a_elapsedTime >= bossStart * 0.62f)
    {
        return StagePhase::CombineStates;
    }
    if (a_elapsedTime >= bossStart * 0.48f)
    {
        return StagePhase::LargeMissileIntro;
    }
    if (a_elapsedTime >= bossStart * 0.34f)
    {
        return StagePhase::SalvageIntro;
    }
    if (a_elapsedTime >= bossStart * 0.20f)
    {
        return StagePhase::SmallMissileIntro;
    }
    if (a_elapsedTime >= bossStart * 0.08f)
    {
        return StagePhase::EnemyFormation;
    }
    return StagePhase::Launch;
}

void GameManager::reset_player_transform() const
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform transform{};
    if (get_transform(playerEntity, transform) != CueResult_Ok)
    {
        return;
    }

    transform.position = { 0.0f, 0.0f, 0.0f };
    transform.rotation = { 0.0f, 0.0f, 0.0f };
    transform.scale = { 1.0f, 1.0f, 1.0f };
    (void)set_transform(playerEntity, transform);
}

void GameManager::clear_dynamic_entities()
{
    deactivate_convert_field();

    for (const Missile& missile : missiles)
    {
        destroy_entity_safe(missile.entity);
    }
    missiles.clear();

    for (const EnemyBullet& bullet : enemyBullets)
    {
        destroy_entity_safe(bullet.entity);
    }
    enemyBullets.clear();

    for (const EnemyMissile& missile : enemyMissiles)
    {
        destroy_entity_safe(missile.entity);
    }
    enemyMissiles.clear();

    for (const LargeMissile& missile : largeMissiles)
    {
        destroy_entity_safe(missile.entity);
    }
    largeMissiles.clear();

    for (const Enemy& enemy : enemies)
    {
        destroy_entity_safe(enemy.entity);
    }
    enemies.clear();

    for (const Salvage& salvage : salvages)
    {
        destroy_entity_safe(salvage.entity);
    }
    salvages.clear();

    for (ActiveTerrainSegment& segment : activeTerrainSegments)
    {
        destroy_terrain_segment(segment);
    }
    activeTerrainSegments.clear();

    lockedEnemies.clear();
    visualLockedEnemies.clear();
}

void GameManager::ensure_flow_ui()
{
    if (uiCanvasEntity.value != k_cueInvalidHandleValue)
    {
        Marionette::Transform transform{};
        if (get_transform(uiCanvasEntity, transform) == CueResult_Ok)
        {
            return;
        }
    }

    destroy_flow_ui();

    CueEntityHandle canvas{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc(
                Marionette::SpawnObjectKindEmpty,
                "FlowCanvas",
                "FlowUi",
                make_transform(
                    { 0.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 1.0f })),
            canvas) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        canvas,
        Marionette::ComponentKindCanvas,
        make_canvas());
    (void)add_or_set_component(
        canvas,
        Marionette::ComponentKindUiRectTransform,
        make_ui_rect(
            { 0.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 0.0f },
            { 0.0f, 0.0f },
            { 0.0f, 0.0f }));
    uiCanvasEntity = canvas;

    uiTitleEntity = spawn_ui_text(
        "FlowTitle",
        -220.0f,
        150.0f,
        82u,
        20u,
        k_uiTitleColor);
    uiBodyEntity = spawn_ui_text(
        "FlowBody",
        -40.0f,
        280.0f,
        34u,
        21u,
        k_uiBodyColor);
    uiHudEntity = spawn_ui_text(
        "FlowHud",
        34.0f,
        84.0f,
        28u,
        22u,
        k_uiHudColor);
}

void GameManager::update_flow_ui()
{
    ensure_flow_ui();

    char body[512]{};
    char hud[256]{};
    switch (flowState)
    {
    case FlowState::Title:
        set_ui_text(
            uiTitleEntity,
            "BorroWing",
            82u,
            20u,
            k_uiTitleColor);
        set_ui_text(
            uiBodyEntity,
            "Press ENTER or SPACE to start\nWASD / Arrow: Move  Space: Gun  Right Mouse: Missile\nE: Convert Field  R: Infinite Missile",
            30u,
            21u,
            k_uiBodyColor);
        set_ui_text(uiHudEntity, "", 24u, 22u, k_uiHudColor);
        return;
    case FlowState::Playing:
        (void)std::snprintf(
            hud,
            sizeof(hud),
            "Score %d   Hull %d/%d   Armor %d   Salvage %d   Time %.0f/%.0f",
            score,
            playerHull,
            std::max(1, playerHullMax),
            armorCount,
            salvageCount,
            elapsedTime,
            std::max(1.0f, stageDuration));
        set_ui_text(uiTitleEntity, "", 24u, 20u, k_uiTitleColor);
        set_ui_text(uiBodyEntity, "", 24u, 21u, k_uiBodyColor);
        set_ui_text(uiHudEntity, hud, 26u, 22u, k_uiHudColor);
        return;
    case FlowState::GameOver:
        (void)std::snprintf(
            body,
            sizeof(body),
            "Score %d\nSurvival Time %.1f sec\nPress ENTER or SPACE to retry\nPress ESC for title",
            score,
            elapsedTime);
        set_ui_text(
            uiTitleEntity,
            "GAME OVER",
            76u,
            20u,
            k_uiWarningColor);
        set_ui_text(uiBodyEntity, body, 34u, 21u, k_uiBodyColor);
        set_ui_text(uiHudEntity, "", 24u, 22u, k_uiHudColor);
        return;
    case FlowState::Result:
        (void)std::snprintf(
            body,
            sizeof(body),
            "Score %d\nSalvage %d   Armor %d\nPress ENTER or SPACE to play again\nPress ESC for title",
            score,
            salvageCount,
            armorCount);
        set_ui_text(uiTitleEntity, "RESULT", 76u, 20u, k_uiResultColor);
        set_ui_text(uiBodyEntity, body, 34u, 21u, k_uiBodyColor);
        set_ui_text(uiHudEntity, "", 24u, 22u, k_uiHudColor);
        return;
    }
}

void GameManager::destroy_flow_ui()
{
    destroy_entity_safe(uiTitleEntity);
    destroy_entity_safe(uiBodyEntity);
    destroy_entity_safe(uiHudEntity);
    destroy_entity_safe(uiCanvasEntity);
    uiTitleEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiBodyEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiHudEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiCanvasEntity = CueEntityHandle{ k_cueInvalidHandleValue };
}

CueEntityHandle GameManager::spawn_ui_text(
    const char* a_name,
    float a_y,
    float a_height,
    uint32_t a_fontSize,
    uint32_t a_order,
    const Marionette::Color& a_color)
{
    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (uiCanvasEntity.value == k_cueInvalidHandleValue)
    {
        return entity;
    }

    if (spawn_object(
            make_spawn_desc(
                Marionette::SpawnObjectKindEmpty,
                a_name,
                "FlowUi",
                make_transform(
                    { 0.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 1.0f })),
            entity) != CueResult_Ok)
    {
        return CueEntityHandle{ k_cueInvalidHandleValue };
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindUiRectTransform,
        make_ui_rect(
            { 0.5f, 0.5f },
            { 0.5f, 0.5f },
            { 0.5f, 0.5f },
            { 0.0f, a_y },
            { 1500.0f, a_height }));
    set_ui_text(entity, "", a_fontSize, a_order, a_color);
    (void)set_parent(entity, uiCanvasEntity, false);
    return entity;
}

void GameManager::set_ui_text(
    CueEntityHandle a_entity,
    std::string_view a_text,
    uint32_t a_fontSize,
    uint32_t a_order,
    const Marionette::Color& a_color) const
{
    if (a_entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    (void)add_or_set_component(
        a_entity,
        Marionette::ComponentKindTextRenderer,
        make_text_renderer(a_text, a_fontSize, a_order, a_color));
}

void GameManager::register_player_hit(int a_scorePenalty)
{
    if (consume_armor())
    {
        return;
    }

    score = std::max(0, score - std::max(0, a_scorePenalty));
    playerHull = std::max(0, playerHull - 1);
    if (playerHull <= 0)
    {
        pendingGameOver = true;
    }
}

void GameManager::resolve_player()
{
    if (playerEntity.value != k_cueInvalidHandleValue)
    {
        Marionette::Transform transform{};
        if (get_transform(playerEntity, transform) == CueResult_Ok)
        {
            return;
        }
    }

    const std::vector<CueEntityHandle> players = find_entities_by_tag("Player");
    playerEntity = players.empty()
        ? CueEntityHandle{ k_cueInvalidHandleValue }
        : players.front();
}

void GameManager::configure_player_collider() const
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    (void)add_or_set_component(
        playerEntity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.65f, 0.3f, 1.25f }));
}

void GameManager::update_convert_field(float a_deltaTime)
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        deactivate_convert_field();
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        deactivate_convert_field();
        return;
    }

    const bool wantsField = push_key(Marionette::Key::E);
    if (!wantsField || fieldGauge <= 0.0f)
    {
        deactivate_convert_field();
        fieldGauge = std::min(
            std::max(0.0f, fieldMaxGauge),
            fieldGauge + std::max(0.0f, fieldRechargePerSecond) * a_deltaTime);
        return;
    }

    activate_convert_field(playerTransform);
    update_convert_field_transform(playerTransform);
    update_convert_field_targets(playerTransform);

    fieldGauge = std::max(
        0.0f,
        fieldGauge - std::max(0.0f, fieldDrainPerSecond) * a_deltaTime);
    if (fieldGauge <= 0.0f)
    {
        deactivate_convert_field();
    }
}

void GameManager::activate_convert_field(
    const Marionette::Transform& a_playerTransform)
{
    if (isConvertFieldActive &&
        convertFieldEntity.value != k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform transform = make_transform(
        a_playerTransform.position,
        {
            fieldHalfExtentX * 2.0f,
            fieldHalfExtentY * 2.0f,
            fieldHalfExtentZ * 2.0f,
        });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("ConvertField", "ConvertField", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 0u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ fieldHalfExtentX, fieldHalfExtentY, fieldHalfExtentZ }));
    (void)set_material_color(entity, k_convertFieldColor);

    convertFieldEntity = entity;
    isConvertFieldActive = true;
}

void GameManager::deactivate_convert_field()
{
    if (convertFieldEntity.value != k_cueInvalidHandleValue)
    {
        destroy_entity_safe(convertFieldEntity);
        convertFieldEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    }
    isConvertFieldActive = false;

    for (Salvage& salvage : salvages)
    {
        if (salvage.inConvertField)
        {
            clear_lock_visual(salvage.entity);
            salvage.inConvertField = false;
        }
    }

    for (EnemyMissile& missile : enemyMissiles)
    {
        set_lock_visual(missile.entity, k_enemyMissileColor);
    }

    for (LargeMissile& missile : largeMissiles)
    {
        set_lock_visual(missile.entity, k_largeMissileColor);
    }
}

void GameManager::update_convert_field_transform(
    const Marionette::Transform& a_playerTransform) const
{
    if (convertFieldEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform transform = make_transform(
        a_playerTransform.position,
        {
            fieldHalfExtentX * 2.0f,
            fieldHalfExtentY * 2.0f,
            fieldHalfExtentZ * 2.0f,
        });
    (void)set_transform(convertFieldEntity, transform);
}

void GameManager::update_convert_field_targets(
    const Marionette::Transform& a_playerTransform)
{
    for (EnemyMissile& missile : enemyMissiles)
    {
        if (missile.entity.value == k_cueInvalidHandleValue)
        {
            continue;
        }

        Marionette::Transform transform{};
        if (get_transform(missile.entity, transform) != CueResult_Ok)
        {
            missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        if (is_inside_convert_field(
                a_playerTransform.position,
                transform.position,
                missile.radius))
        {
            absorb_enemy_missile(missile);
        }
        else
        {
            (void)set_material_color(missile.entity, k_enemyMissileColor);
        }
    }

    for (LargeMissile& missile : largeMissiles)
    {
        if (missile.entity.value == k_cueInvalidHandleValue)
        {
            continue;
        }

        Marionette::Transform transform{};
        if (get_transform(missile.entity, transform) != CueResult_Ok)
        {
            missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        if (is_inside_convert_field(
                a_playerTransform.position,
                transform.position,
                missile.radius))
        {
            absorb_large_missile(missile);
        }
        else
        {
            (void)set_material_color(missile.entity, k_largeMissileColor);
        }
    }

    for (Salvage& salvage : salvages)
    {
        Marionette::Transform transform{};
        const bool isValid =
            get_transform(salvage.entity, transform) == CueResult_Ok;
        const bool isInside =
            isValid &&
            is_inside_convert_field(
                a_playerTransform.position,
                transform.position,
                salvage.radius);

        if (isInside)
        {
            absorb_salvage(salvage);
        }
        else if (salvage.inConvertField)
        {
            clear_lock_visual(salvage.entity);
            salvage.inConvertField = false;
        }
    }
}

void GameManager::absorb_enemy_missile(EnemyMissile& a_missile)
{
    if (a_missile.entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    destroy_entity_safe(a_missile.entity);
    a_missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
    ++reverseMissileAmmo;
    reverseMissileStateTimer = std::max(
        reverseMissileStateTimer,
        std::max(0.0f, reverseMissileStateDuration));
    score += 10;
}

void GameManager::absorb_large_missile(LargeMissile& a_missile)
{
    if (a_missile.entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    destroy_entity_safe(a_missile.entity);
    a_missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
    sonicStreamTimer = std::max(
        sonicStreamTimer,
        std::max(0.0f, sonicStreamDuration));
    score += 50;
}

void GameManager::absorb_salvage(Salvage& a_salvage)
{
    if (a_salvage.entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    destroy_entity_safe(a_salvage.entity);
    a_salvage.entity = CueEntityHandle{ k_cueInvalidHandleValue };
    a_salvage.inConvertField = false;
    ++armorCount;
    ++salvageCount;
    armorStateTimer = std::max(
        armorStateTimer,
        std::max(0.0f, armorStateDuration));
    score += 25;
}

bool GameManager::is_inside_convert_field(
    const CueFloat3& a_center,
    const CueFloat3& a_position,
    float a_radius) const
{
    return
        std::abs(a_position.x - a_center.x) <= fieldHalfExtentX + a_radius &&
        std::abs(a_position.y - a_center.y) <= fieldHalfExtentY + a_radius &&
        std::abs(a_position.z - a_center.z) <= fieldHalfExtentZ + a_radius;
}

bool GameManager::is_sonic_stream_active() const noexcept
{
    return sonicStreamTimer > 0.0f;
}

bool GameManager::is_armor_active() const noexcept
{
    return armorCount > 0;
}

bool GameManager::is_infinite_missile_ready() const noexcept
{
    return
        reverseMissileStateTimer > 0.0f &&
        sonicStreamTimer > 0.0f &&
        armorCount > 0;
}

bool GameManager::is_infinite_missile_active() const noexcept
{
    return infiniteMissileTimer > 0.0f;
}

void GameManager::update_infinite_missile(float)
{
    const bool isReady = is_infinite_missile_ready();
    if (isReady && !hasLoggedInfiniteReady)
    {
        log_info("Infinite missile READY.");
        hasLoggedInfiniteReady = true;
    }
    else if (!isReady)
    {
        hasLoggedInfiniteReady = false;
    }

    if (!isReady || is_infinite_missile_active())
    {
        return;
    }

    if (!push_key(Marionette::Key::R))
    {
        return;
    }

    infiniteMissileTimer = std::max(0.0f, infiniteMissileDuration);
    reverseMissileStateTimer = 0.0f;
    sonicStreamTimer = 0.0f;
    armorStateTimer = 0.0f;
    armorCount = 0;
    hasLoggedInfiniteReady = false;
    log_info("Infinite missile activated.");
}

void GameManager::update_player_status_visual()
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        isPlayerStatusVisualActive = false;
        return;
    }

    if (is_infinite_missile_active())
    {
        set_lock_visual(playerEntity, k_infiniteMissileColor);
        isPlayerStatusVisualActive = true;
        return;
    }

    if (is_armor_active())
    {
        set_lock_visual(playerEntity, k_armorColor);
        isPlayerStatusVisualActive = true;
        return;
    }

    if (is_sonic_stream_active())
    {
        set_lock_visual(playerEntity, k_sonicStreamColor);
        isPlayerStatusVisualActive = true;
        return;
    }

    if (isPlayerStatusVisualActive)
    {
        clear_lock_visual(playerEntity);
        isPlayerStatusVisualActive = false;
    }
}

bool GameManager::consume_armor()
{
    if (armorCount <= 0)
    {
        return false;
    }

    --armorCount;
    if (armorCount <= 0)
    {
        armorStateTimer = 0.0f;
    }
    else
    {
        armorStateTimer = std::max(
            armorStateTimer,
            std::max(0.0f, armorStateDuration));
    }
    return true;
}

void GameManager::update_lock_on(float)
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        lockedEnemies.clear();
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        lockedEnemies.clear();
        return;
    }

    lockedEnemies.erase(
        std::remove_if(
            lockedEnemies.begin(),
            lockedEnemies.end(),
            [this](CueEntityHandle a_entity)
            {
                Marionette::Transform transform{};
                return
                    a_entity.value == k_cueInvalidHandleValue ||
                    get_transform(a_entity, transform) != CueResult_Ok;
            }),
        lockedEnemies.end());

    for (const Enemy& enemy : enemies)
    {
        if (enemy.entity.value == k_cueInvalidHandleValue ||
            is_locked(enemy.entity) ||
            (enemy.kind == EnemyKind::BossCore && !bossCoreExposed))
        {
            continue;
        }

        Marionette::Transform enemyTransform{};
        if (get_transform(enemy.entity, enemyTransform) != CueResult_Ok)
        {
            continue;
        }

        if (is_enemy_in_lock_range(playerTransform, enemyTransform))
        {
            lockedEnemies.push_back(enemy.entity);
        }
    }
}

bool GameManager::is_enemy_in_lock_range(
    const Marionette::Transform& a_playerTransform,
    const Marionette::Transform& a_enemyTransform) const
{
    const float rangeMultiplier = is_infinite_missile_active()
        ? std::max(1.0f, infiniteLockRangeMultiplier)
        : 1.0f;
    const float dz =
        a_enemyTransform.position.z - a_playerTransform.position.z;
    if (dz <= 0.0f || dz > lockRange * rangeMultiplier)
    {
        return false;
    }

    const float dx =
        a_enemyTransform.position.x - a_playerTransform.position.x;
    const float dy =
        a_enemyTransform.position.y - a_playerTransform.position.y;
    return
        std::abs(dx) <= lockWidth * rangeMultiplier &&
        std::abs(dy) <= lockHeight * rangeMultiplier;
}

bool GameManager::is_locked(CueEntityHandle a_entity) const
{
    return std::any_of(
        lockedEnemies.begin(),
        lockedEnemies.end(),
        [a_entity](CueEntityHandle a_locked)
        {
            return a_locked.value == a_entity.value;
        });
}

void GameManager::update_lock_visuals()
{
    for (CueEntityHandle visualEntity : visualLockedEnemies)
    {
        if (!is_locked(visualEntity))
        {
            clear_lock_visual(visualEntity);
        }
    }

    for (CueEntityHandle lockedEntity : lockedEnemies)
    {
        set_lock_visual(lockedEntity, k_lockedEnemyColor);
    }

    visualLockedEnemies = lockedEnemies;
}

void GameManager::clear_lock_visual(CueEntityHandle a_entity) const
{
    if (a_entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    (void)clear_material_property_block(a_entity);
}

void GameManager::set_lock_visual(
    CueEntityHandle a_entity,
    const Marionette::Color& a_color) const
{
    if (a_entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    (void)set_material_color(a_entity, a_color);
}

void GameManager::update_combat(float)
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    const bool wantsMachineGun =
        push_key(Marionette::Key::Space) ||
        push_mouse_button(Marionette::MouseButton::Left);
    if (wantsMachineGun && machineGunTimer <= 0.0f)
    {
        spawn_machine_gun_bullet(playerTransform);
        machineGunTimer = std::max(0.01f, machineGunInterval);
    }

    const bool wantsMissile =
        push_mouse_button(Marionette::MouseButton::Right);
    if (!wantsMissile || missileFireTimer > 0.0f || lockedEnemies.empty())
    {
        return;
    }

    const bool fireInfiniteMissile = is_infinite_missile_active();
    for (CueEntityHandle lockedEnemy : lockedEnemies)
    {
        Marionette::Transform targetTransform{};
        if (lockedEnemy.value == k_cueInvalidHandleValue ||
            get_transform(lockedEnemy, targetTransform) != CueResult_Ok)
        {
            continue;
        }

        const bool fireReverseMissile =
            fireInfiniteMissile || reverseMissileAmmo > 0;
        if (!fireInfiniteMissile && fireReverseMissile)
        {
            --reverseMissileAmmo;
        }

        spawn_missile(playerTransform, lockedEnemy, fireReverseMissile);
    }

    float fireIntervalMultiplier = is_sonic_stream_active()
        ? std::clamp(sonicFireIntervalMultiplier, 0.1f, 1.0f)
        : 1.0f;
    if (fireInfiniteMissile)
    {
        fireIntervalMultiplier = std::min(
            fireIntervalMultiplier,
            std::clamp(infiniteFireIntervalMultiplier, 0.05f, 1.0f));
    }
    missileFireTimer = std::max(0.03f, fireInterval * fireIntervalMultiplier);
}

void GameManager::load_terrain_config()
{
    if (hasLoadedTerrainConfig)
    {
        return;
    }

    Marionette::JsonConfigHandle indexConfig{};
    if (load_json_config(
            "Terrain/GrandCanyonSegments/grand_canyon_segments.index.json",
            indexConfig) != CueResult_Ok)
    {
        log_info("Grand canyon terrain config was not loaded.");
        hasLoadedTerrainConfig = true;
        return;
    }

    int32_t segmentCount = 0;
    if (get_json_config_int(indexConfig, "segmentCount", segmentCount) !=
            CueResult_Ok ||
        segmentCount <= 0)
    {
        (void)unload_json_config(indexConfig);
        hasLoadedTerrainConfig = true;
        log_info("Grand canyon terrain config has no segments.");
        return;
    }

    terrainDefinitions.clear();
    terrainDefinitions.reserve(static_cast<size_t>(segmentCount));
    for (int32_t index = 0; index < segmentCount; ++index)
    {
        TerrainSegmentDef definition{};
        if (read_terrain_segment_definition(
                indexConfig,
                static_cast<uint32_t>(index),
                definition))
        {
            terrainDefinitions.push_back(std::move(definition));
        }
    }

    (void)unload_json_config(indexConfig);
    hasLoadedTerrainConfig = true;

    char message[160]{};
    (void)std::snprintf(
        message,
        sizeof(message),
        "Grand canyon terrain definitions loaded: %d",
        static_cast<int>(terrainDefinitions.size()));
    log_info(message);
}

void GameManager::unload_terrain_config()
{
    for (ActiveTerrainSegment& segment : activeTerrainSegments)
    {
        destroy_terrain_segment(segment);
    }
    activeTerrainSegments.clear();
    terrainDefinitions.clear();
    hasLoadedTerrainConfig = false;
}

bool GameManager::read_terrain_segment_definition(
    Marionette::JsonConfigHandle a_indexConfig,
    uint32_t a_index,
    TerrainSegmentDef& a_outDefinition)
{
    const std::string basePath =
        "segments[" + std::to_string(a_index) + "]";
    std::string metadataPath{};
    if (get_json_config_string(
            a_indexConfig,
            basePath + ".metadata",
            metadataPath) != CueResult_Ok)
    {
        return false;
    }

    Marionette::JsonConfigHandle segmentConfig{};
    if (load_json_config(
            "Terrain/GrandCanyonSegments/" + metadataPath,
            segmentConfig) != CueResult_Ok)
    {
        return false;
    }

    TerrainSegmentDef definition{};
    (void)get_json_config_string(segmentConfig, "id", definition.id);
    (void)get_json_config_string(
        segmentConfig,
        "visualModelName",
        definition.modelName);
    (void)get_json_config_float(segmentConfig, "length", definition.length);
    (void)get_json_config_bool(
        segmentConfig,
        "hasObstacles",
        definition.hasObstacles);

    int32_t proxyCount = 0;
    (void)get_json_config_int(
        segmentConfig,
        "collision.proxyCount",
        proxyCount);
    proxyCount = std::max(0, proxyCount);
    definition.proxies.reserve(static_cast<size_t>(proxyCount));
    for (int32_t proxyIndex = 0; proxyIndex < proxyCount; ++proxyIndex)
    {
        const std::string proxyPath =
            "collision.proxies[" + std::to_string(proxyIndex) + "]";
        TerrainProxyDef proxy{};
        (void)get_json_config_string(
            segmentConfig,
            proxyPath + ".name",
            proxy.name);
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".position[0]",
            proxy.position.x);
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".position[1]",
            proxy.position.y);
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".position[2]",
            proxy.position.z);

        CueFloat3 size{ 1.0f, 1.0f, 1.0f };
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".size[0]",
            size.x);
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".size[1]",
            size.y);
        (void)get_json_config_float(
            segmentConfig,
            proxyPath + ".size[2]",
            size.z);
        proxy.halfExtent = scale(size, 0.5f);
        definition.proxies.push_back(std::move(proxy));
    }

    (void)unload_json_config(segmentConfig);

    if (definition.id.empty() || definition.modelName.empty() ||
        definition.length <= 0.0f)
    {
        return false;
    }

    a_outDefinition = std::move(definition);
    return true;
}

void GameManager::update_terrain_segments(float a_deltaTime)
{
    if (terrainDefinitions.empty() ||
        playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    const float scrollDelta =
        std::max(0.0f, worldScrollSpeed) * a_deltaTime;
    if (scrollDelta > 0.0f)
    {
        for (ActiveTerrainSegment& segment : activeTerrainSegments)
        {
            segment.centerZ -= scrollDelta;
            translate_entity_z(segment.visualEntity, -scrollDelta);
            for (CueEntityHandle proxyEntity : segment.proxyEntities)
            {
                translate_entity_z(proxyEntity, -scrollDelta);
            }
        }
        terrainNextEntryZ -= scrollDelta;
    }

    constexpr float k_forwardTerrainDistance = 260.0f;
    constexpr float k_cleanupDistance = 120.0f;
    const float targetEntryZ =
        playerTransform.position.z + k_forwardTerrainDistance;
    while (terrainNextEntryZ < targetEntryZ)
    {
        const uint32_t definitionIndex = choose_terrain_segment_index();
        const TerrainSegmentDef& definition =
            terrainDefinitions[definitionIndex];
        const float centerZ = terrainNextEntryZ + definition.length * 0.5f;
        spawn_terrain_segment(definition, definitionIndex, centerZ);
        terrainNextEntryZ += definition.length;
    }

    const float cleanupZ = playerTransform.position.z - k_cleanupDistance;
    activeTerrainSegments.erase(
        std::remove_if(
            activeTerrainSegments.begin(),
            activeTerrainSegments.end(),
            [this, cleanupZ](ActiveTerrainSegment& a_segment)
            {
                if (a_segment.centerZ + a_segment.length * 0.5f >= cleanupZ)
                {
                    return false;
                }

                destroy_terrain_segment(a_segment);
                return true;
            }),
        activeTerrainSegments.end());
}

void GameManager::translate_entity_z(
    CueEntityHandle a_entity,
    float a_deltaZ) const
{
    if (a_entity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform transform{};
    if (get_transform(a_entity, transform) != CueResult_Ok)
    {
        return;
    }

    transform.position.z += a_deltaZ;
    (void)set_transform(a_entity, transform);
}

uint32_t GameManager::choose_terrain_segment_index() const noexcept
{
    if (terrainDefinitions.empty())
    {
        return 0u;
    }

    uint32_t state = spawnIndex * 1664525u + 1013904223u;
    state ^= static_cast<uint32_t>(activeTerrainSegments.size() * 747796405u);
    return state % static_cast<uint32_t>(terrainDefinitions.size());
}

void GameManager::spawn_terrain_segment(
    const TerrainSegmentDef& a_definition,
    uint32_t a_definitionIndex,
    float a_centerZ)
{
    Marionette::Transform transform = make_transform(
        { 0.0f, 0.0f, a_centerZ },
        { 1.0f, 1.0f, 1.0f });

    CueEntityHandle visualEntity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc(
                Marionette::SpawnObjectKindStaticMesh,
                a_definition.id,
                "Terrain",
                transform),
            visualEntity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        visualEntity,
        Marionette::ComponentKindMeshFilter,
        make_mesh_filter(a_definition.modelName));
    (void)add_or_set_component(
        visualEntity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(1u, 1u));

    ActiveTerrainSegment segment{};
    segment.visualEntity = visualEntity;
    segment.definitionIndex = a_definitionIndex;
    segment.centerZ = a_centerZ;
    segment.length = a_definition.length;
    for (const TerrainProxyDef& proxy : a_definition.proxies)
    {
        Marionette::Transform proxyTransform = make_transform(
            {
                proxy.position.x,
                proxy.position.y,
                a_centerZ + proxy.position.z,
            },
            {
                proxy.halfExtent.x * 2.0f,
                proxy.halfExtent.y * 2.0f,
                proxy.halfExtent.z * 2.0f,
            });

        CueEntityHandle proxyEntity{ k_cueInvalidHandleValue };
        if (spawn_object(
                make_spawn_desc(
                    Marionette::SpawnObjectKindEmpty,
                    proxy.name,
                    "TerrainCollision",
                    proxyTransform),
                proxyEntity) != CueResult_Ok)
        {
            continue;
        }

        (void)add_or_set_component(
            proxyEntity,
            Marionette::ComponentKindCollider,
            make_box_collider(proxy.halfExtent, false));
        segment.proxyEntities.push_back(proxyEntity);
    }

    activeTerrainSegments.push_back(std::move(segment));
}

void GameManager::destroy_terrain_segment(
    ActiveTerrainSegment& a_segment) const
{
    destroy_entity_safe(a_segment.visualEntity);
    a_segment.visualEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    for (CueEntityHandle proxyEntity : a_segment.proxyEntities)
    {
        destroy_entity_safe(proxyEntity);
    }
    a_segment.proxyEntities.clear();
}

void GameManager::update_terrain_collisions(float)
{
    if (terrainHitCooldown > 0.0f ||
        playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    constexpr CueFloat3 k_playerHalfExtent{ 0.65f, 0.3f, 1.25f };
    for (const ActiveTerrainSegment& segment : activeTerrainSegments)
    {
        if (segment.definitionIndex >= terrainDefinitions.size())
        {
            continue;
        }

        const TerrainSegmentDef& definition =
            terrainDefinitions[segment.definitionIndex];
        for (const TerrainProxyDef& proxy : definition.proxies)
        {
            const CueFloat3 proxyCenter{
                proxy.position.x,
                proxy.position.y,
                segment.centerZ + proxy.position.z
            };
            if (!aabb_overlap(
                    playerTransform.position,
                    k_playerHalfExtent,
                    proxyCenter,
                    proxy.halfExtent))
            {
                continue;
            }

            register_player_hit(50);
            terrainHitCooldown = 1.0f;
            log_info("Player hit grand canyon terrain.");
            return;
        }
    }
}

void GameManager::update_spawning(float a_deltaTime)
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    enemySpawnTimer -= a_deltaTime;
    normalBulletSpawnTimer -= a_deltaTime;
    enemyMissileSpawnTimer -= a_deltaTime;
    largeMissileSpawnTimer -= a_deltaTime;
    salvageSpawnTimer -= a_deltaTime;

    if (stagePhase == StagePhase::Boss)
    {
        if (!hasSpawnedBoss)
        {
            spawn_boss(playerTransform.position.z);
            hasSpawnedBoss = true;
        }

        bossAttackTimer -= a_deltaTime;
        if (bossAttackTimer <= 0.0f)
        {
            run_boss_attack_pattern(playerTransform);
        }
        return;
    }

    const uint8_t phaseValue = static_cast<uint8_t>(stagePhase);
    const bool spawnEnemies =
        phaseValue >= static_cast<uint8_t>(StagePhase::EnemyFormation);
    const bool spawnSmallMissiles =
        phaseValue >= static_cast<uint8_t>(StagePhase::SmallMissileIntro);
    const bool spawnSalvage =
        phaseValue >= static_cast<uint8_t>(StagePhase::SalvageIntro);
    const bool spawnLargeMissiles =
        phaseValue >= static_cast<uint8_t>(StagePhase::LargeMissileIntro);
    const bool spawnNormalBullets =
        phaseValue >= static_cast<uint8_t>(StagePhase::EnemyFormation);

    if (spawnEnemies && enemySpawnTimer <= 0.0f && enemies.size() < 12u)
    {
        if (phaseValue >= static_cast<uint8_t>(StagePhase::LargeMissileIntro) &&
            spawnIndex % 5u == 0u)
        {
            spawn_enemy_kind(EnemyKind::HeavyCarrier, playerTransform.position.z);
        }
        else if (
            phaseValue >= static_cast<uint8_t>(StagePhase::SmallMissileIntro) &&
            spawnIndex % 3u == 0u)
        {
            spawn_enemy_kind(
                EnemyKind::MissileCarrier,
                playerTransform.position.z);
        }
        else
        {
            spawn_enemy(playerTransform.position.z);
        }
        enemySpawnTimer = std::max(0.2f, enemySpawnInterval);
    }
    if (spawnNormalBullets &&
        normalBulletSpawnTimer <= 0.0f &&
        enemyBullets.size() < 12u)
    {
        spawn_enemy_bullet(playerTransform);
        normalBulletSpawnTimer = std::max(0.35f, normalBulletSpawnInterval);
    }
    if (spawnSmallMissiles &&
        enemyMissileSpawnTimer <= 0.0f &&
        enemyMissiles.size() < 14u)
    {
        spawn_enemy_missile(playerTransform);
        enemyMissileSpawnTimer = std::max(0.25f, enemyMissileSpawnInterval);
    }
    if (spawnLargeMissiles &&
        largeMissileSpawnTimer <= 0.0f &&
        largeMissiles.size() < 4u)
    {
        spawn_large_missile(playerTransform);
        largeMissileSpawnTimer = std::max(1.0f, largeMissileSpawnInterval);
    }
    if (spawnSalvage && salvageSpawnTimer <= 0.0f && salvages.size() < 8u)
    {
        spawn_salvage(playerTransform.position.z);
        salvageSpawnTimer = std::max(0.5f, salvageSpawnInterval);
    }
}

void GameManager::update_missiles(float a_deltaTime)
{
    for (Missile& missile : missiles)
    {
        missile.age += a_deltaTime;
        Marionette::Transform transform{};
        if (get_transform(missile.entity, transform) != CueResult_Ok)
        {
            missile.age = missile.lifeTime + 1.0f;
            continue;
        }

        if (missile.target.value != k_cueInvalidHandleValue)
        {
            Marionette::Transform targetTransform{};
            if (get_transform(missile.target, targetTransform) == CueResult_Ok)
            {
                const CueFloat3 desiredDirection = normalize_or_forward(
                    subtract(targetTransform.position, transform.position));
                const float sonicTurnMultiplier = is_sonic_stream_active()
                    ? std::max(1.0f, sonicMissileTurnMultiplier)
                    : 1.0f;
                const float turnBlend =
                    std::clamp(
                        missile.turnSpeed * sonicTurnMultiplier * a_deltaTime,
                        0.0f,
                        1.0f);
                missile.direction = blend_direction(
                    missile.direction,
                    desiredDirection,
                    turnBlend);
            }
            else
            {
                missile.target = CueEntityHandle{ k_cueInvalidHandleValue };
            }
        }

        transform.position = add(
            transform.position,
            scale(
                normalize_or_forward(missile.direction),
                missile.speed * a_deltaTime));
        (void)set_transform(missile.entity, transform);

        for (Enemy& enemy : enemies)
        {
            if (enemy.entity.value == k_cueInvalidHandleValue)
            {
                continue;
            }

            Marionette::Transform enemyTransform{};
            if (get_transform(enemy.entity, enemyTransform) != CueResult_Ok)
            {
                enemy.entity = CueEntityHandle{ k_cueInvalidHandleValue };
                continue;
            }

            const float hitRadius = enemy.radius + missile.radius;
            if (distance_sq(transform.position, enemyTransform.position) <=
                hitRadius * hitRadius)
            {
                destroy_entity_safe(missile.entity);
                missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
                if (enemy.kind == EnemyKind::BossCore && !bossCoreExposed)
                {
                    break;
                }

                enemy.hp -= missile.damage;
                if (enemy.hp <= 0)
                {
                    spawn_salvage_at(enemyTransform.position);
                    destroy_entity_safe(enemy.entity);
                    enemy.entity = CueEntityHandle{ k_cueInvalidHandleValue };
                    if (enemy.kind == EnemyKind::BossPart)
                    {
                        ++bossPartsDestroyed;
                        score += 500;
                        if (bossPartsDestroyed >= 2)
                        {
                            bossCoreExposed = true;
                            for (const Enemy& candidate : enemies)
                            {
                                if (candidate.kind == EnemyKind::BossCore &&
                                    candidate.entity.value !=
                                        k_cueInvalidHandleValue)
                                {
                                    (void)set_material_color(
                                        candidate.entity,
                                        k_bossColor);
                                }
                            }
                            log_info("Boss core exposed.");
                        }
                    }
                    else if (enemy.kind == EnemyKind::BossCore)
                    {
                        score += 2500;
                        pendingResult = true;
                    }
                    else
                    {
                        score += 100;
                    }
                }
                break;
            }
        }
    }

    missiles.erase(
        std::remove_if(
            missiles.begin(),
            missiles.end(),
            [this](const Missile& a_missile)
            {
                if (a_missile.entity.value == k_cueInvalidHandleValue)
                {
                    return true;
                }
                if (a_missile.age > a_missile.lifeTime)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        missiles.end());
}

void GameManager::update_enemy_bullets(float a_deltaTime)
{
    for (EnemyBullet& bullet : enemyBullets)
    {
        Marionette::Transform transform{};
        if (get_transform(bullet.entity, transform) != CueResult_Ok)
        {
            bullet.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        bullet.age += a_deltaTime;
        transform.position = add(
            transform.position,
            scale(
                normalize_or_forward(bullet.direction),
                normalBulletSpeed * a_deltaTime));
        transform.position.z -=
            std::max(0.0f, worldScrollSpeed) * a_deltaTime;
        (void)set_transform(bullet.entity, transform);

        if (playerEntity.value == k_cueInvalidHandleValue)
        {
            continue;
        }

        Marionette::Transform playerTransform{};
        if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
        {
            continue;
        }

        constexpr float k_playerRadius = 1.0f;
        const float hitRadius = bullet.radius + k_playerRadius;
        if (distance_sq(transform.position, playerTransform.position) <=
            hitRadius * hitRadius)
        {
            register_player_hit(20);
            destroy_entity_safe(bullet.entity);
            bullet.entity = CueEntityHandle{ k_cueInvalidHandleValue };
        }
    }

    enemyBullets.erase(
        std::remove_if(
            enemyBullets.begin(),
            enemyBullets.end(),
            [this](const EnemyBullet& a_bullet)
            {
                if (a_bullet.entity.value == k_cueInvalidHandleValue)
                {
                    return true;
                }
                if (a_bullet.age > 5.0f)
                {
                    destroy_entity_safe(a_bullet.entity);
                    return true;
                }
                return false;
            }),
        enemyBullets.end());
}

void GameManager::update_enemy_missiles(float a_deltaTime)
{
    for (EnemyMissile& missile : enemyMissiles)
    {
        Marionette::Transform transform{};
        if (get_transform(missile.entity, transform) != CueResult_Ok)
        {
            missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        missile.age += a_deltaTime;
        transform.position = add(
            transform.position,
            scale(
                normalize_or_forward(missile.direction),
                enemyMissileSpeed * a_deltaTime));
        transform.position.z -=
            std::max(0.0f, worldScrollSpeed) * a_deltaTime;
        transform.rotation.z += 2.4f * a_deltaTime;
        (void)set_transform(missile.entity, transform);

        if (playerEntity.value != k_cueInvalidHandleValue)
        {
            Marionette::Transform playerTransform{};
            if (get_transform(playerEntity, playerTransform) == CueResult_Ok)
            {
                constexpr float k_playerRadius = 1.0f;
                const float hitRadius = missile.radius + k_playerRadius;
                if (distance_sq(transform.position, playerTransform.position) <=
                    hitRadius * hitRadius)
                {
                    register_player_hit(25);
                    destroy_entity_safe(missile.entity);
                    missile.entity =
                        CueEntityHandle{ k_cueInvalidHandleValue };
                }
            }
        }
    }

    enemyMissiles.erase(
        std::remove_if(
            enemyMissiles.begin(),
            enemyMissiles.end(),
            [this](const EnemyMissile& a_missile)
            {
                if (a_missile.entity.value == k_cueInvalidHandleValue)
                {
                    return true;
                }

                if (a_missile.age > 5.0f)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        enemyMissiles.end());

}

void GameManager::update_large_missiles(float a_deltaTime)
{
    for (LargeMissile& missile : largeMissiles)
    {
        Marionette::Transform transform{};
        if (get_transform(missile.entity, transform) != CueResult_Ok)
        {
            missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        missile.age += a_deltaTime;
        transform.position = add(
            transform.position,
            scale(
                normalize_or_forward(missile.direction),
                largeMissileSpeed * a_deltaTime));
        transform.position.z -=
            std::max(0.0f, worldScrollSpeed) * a_deltaTime;
        transform.rotation.z += 0.8f * a_deltaTime;
        (void)set_transform(missile.entity, transform);

        if (playerEntity.value != k_cueInvalidHandleValue)
        {
            Marionette::Transform playerTransform{};
            if (get_transform(playerEntity, playerTransform) == CueResult_Ok)
            {
                constexpr float k_playerRadius = 1.0f;
                const float hitRadius = missile.radius + k_playerRadius;
                if (distance_sq(transform.position, playerTransform.position) <=
                    hitRadius * hitRadius)
                {
                    register_player_hit(50);
                    destroy_entity_safe(missile.entity);
                    missile.entity =
                        CueEntityHandle{ k_cueInvalidHandleValue };
                }
            }
        }
    }

    largeMissiles.erase(
        std::remove_if(
            largeMissiles.begin(),
            largeMissiles.end(),
            [this](const LargeMissile& a_missile)
            {
                if (a_missile.entity.value == k_cueInvalidHandleValue)
                {
                    return true;
                }

                if (a_missile.age > 8.0f)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        largeMissiles.end());
}

void GameManager::update_enemies(float a_deltaTime)
{
    for (Enemy& enemy : enemies)
    {
        Marionette::Transform transform{};
        if (get_transform(enemy.entity, transform) != CueResult_Ok)
        {
            enemy.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        if (enemy.kind == EnemyKind::BossPart ||
            enemy.kind == EnemyKind::BossCore)
        {
            transform.position.z = std::max(
                34.0f,
                transform.position.z -
                    std::max(0.0f, worldScrollSpeed) * 0.35f * a_deltaTime);
            transform.rotation.z += 0.25f * a_deltaTime;
        }
        else
        {
            transform.position.z -=
                (std::max(0.0f, worldScrollSpeed) + 5.0f) * a_deltaTime;
            transform.rotation.z += 1.1f * a_deltaTime;
        }
        (void)set_transform(enemy.entity, transform);
    }

    enemies.erase(
        std::remove_if(
            enemies.begin(),
            enemies.end(),
            [](const Enemy& a_enemy)
            {
                return a_enemy.entity.value == k_cueInvalidHandleValue;
            }),
        enemies.end());
}

void GameManager::update_salvage()
{
    const float scrollDelta =
        std::max(0.0f, worldScrollSpeed) * delta_time();
    for (Salvage& salvage : salvages)
    {
        Marionette::Transform transform{};
        if (get_transform(salvage.entity, transform) != CueResult_Ok)
        {
            salvage.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

        transform.position.z -= scrollDelta;
        transform.rotation.z += 0.035f;
        (void)set_transform(salvage.entity, transform);
    }

    salvages.erase(
        std::remove_if(
            salvages.begin(),
            salvages.end(),
            [](const Salvage& a_salvage)
            {
                return a_salvage.entity.value == k_cueInvalidHandleValue;
            }),
        salvages.end());
}

void GameManager::cleanup_behind_player()
{
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    const float cleanupZ = playerTransform.position.z - 18.0f;
    enemies.erase(
        std::remove_if(
            enemies.begin(),
            enemies.end(),
            [this, cleanupZ](const Enemy& a_enemy)
            {
                if (a_enemy.kind == EnemyKind::BossPart ||
                    a_enemy.kind == EnemyKind::BossCore)
                {
                    return false;
                }
                Marionette::Transform transform{};
                if (get_transform(a_enemy.entity, transform) != CueResult_Ok ||
                    transform.position.z < cleanupZ)
                {
                    destroy_entity_safe(a_enemy.entity);
                    return true;
                }
                return false;
            }),
        enemies.end());

    enemyBullets.erase(
        std::remove_if(
            enemyBullets.begin(),
            enemyBullets.end(),
            [this, cleanupZ](const EnemyBullet& a_bullet)
            {
                Marionette::Transform transform{};
                if (get_transform(a_bullet.entity, transform) != CueResult_Ok ||
                    transform.position.z < cleanupZ)
                {
                    destroy_entity_safe(a_bullet.entity);
                    return true;
                }
                return false;
            }),
        enemyBullets.end());

    salvages.erase(
        std::remove_if(
            salvages.begin(),
            salvages.end(),
            [this, cleanupZ](const Salvage& a_salvage)
            {
                Marionette::Transform transform{};
                if (get_transform(a_salvage.entity, transform) != CueResult_Ok ||
                    transform.position.z < cleanupZ)
                {
                    destroy_entity_safe(a_salvage.entity);
                    return true;
                }
                return false;
            }),
        salvages.end());

    enemyMissiles.erase(
        std::remove_if(
            enemyMissiles.begin(),
            enemyMissiles.end(),
            [this, cleanupZ](const EnemyMissile& a_missile)
            {
                Marionette::Transform transform{};
                if (get_transform(a_missile.entity, transform) != CueResult_Ok ||
                    transform.position.z < cleanupZ)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        enemyMissiles.end());

    largeMissiles.erase(
        std::remove_if(
            largeMissiles.begin(),
            largeMissiles.end(),
            [this, cleanupZ](const LargeMissile& a_missile)
            {
                Marionette::Transform transform{};
                if (get_transform(a_missile.entity, transform) != CueResult_Ok ||
                    transform.position.z < cleanupZ)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        largeMissiles.end());
}

void GameManager::spawn_machine_gun_bullet(
    const Marionette::Transform& a_playerTransform)
{
    Marionette::Transform transform = make_transform(
        {
            a_playerTransform.position.x,
            a_playerTransform.position.y,
            a_playerTransform.position.z + 1.75f,
        },
        { 0.12f, 0.12f, 0.75f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("MachineGun", "MachineGun", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 0u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.08f, 0.08f, 0.36f }));
    (void)set_material_color(entity, k_machineGunColor);

    missiles.push_back(Missile{
        entity,
        CueEntityHandle{ k_cueInvalidHandleValue },
        { 0.0f, 0.0f, 1.0f },
        0.18f,
        0.0f,
        machineGunSpeed,
        machineGunLifeTime,
        1,
        0.0f });
}

void GameManager::spawn_missile(
    const Marionette::Transform& a_playerTransform,
    CueEntityHandle a_target,
    bool a_isReverse)
{
    Marionette::Transform transform = make_transform(
        {
            a_playerTransform.position.x,
            a_playerTransform.position.y,
            a_playerTransform.position.z + 2.1f,
        },
        { 0.22f, 0.22f, 1.4f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("Missile", "Missile", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindMeshFilter,
        make_mesh_filter("missile"));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 0u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.18f, 0.18f, 0.7f }));
    const bool isInfinite = is_infinite_missile_active();
    (void)set_material_color(
        entity,
        isInfinite
            ? k_infiniteMissileColor
            : (a_isReverse ? k_reverseMissileColor : k_playerMissileColor));

    missiles.push_back(Missile{
        entity,
        a_target,
        { 0.0f, 0.0f, 1.0f },
        isInfinite ? 0.75f : (a_isReverse ? 0.55f : 0.35f),
        isInfinite ? reverseMissileTurnSpeed * 1.35f :
            (a_isReverse ? reverseMissileTurnSpeed : missileTurnSpeed),
        missileSpeed,
        missileLifeTime,
        isInfinite ? 3 : (a_isReverse ? 2 : 1),
        0.0f });
}

void GameManager::spawn_enemy(float a_playerZ)
{
    spawn_enemy_kind(EnemyKind::Fighter, a_playerZ);
}

void GameManager::spawn_enemy_kind(EnemyKind a_kind, float a_playerZ)
{
    const float laneX =
        static_cast<float>(static_cast<int>(spawnIndex % 5u) - 2) * 2.2f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex / 2u) % 3u) - 1) *
        1.15f;
    ++spawnIndex;

    CueFloat3 scale{ 1.25f, 0.75f, 1.25f };
    float radius = 1.05f;
    int hp = 1;
    Marionette::Color color = k_lockedEnemyColor;
    const char* name = "Enemy";
    const char* tag = "Enemy";
    if (a_kind == EnemyKind::MissileCarrier)
    {
        scale = { 1.45f, 0.85f, 1.55f };
        radius = 1.25f;
        hp = 2;
        color = k_missileCarrierColor;
        name = "MissileCarrier";
        tag = "MissileCarrier";
    }
    else if (a_kind == EnemyKind::HeavyCarrier)
    {
        scale = { 1.9f, 1.05f, 2.0f };
        radius = 1.55f;
        hp = 3;
        color = k_heavyCarrierColor;
        name = "HeavyCarrier";
        tag = "HeavyCarrier";
    }

    Marionette::Transform transform = make_transform(
        { laneX, laneY, a_playerZ + spawnLeadDistance },
        scale);

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc(name, tag, transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindMeshFilter,
        make_mesh_filter("f16"));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(1u, 1u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.7f, 0.7f, 0.7f }));
    if (a_kind != EnemyKind::Fighter)
    {
        (void)set_material_color(entity, color);
    }
    enemies.push_back(Enemy{ entity, radius, hp, a_kind });
}

void GameManager::spawn_boss(float a_playerZ)
{
    const float bossZ = a_playerZ + spawnLeadDistance + 18.0f;
    const auto spawnBossObject =
        [this](
            const char* a_name,
            const CueFloat3& a_position,
            const CueFloat3& a_scale,
            const CueFloat3& a_colliderHalfExtent,
            float a_radius,
            int a_hp,
            EnemyKind a_kind,
            const Marionette::Color& a_color)
        {
            CueEntityHandle entity{ k_cueInvalidHandleValue };
            if (spawn_object(
                    make_spawn_desc(
                        a_name,
                        a_kind == EnemyKind::BossCore ? "BossCore" : "BossPart",
                        make_transform(a_position, a_scale)),
                    entity) != CueResult_Ok)
            {
                return;
            }

            (void)add_or_set_component(
                entity,
                Marionette::ComponentKindMeshFilter,
                make_mesh_filter("f16"));
            (void)add_or_set_component(
                entity,
                Marionette::ComponentKindStaticMeshRenderer,
                make_renderer(1u, 1u));
            (void)add_or_set_component(
                entity,
                Marionette::ComponentKindCollider,
                make_box_trigger(a_colliderHalfExtent));
            (void)set_material_color(entity, a_color);
            enemies.push_back(Enemy{ entity, a_radius, a_hp, a_kind });
        };

    spawnBossObject(
        "BossLeftWing",
        { -3.1f, 0.0f, bossZ - 1.0f },
        { 2.6f, 0.75f, 2.8f },
        { 1.8f, 0.55f, 1.9f },
        2.2f,
        12,
        EnemyKind::BossPart,
        k_bossPartColor);
    spawnBossObject(
        "BossRightWing",
        { 3.1f, 0.0f, bossZ - 1.0f },
        { 2.6f, 0.75f, 2.8f },
        { 1.8f, 0.55f, 1.9f },
        2.2f,
        12,
        EnemyKind::BossPart,
        k_bossPartColor);
    spawnBossObject(
        "BossCore",
        { 0.0f, 0.15f, bossZ },
        { 3.1f, 1.25f, 4.4f },
        { 2.2f, 0.9f, 3.1f },
        3.3f,
        32,
        EnemyKind::BossCore,
        k_bossCoreLockedColor);

    bossCoreExposed = false;
    bossPartsDestroyed = 0;
    bossAttackStep = 0;
    bossAttackTimer = 1.0f;
}

void GameManager::run_boss_attack_pattern(
    const Marionette::Transform& a_playerTransform)
{
    switch (bossAttackStep % 4u)
    {
    case 0u:
        spawn_enemy_bullet(a_playerTransform);
        spawn_enemy_bullet(a_playerTransform);
        bossAttackTimer = 0.75f;
        break;
    case 1u:
        spawn_enemy_missile(a_playerTransform);
        spawn_enemy_missile(a_playerTransform);
        bossAttackTimer = 1.15f;
        break;
    case 2u:
        spawn_large_missile(a_playerTransform);
        bossAttackTimer = 1.7f;
        break;
    default:
        spawn_enemy_bullet(a_playerTransform);
        spawn_enemy_missile(a_playerTransform);
        if (bossCoreExposed)
        {
            spawn_large_missile(a_playerTransform);
        }
        bossAttackTimer = bossCoreExposed ? 0.9f : 1.25f;
        break;
    }
    ++bossAttackStep;
}

void GameManager::spawn_enemy_bullet(
    const Marionette::Transform& a_playerTransform)
{
    const float laneX =
        static_cast<float>(static_cast<int>((spawnIndex + 5u) % 5u) - 2) *
        1.5f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex + 4u) % 3u) - 1) *
        0.9f;
    ++spawnIndex;

    Marionette::Transform transform = make_transform(
        {
            laneX,
            laneY,
            a_playerTransform.position.z + spawnLeadDistance * 0.45f,
        },
        { 0.34f, 0.34f, 0.34f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("EnemyNormalBullet", "EnemyBullet", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 0u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.22f, 0.22f, 0.22f }));
    (void)set_material_color(entity, k_enemyBulletColor);

    enemyBullets.push_back(EnemyBullet{
        entity,
        normalize_or_forward(
            subtract(a_playerTransform.position, transform.position)),
        0.32f,
        0.0f });
}

void GameManager::spawn_enemy_missile(
    const Marionette::Transform& a_playerTransform)
{
    const float laneX =
        static_cast<float>(static_cast<int>((spawnIndex + 3u) % 5u) - 2) *
        1.8f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex + 1u) % 3u) - 1) *
        1.0f;

    Marionette::Transform transform = make_transform(
        {
            laneX,
            laneY,
            a_playerTransform.position.z + spawnLeadDistance * 0.55f,
        },
        { 0.28f, 0.28f, 1.3f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("EnemySmallMissile", "EnemyMissile", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindMeshFilter,
        make_mesh_filter("missile"));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 0u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.18f, 0.18f, 0.65f }));
    (void)set_material_color(entity, k_enemyMissileColor);

    enemyMissiles.push_back(EnemyMissile{
        entity,
        normalize_or_forward(
            subtract(a_playerTransform.position, transform.position)),
        0.35f,
        0.0f });
}

void GameManager::spawn_large_missile(
    const Marionette::Transform& a_playerTransform)
{
    const float laneX =
        static_cast<float>(static_cast<int>((spawnIndex + 4u) % 5u) - 2) *
        1.5f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex + 5u) % 3u) - 1) *
        0.9f;

    Marionette::Transform transform = make_transform(
        {
            laneX,
            laneY,
            a_playerTransform.position.z + spawnLeadDistance * 0.82f,
        },
        { 0.85f, 0.85f, 2.6f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("EnemyLargeMissile", "LargeMissile", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindMeshFilter,
        make_mesh_filter("missile"));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(1u, 1u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.45f, 0.45f, 1.3f }));
    (void)set_material_color(entity, k_largeMissileColor);

    largeMissiles.push_back(LargeMissile{
        entity,
        normalize_or_forward(
            subtract(a_playerTransform.position, transform.position)),
        0.95f,
        0.0f });
}

void GameManager::spawn_salvage(float a_playerZ)
{
    const float laneX =
        static_cast<float>(static_cast<int>((spawnIndex + 1u) % 5u) - 2) *
        1.8f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex + 2u) % 3u) - 1) *
        1.0f;

    spawn_salvage_at({ laneX, laneY, a_playerZ + spawnLeadDistance * 0.72f });
}

void GameManager::spawn_salvage_at(const CueFloat3& a_position)
{
    Marionette::Transform transform = make_transform(
        a_position,
        { 0.7f, 0.7f, 0.7f });
    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("Salvage", "Salvage", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(0u, 1u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.45f, 0.45f, 0.45f }));
    (void)set_material_color(entity, k_salvageColor);
    salvages.push_back(Salvage{ entity, 0.9f });
}

void GameManager::destroy_entity_safe(CueEntityHandle a_entity) const
{
    if (a_entity.value != k_cueInvalidHandleValue)
    {
        (void)destroy_entity(a_entity);
    }
}

void GameManager::log_progress()
{
    if (progressLogTimer > 0.0f)
    {
        return;
    }

    char message[320]{};
    (void)std::snprintf(
        message,
        sizeof(message),
        "Score %d / Salvage %d / Armor %d / ArmorTimer %.1f / Reverse %d / ReverseTimer %.1f / SonicTimer %.1f / Ready %s / InfiniteTimer %.1f / Time %.1f",
        score,
        salvageCount,
        armorCount,
        armorStateTimer,
        reverseMissileAmmo,
        reverseMissileStateTimer,
        sonicStreamTimer,
        is_infinite_missile_ready() ? "YES" : "NO",
        infiniteMissileTimer,
        elapsedTime);
    log_info(message);
    progressLogTimer = 5.0f;
}

MARIONETTE_DEFINE_SCRIPT(game_manager, GameManager);
