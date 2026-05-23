#pragma once

#include <ScriptFramework/Marionette.h>

MARIONETTE_DECLARE_SCRIPT_TYPE(PlayerFlight, "PlayerFlight");

class PlayerFlight final : public Marionette::Behaviour<PlayerFlight>
{
public:
    using StateBlob = Marionette::StateBlob<PlayerFlight>;
    using Marionette::Behaviour<PlayerFlight>::update;
    MARIONETTE_FIELDS(
        CUE_FIELD_FLOAT_META(
            "Flight",
            "railSpeed",
            18.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Flight",
            "moveSpeed",
            8.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Flight",
            "maxOffsetX",
            5.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Flight",
            "maxOffsetY",
            3.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_ENTITY_META(
            "Camera",
            "cameraEntity",
            CueEntityHandle{ k_cueInvalidHandleValue },
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Camera",
            "cameraDistance",
            10.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Camera",
            "cameraHeight",
            2.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Camera",
            "cameraFovY",
            64.0f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Camera",
            "cameraTiltAngle",
            0.25f,
            Marionette::EditAnywhere | Marionette::Serialize),
        CUE_FIELD_FLOAT_META(
            "Camera",
            "cameraTiltSmoothing",
            8.0f,
            Marionette::EditAnywhere | Marionette::Serialize)
    );
    MARIONETTE_NO_FUNCTIONS();

    void bind_fields(const Marionette::ScriptFieldReader& a_reader);
    void start();
    void update();

private:
    void update_player(float a_deltaTime);
    void resolve_camera();
    void attach_camera();
    void update_camera() const;

    CueEntityHandle cameraEntity{ k_cueInvalidHandleValue };
    float railDistance = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float railSpeed = 18.0f;
    float moveSpeed = 8.0f;
    float maxOffsetX = 5.0f;
    float maxOffsetY = 3.0f;
    float cameraDistance = 10.0f;
    float cameraHeight = 2.0f;
    float cameraFovY = 64.0f;
    float cameraTiltAngle = 0.25f;
    float cameraTiltSmoothing = 8.0f;
    float currentTilt = 0.0f;
    uint32_t observedResetSerial = 0u;
};

[[nodiscard]] Cue::Core::Native::ScriptClassDefinition
make_player_flight_script_definition() noexcept;
