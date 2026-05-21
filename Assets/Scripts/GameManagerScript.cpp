#include "GameManagerScript.h"

// === C++ includes ===
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace
{
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
    constexpr Marionette::Color k_largeMissileColor{
        0.78f, 0.18f, 1.0f, 1.0f
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

    [[nodiscard]] Marionette::StaticMeshRendererComponentData
    make_renderer(uint8_t a_castsShadow, uint8_t a_receivesShadow) noexcept
    {
        Marionette::StaticMeshRendererComponentData renderer{};
        renderer.visible = 1u;
        renderer.castsShadow = a_castsShadow;
        renderer.receivesShadow = a_receivesShadow;
        return renderer;
    }

    [[nodiscard]] Marionette::ColliderComponentData make_box_trigger(
        const CueFloat3& a_halfExtent) noexcept
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
        collider.isTrigger = 1u;
        return collider;
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
    (void)read_float(a_reader, "spawnLeadDistance", spawnLeadDistance);
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
}

void GameManager::update()
{
    const float dt = delta_time();
    elapsedTime += dt;
    machineGunTimer = std::max(0.0f, machineGunTimer - dt);
    missileFireTimer = std::max(0.0f, missileFireTimer - dt);
    reverseMissileStateTimer =
        std::max(0.0f, reverseMissileStateTimer - dt);
    sonicStreamTimer = std::max(0.0f, sonicStreamTimer - dt);
    armorStateTimer = std::max(0.0f, armorStateTimer - dt);
    infiniteMissileTimer = std::max(0.0f, infiniteMissileTimer - dt);
    progressLogTimer = std::max(0.0f, progressLogTimer - dt);

    resolve_player();
    update_convert_field(dt);
    update_infinite_missile(dt);
    update_player_status_visual();
    update_lock_on(dt);
    update_lock_visuals();
    update_combat(dt);
    update_spawning(dt);
    update_missiles(dt);
    update_enemy_missiles(dt);
    update_large_missiles(dt);
    update_enemies(dt);
    update_salvage();
    cleanup_behind_player();
    log_progress();
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
            is_locked(enemy.entity))
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
    enemyMissileSpawnTimer -= a_deltaTime;
    largeMissileSpawnTimer -= a_deltaTime;
    salvageSpawnTimer -= a_deltaTime;
    if (enemySpawnTimer <= 0.0f && enemies.size() < 12u)
    {
        spawn_enemy(playerTransform.position.z);
        enemySpawnTimer = std::max(0.2f, enemySpawnInterval);
    }
    if (enemyMissileSpawnTimer <= 0.0f && enemyMissiles.size() < 14u)
    {
        spawn_enemy_missile(playerTransform);
        enemyMissileSpawnTimer = std::max(0.25f, enemyMissileSpawnInterval);
    }
    if (largeMissileSpawnTimer <= 0.0f && largeMissiles.size() < 4u)
    {
        spawn_large_missile(playerTransform);
        largeMissileSpawnTimer = std::max(1.0f, largeMissileSpawnInterval);
    }
    if (salvageSpawnTimer <= 0.0f && salvages.size() < 8u)
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
                enemy.hp -= missile.damage;
                if (enemy.hp <= 0)
                {
                    spawn_salvage_at(enemyTransform.position);
                    destroy_entity_safe(enemy.entity);
                    enemy.entity = CueEntityHandle{ k_cueInvalidHandleValue };
                    score += 100;
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
                    (void)consume_armor();
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
                    (void)consume_armor();
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

        transform.position.z -= 5.0f * a_deltaTime;
        transform.rotation.z += 1.1f * a_deltaTime;
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
    for (Salvage& salvage : salvages)
    {
        Marionette::Transform transform{};
        if (get_transform(salvage.entity, transform) != CueResult_Ok)
        {
            salvage.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            continue;
        }

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
    const float laneX =
        static_cast<float>(static_cast<int>(spawnIndex % 5u) - 2) * 2.2f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex / 2u) % 3u) - 1) *
        1.15f;
    ++spawnIndex;

    Marionette::Transform transform = make_transform(
        { laneX, laneY, a_playerZ + spawnLeadDistance },
        { 1.25f, 0.75f, 1.25f });

    CueEntityHandle entity{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc("Enemy", "Enemy", transform),
            entity) != CueResult_Ok)
    {
        return;
    }

    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindStaticMeshRenderer,
        make_renderer(1u, 1u));
    (void)add_or_set_component(
        entity,
        Marionette::ComponentKindCollider,
        make_box_trigger({ 0.7f, 0.7f, 0.7f }));
    enemies.push_back(Enemy{ entity, 1.05f, 1 });
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
