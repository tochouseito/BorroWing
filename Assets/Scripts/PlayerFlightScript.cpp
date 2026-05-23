#include "PlayerFlightScript.h"

// === C++ includes ===
#include <algorithm>
#include <vector>

#include "BorroWingGameState.h"

void PlayerFlight::bind_fields(const Marionette::ScriptFieldReader& a_reader)
{
    (void)read_float(a_reader, "railSpeed", railSpeed);
    (void)read_float(a_reader, "moveSpeed", moveSpeed);
    (void)read_float(a_reader, "maxOffsetX", maxOffsetX);
    (void)read_float(a_reader, "maxOffsetY", maxOffsetY);
    (void)read_entity_handle(a_reader, "cameraEntity", cameraEntity);
    (void)read_float(a_reader, "cameraDistance", cameraDistance);
    (void)read_float(a_reader, "cameraHeight", cameraHeight);
    (void)read_float(a_reader, "cameraFovY", cameraFovY);
    (void)read_float(a_reader, "cameraTiltAngle", cameraTiltAngle);
    (void)read_float(a_reader, "cameraTiltSmoothing", cameraTiltSmoothing);
}

void PlayerFlight::start()
{
    if (!is_entity_valid() || !has_transform())
    {
        log_warning("PlayerFlight owner is not a valid transform entity.");
        return;
    }

    Transform transform{};
    if (get_transform(transform) == CueResult_Ok)
    {
        railDistance = 0.0f;
        offsetX = transform.position.x;
        offsetY = transform.position.y;
    }

    resolve_camera();
    attach_camera();
    update_camera();
}

void PlayerFlight::update()
{
    if (!is_entity_valid() || !has_transform())
    {
        return;
    }

    if (observedResetSerial != BorroWing::player_reset_serial())
    {
        observedResetSerial = BorroWing::player_reset_serial();
        Transform transform{};
        if (get_transform(transform) == CueResult_Ok)
        {
            railDistance = 0.0f;
            offsetX = transform.position.x;
            offsetY = transform.position.y;
            currentTilt = 0.0f;
        }
    }

    if (!BorroWing::is_gameplay_active())
    {
        update_camera();
        return;
    }

    const float dt = delta_time();
    update_player(dt);
    update_camera();
}

void PlayerFlight::update_player(float a_deltaTime)
{
    float inputX = 0.0f;
    float inputY = 0.0f;

    if (push_key(Marionette::Key::A) || push_key(Marionette::Key::Left))
    {
        inputX -= 1.0f;
    }
    if (push_key(Marionette::Key::D) || push_key(Marionette::Key::Right))
    {
        inputX += 1.0f;
    }
    if (push_key(Marionette::Key::W) || push_key(Marionette::Key::Up))
    {
        inputY += 1.0f;
    }
    if (push_key(Marionette::Key::S) || push_key(Marionette::Key::Down))
    {
        inputY -= 1.0f;
    }

    railDistance = 0.0f;
    offsetX = std::clamp(
        offsetX + inputX * moveSpeed * a_deltaTime,
        -maxOffsetX,
        maxOffsetX);
    offsetY = std::clamp(
        offsetY + inputY * moveSpeed * a_deltaTime,
        -maxOffsetY,
        maxOffsetY);

    const float targetTilt = -inputX * cameraTiltAngle;
    const float tiltBlend = std::clamp(cameraTiltSmoothing * a_deltaTime, 0.0f, 1.0f);
    currentTilt += (targetTilt - currentTilt) * tiltBlend;

    Transform transform{};
    if (get_transform(transform) != CueResult_Ok)
    {
        return;
    }

    transform.position = { offsetX, offsetY, 0.0f };
    transform.rotation = { 0.0f, 0.0f, currentTilt };
    (void)set_transform(transform);
}

void PlayerFlight::resolve_camera()
{
    if (cameraEntity.value == k_cueInvalidHandleValue)
    {
        const std::vector<CueEntityHandle> cameras =
            find_entities_by_tag("MainCamera");
        if (!cameras.empty())
        {
            cameraEntity = cameras.front();
        }
    }
}

void PlayerFlight::attach_camera()
{
    if (cameraEntity.value == k_cueInvalidHandleValue)
    {
        log_warning("PlayerFlight cameraEntity is not assigned.");
        return;
    }

    Transform cameraTransform{};
    if (get_transform(cameraEntity, cameraTransform) != CueResult_Ok)
    {
        return;
    }

    cameraTransform.position = { 0.0f, cameraHeight, -cameraDistance };
    cameraTransform.rotation = { 0.0f, 0.0f, 0.0f };
    cameraTransform.scale = { 1.0f, 1.0f, 1.0f };
    (void)set_transform_degrees(cameraEntity, cameraTransform);
    (void)set_parent(cameraEntity, self(), false);
}

void PlayerFlight::update_camera() const
{
    if (cameraEntity.value == k_cueInvalidHandleValue)
    {
        return;
    }

    (void)set_camera_fov_y(cameraEntity, cameraFovY);
}

MARIONETTE_DEFINE_SCRIPT(player_flight, PlayerFlight);
