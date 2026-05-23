#include "SceneMenuScript.h"

#include "BorroWingGameState.h"

#include <algorithm>
#include <cstdio>

namespace
{
    constexpr Marionette::Color k_titleColor{ 0.82f, 0.95f, 1.0f, 1.0f };
    constexpr Marionette::Color k_bodyColor{ 0.94f, 0.94f, 0.9f, 1.0f };
    constexpr Marionette::Color k_warningColor{ 1.0f, 0.32f, 0.22f, 1.0f };
    constexpr Marionette::Color k_resultColor{ 0.35f, 1.0f, 0.68f, 1.0f };

    [[nodiscard]] CueStringView make_view(std::string_view a_value) noexcept
    {
        return CueStringView{
            a_value.data(),
            static_cast<uint32_t>(a_value.size())
        };
    }

    [[nodiscard]] Marionette::Transform make_transform(
        const CueFloat3& a_position) noexcept
    {
        Marionette::Transform transform{};
        transform.position = a_position;
        transform.rotation = { 0.0f, 0.0f, 0.0f };
        transform.scale = { 1.0f, 1.0f, 1.0f };
        return transform;
    }

    [[nodiscard]] Marionette::SpawnObjectDesc make_spawn_desc(
        Marionette::SpawnObjectKind a_kind,
        std::string_view a_name,
        std::string_view a_tag) noexcept
    {
        Marionette::SpawnObjectDesc desc{};
        desc.kind = a_kind;
        desc.sceneId = Marionette::k_invalidSceneId;
        desc.name = make_view(a_name);
        desc.tag = make_view(a_tag);
        desc.transform = make_transform({ 0.0f, 0.0f, 0.0f });
        desc.isActive = 1u;
        desc.isPersistent = 0u;
        return desc;
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
}

void SceneMenu::bind_fields(const Marionette::ScriptFieldReader& a_reader)
{
    int32_t kind = static_cast<int32_t>(sceneKind);
    if (read_int32(a_reader, "sceneKind", kind))
    {
        kind = std::clamp(kind, 0, 2);
        sceneKind = static_cast<Kind>(kind);
    }
}

void SceneMenu::start()
{
    BorroWing::set_gameplay_active(false);
    ensure_ui();
    refresh_ui();
}

void SceneMenu::update()
{
    if (hasTransitioned)
    {
        return;
    }

    ensure_ui();
    refresh_ui();

    if (push_key(Marionette::Key::Enter) ||
        push_key(Marionette::Key::Space))
    {
        transition_to("Play");
        return;
    }

    if (sceneKind != Kind::Title && push_key(Marionette::Key::Escape))
    {
        transition_to("Title");
    }
}

void SceneMenu::ensure_ui()
{
    if (uiCanvasEntity.value != k_cueInvalidHandleValue)
    {
        Marionette::Transform transform{};
        if (get_transform(uiCanvasEntity, transform) == CueResult_Ok)
        {
            return;
        }
    }

    uiCanvasEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiTitleEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiBodyEntity = CueEntityHandle{ k_cueInvalidHandleValue };

    CueEntityHandle canvas{ k_cueInvalidHandleValue };
    if (spawn_object(
            make_spawn_desc(
                Marionette::SpawnObjectKindEmpty,
                "SceneMenuCanvas",
                "SceneMenuUi"),
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

    uiTitleEntity =
        spawn_ui_text("SceneMenuTitle", -190.0f, 150.0f, 82u, 20u, k_titleColor);
    uiBodyEntity =
        spawn_ui_text("SceneMenuBody", 10.0f, 330.0f, 34u, 21u, k_bodyColor);
}

void SceneMenu::refresh_ui()
{
    char body[512]{};
    const BorroWing::RunSummary summary = BorroWing::last_run_summary();

    switch (sceneKind)
    {
    case Kind::Title:
        set_ui_text(uiTitleEntity, "BorroWing", 82u, 20u, k_titleColor);
        set_ui_text(
            uiBodyEntity,
            "Press ENTER or SPACE to start\nWASD / Arrow: Move  Space: Gun  Right Mouse: Missile\nE: Convert Field  R: Infinite Missile",
            30u,
            21u,
            k_bodyColor);
        return;
    case Kind::GameOver:
        (void)std::snprintf(
            body,
            sizeof(body),
            "Score %d\nSurvival Time %.1f sec\nPress ENTER or SPACE to retry\nPress ESC for title",
            summary.score,
            summary.elapsedTime);
        set_ui_text(uiTitleEntity, "GAME OVER", 76u, 20u, k_warningColor);
        set_ui_text(uiBodyEntity, body, 34u, 21u, k_bodyColor);
        return;
    case Kind::Result:
        (void)std::snprintf(
            body,
            sizeof(body),
            "Score %d\nSalvage %d   Armor %d\nClear Time %.1f sec\nPress ENTER or SPACE to play again\nPress ESC for title",
            summary.score,
            summary.salvageCount,
            summary.armorCount,
            summary.elapsedTime);
        set_ui_text(uiTitleEntity, "RESULT", 76u, 20u, k_resultColor);
        set_ui_text(uiBodyEntity, body, 34u, 21u, k_bodyColor);
        return;
    }
}

void SceneMenu::transition_to(const char* a_sceneName)
{
    const Marionette::SceneId loadedScene =
        Marionette::SceneManager.load_scene(a_sceneName);
    if (loadedScene == Marionette::k_invalidSceneId)
    {
        log_warning("Scene load request failed.");
        return;
    }

    hasTransitioned = true;
    cleanup_scene();
}

void SceneMenu::cleanup_scene()
{
    destroy_entity_safe(uiTitleEntity);
    destroy_entity_safe(uiBodyEntity);
    destroy_entity_safe(uiCanvasEntity);
    uiTitleEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiBodyEntity = CueEntityHandle{ k_cueInvalidHandleValue };
    uiCanvasEntity = CueEntityHandle{ k_cueInvalidHandleValue };

    const std::vector<CueEntityHandle> cameras =
        find_entities_by_tag("MenuCamera");
    for (CueEntityHandle camera : cameras)
    {
        destroy_entity_safe(camera);
    }

    destroy_entity_safe(self());
}

CueEntityHandle SceneMenu::spawn_ui_text(
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
                "SceneMenuUi"),
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

void SceneMenu::set_ui_text(
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

void SceneMenu::destroy_entity_safe(CueEntityHandle a_entity) const
{
    if (a_entity.value != k_cueInvalidHandleValue)
    {
        (void)destroy_entity(a_entity);
    }
}

MARIONETTE_DEFINE_SCRIPT(scene_menu, SceneMenu);
