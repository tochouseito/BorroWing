#pragma once

#include <ScriptFramework/Marionette.h>

#include <cstdint>
#include <string_view>
#include <vector>

MARIONETTE_DECLARE_SCRIPT_TYPE(SceneMenu, "SceneMenu");

class SceneMenu final : public Marionette::Behaviour<SceneMenu>
{
public:
    using StateBlob = Marionette::StateBlob<SceneMenu>;
    using Marionette::Behaviour<SceneMenu>::update;
    MARIONETTE_FIELDS(
        CUE_FIELD_INT32_META(
            "Scene",
            "sceneKind",
            0,
            Marionette::EditAnywhere | Marionette::Serialize)
    );
    MARIONETTE_NO_FUNCTIONS();

    void bind_fields(const Marionette::ScriptFieldReader& a_reader);
    void start();
    void update();

private:
    enum class Kind : int32_t
    {
        Title = 0,
        GameOver = 1,
        Result = 2,
    };

    void ensure_ui();
    void refresh_ui();
    void transition_to(const char* a_sceneName);
    void cleanup_scene();
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
    void destroy_entity_safe(CueEntityHandle a_entity) const;

    Kind sceneKind = Kind::Title;
    CueEntityHandle uiCanvasEntity{ k_cueInvalidHandleValue };
    CueEntityHandle uiTitleEntity{ k_cueInvalidHandleValue };
    CueEntityHandle uiBodyEntity{ k_cueInvalidHandleValue };
    bool hasTransitioned = false;
};

[[nodiscard]] Cue::Core::Native::ScriptClassDefinition
make_scene_menu_script_definition() noexcept;
