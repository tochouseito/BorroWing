#include "GameManagerScript.h"

// === C++ includes ===
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace
{
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
    (void)read_float(a_reader, "missileSpeed", missileSpeed);
    (void)read_float(a_reader, "missileLifeTime", missileLifeTime);
    (void)read_float(a_reader, "enemySpawnInterval", enemySpawnInterval);
    (void)read_float(a_reader, "salvageSpawnInterval", salvageSpawnInterval);
    (void)read_float(a_reader, "spawnLeadDistance", spawnLeadDistance);
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
    fireTimer = std::max(0.0f, fireTimer - dt);
    progressLogTimer = std::max(0.0f, progressLogTimer - dt);

    resolve_player();
    update_combat(dt);
    update_spawning(dt);
    update_missiles(dt);
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

void GameManager::update_combat(float)
{
    if (playerEntity.value == k_cueInvalidHandleValue || fireTimer > 0.0f)
    {
        return;
    }

    const bool wantsFire =
        push_key(Marionette::Key::Space) ||
        push_mouse_button(Marionette::MouseButton::Left);
    if (!wantsFire)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

    spawn_missile(playerTransform);
    fireTimer = std::max(0.03f, fireInterval);
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
    salvageSpawnTimer -= a_deltaTime;
    if (enemySpawnTimer <= 0.0f && enemies.size() < 12u)
    {
        spawn_enemy(playerTransform.position.z);
        enemySpawnTimer = std::max(0.2f, enemySpawnInterval);
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
            missile.age = missileLifeTime + 1.0f;
            continue;
        }

        transform.position.z += missileSpeed * a_deltaTime;
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

            constexpr float k_missileRadius = 0.35f;
            const float hitRadius = enemy.radius + k_missileRadius;
            if (distance_sq(transform.position, enemyTransform.position) <=
                hitRadius * hitRadius)
            {
                destroy_entity_safe(missile.entity);
                missile.entity = CueEntityHandle{ k_cueInvalidHandleValue };
                --enemy.hp;
                if (enemy.hp <= 0)
                {
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
                if (a_missile.age > missileLifeTime)
                {
                    destroy_entity_safe(a_missile.entity);
                    return true;
                }
                return false;
            }),
        missiles.end());
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
    if (playerEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    Marionette::Transform playerTransform{};
    if (get_transform(playerEntity, playerTransform) != CueResult_Ok)
    {
        return;
    }

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

        constexpr float k_playerRadius = 1.0f;
        const float collectRadius = salvage.radius + k_playerRadius;
        if (distance_sq(playerTransform.position, transform.position) <=
            collectRadius * collectRadius)
        {
            destroy_entity_safe(salvage.entity);
            salvage.entity = CueEntityHandle{ k_cueInvalidHandleValue };
            ++salvageCount;
            score += 25;
        }
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
}

void GameManager::spawn_missile(
    const Marionette::Transform& a_playerTransform)
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
    missiles.push_back(Missile{ entity, 0.0f });
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

void GameManager::spawn_salvage(float a_playerZ)
{
    const float laneX =
        static_cast<float>(static_cast<int>((spawnIndex + 1u) % 5u) - 2) *
        1.8f;
    const float laneY =
        static_cast<float>(static_cast<int>((spawnIndex + 2u) % 3u) - 1) *
        1.0f;

    Marionette::Transform transform = make_transform(
        { laneX, laneY, a_playerZ + spawnLeadDistance * 0.72f },
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

    char message[160]{};
    (void)std::snprintf(
        message,
        sizeof(message),
        "Score %d / Salvage %d / Time %.1f",
        score,
        salvageCount,
        elapsedTime);
    log_info(message);
    progressLogTimer = 5.0f;
}

MARIONETTE_DEFINE_SCRIPT(game_manager, GameManager);
