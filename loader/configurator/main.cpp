/*
 * configurator/main.cpp
 *
 * Configurator companion app, based on the one for GTA:SA Android loader.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <vitasdk.h>
#include <vitaGL.h>
#include <imgui_vita.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <sys/dirent.h>
#include <dirent.h>

#include "../utils/settings.h"

char *options_descs[] = {
        "Deadzone for the left analog stick. Increase if you have stick drift issues.\nThe default value is: 0.11.", // setting_leftStickDeadZone
        "Deadzone for the right analog stick. Increase if you have stick drift issues.\nThe default value is: 0.11.", // setting_rightStickDeadZone
        "If you want to reduce FPS fluctuations and instead stay on a lower but constant level, you may try this.\nThe default value is: None.", // setting_fpsLock
        "Choose the level of GFX detail you prefer. This setting mostly influences lighting.\nThe default value is: High.", // setting_gfxDetail
        "Choose the level of geometry detail you prefer. This setting mostly influences polygon count of models.\nThe default value is: Med.", // setting_geometryDetail
        "Choose whether to enable the MIP Maps in the game.\nThe default value is: Disable.", // setting_enableMipMaps
        "How far will the world be rendered before your eyes.\nThe default value is: 1.10.", // setting_viewDistance
};

enum {
    OPT_DEADZONE_L,
    OPT_DEADZONE_R,
    OPT_FPSLOCK,
    OPT_GFXDETAIL,
    OPT_GEOMETRYDETAIL,
    OPT_ENABLEMIPMAPS,
    OPT_VIEWDISTANCE,
};

char *desc = nullptr;

void SetDescription(int i) {
    if (ImGui::IsItemHovered())
        desc = options_descs[i];
}

ImTextureID loadTex(const char* fname) {
    FILE* f = fopen(fname, "r");
    if (!f) return NULL;
    int image_width, image_height, depth;
    unsigned char* data = stbi_load_from_file(f, &image_width, &image_height, &depth, 0);
    fclose(f);

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return reinterpret_cast<ImTextureID>(image_texture);
}

bool FancyButton(const char* label, const ImVec2& pos, const ImVec2& size, ImU32 color_1, ImU32 color_2) {
    bool ret = false;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_2);
    ImGui::PushStyleColor(ImGuiCol_Border, color_1);
    ImGui::PushStyleColor(ImGuiCol_Text, color_1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::SetCursorPos(pos);
    if (ImGui::Button(label, size))
        ret = true;
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    return ret;
}

#include "imgui_internal.h"

ImFont* FontSm;
ImFont* FontMd;
ImFont* FontLg;

namespace ImGui {
    bool SelectableCentered(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg);

    void StyledLabel(const char * label);

    void StyledSliderFloat(const char * l, float * val, float min, float max, ImU32 foreground, ImU32 background);
}

bool ImGui::SelectableCentered(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
        PopClipRect();

    ImGuiID id = window->GetID(label);
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrentLineTextBaseOffset;
    ImRect bb(pos, {pos.x + size.x, pos.y + size.y});
    ItemSize(bb);

    // Fill horizontal space.
    ImVec2 window_padding = window->WindowPadding;
    float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
    float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - window->DC.CursorPos.x);
    ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
    ImRect bb_with_spacing(pos, {pos.x + size_draw.x, pos.y + size_draw.y});
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
        bb_with_spacing.Max.x += window_padding.x;

    // Selectables are tightly packed together, we extend the box to cover spacing between selectable.
    float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
    float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
    float spacing_R = style.ItemSpacing.x - spacing_L;
    float spacing_D = style.ItemSpacing.y - spacing_U;
    bb_with_spacing.Min.x -= spacing_L;
    bb_with_spacing.Min.y -= spacing_U;
    bb_with_spacing.Max.x += spacing_R;
    bb_with_spacing.Max.y += spacing_D;
    if (!ItemAdd(bb_with_spacing, (flags & ImGuiSelectableFlags_Disabled) ? 0 : id))
    {
        if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
            PushColumnClipRect();
        return false;
    }

    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_Menu) button_flags |= ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveID;
    if (flags & ImGuiSelectableFlags_MenuItem) button_flags |= ImGuiButtonFlags_PressedOnRelease;
    if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
    if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb_with_spacing, id, &hovered, &held, button_flags);
    if (flags & ImGuiSelectableFlags_Disabled)
        selected = false;

    // Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
    if (pressed || hovered)// && (g.IO.MouseDelta.x != 0.0f || g.IO.MouseDelta.y != 0.0f))
        if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerActiveMask)
        {
            g.NavDisableHighlight = true;
            //SetNavID(id, window->DC.NavLayerCurrent);
        }

    // Render
    if (hovered || selected)
    {
        const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(bb_with_spacing.Min, bb_with_spacing.Max, col, false, 0.0f);
        RenderNavHighlight(bb_with_spacing, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
    } else {
        const ImU32 col = GetColorU32(ImGuiCol_TitleBgCollapsed);
        RenderFrame(bb_with_spacing.Min, bb_with_spacing.Max, col, false, 0.0f);
    }

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
    {
        PushColumnClipRect();
        bb_with_spacing.Max.x -= (GetContentRegionMax().x - max_x);
    }

    ImVec2 text_pos;
    text_pos.x = bb.Min.x + (((bb.Max.x - bb.Min.x) - (label_size.x)) / 2.f);
    text_pos.y = bb.Min.y + (((bb.Max.y - bb.Min.y) - (label_size.y)) / 2.f);

    if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    RenderText(text_pos, label);
    if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

    // Automatically close popups
    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
        CloseCurrentPopup();
    return pressed;
}

void ImGui::StyledLabel(const char * label) {
    ImGui::PushFont(FontLg);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(203, 149, 93, 255));
    ImGui::Text(label);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void ImGui::StyledSliderFloat(const char * l, float * val, float min, float max, ImU32 foreground, ImU32 background) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 3.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, background);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, background);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, background);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, foreground);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, foreground);
    ImGui::SliderFloat(l, val, min, max, "%.2f");
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar();
}

int main(int argc, char *argv[]) {
    settings_reset();
    settings_load();

    int exit_code = 0xDEAD;

    vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);

    ImTextureID bg = loadTex("app0:/configurator-bg.png");

    ImGui::CreateContext();
    ImGui_ImplVitaGL_Init();
    ImGui_ImplVitaGL_TouchUsage(false);
    ImGui_ImplVitaGL_GamepadUsage(true);
    ImGui::StyleColorsDark();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::GetIO().MouseDrawCursor = false;




    ImGuiIO& io = ImGui::GetIO();
    FontSm = ImGui::GetIO().Fonts->AddFontFromFileTTF("SourceSansPro-Regular.ttf", 14, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    FontLg = ImGui::GetIO().Fonts->AddFontFromFileTTF("SourceSansPro-Regular.ttf", 20, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    FontMd = ImGui::GetIO().Fonts->AddFontFromFileTTF("SourceSansPro-Regular.ttf", 16, nullptr, io.Fonts->GetGlyphRangesCyrillic());

    while (exit_code == 0xDEAD) {
        desc = nullptr;
        ImGui_ImplVitaGL_NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
        ImGui::SetNextWindowSize(ImVec2(960, 544), ImGuiSetCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});


        ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImDrawList* idl = ImGui::GetWindowDrawList();
        idl->AddImage(bg, {0,0}, {960,544});


        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        ImGui::BeginGroup();

        {
            ImGui::PushItemWidth(184);

            ImGui::SetCursorPos({73, 99});
            ImGui::StyledLabel("Left Stick Deadzone");
    
            ImGui::SetCursorPos({73, 123});
            ImGui::StyledSliderFloat("##leftStickDeadZone", &setting_leftStickDeadZone, 0.01f, 0.5f, IM_COL32(206, 140, 71, 255), IM_COL32(77, 70, 61, 255));
            SetDescription(OPT_DEADZONE_L);
    
            ImGui::SetCursorPos({274, 99});
            ImGui::StyledLabel("Right Stick Deadzone");

            ImGui::SetCursorPos({274, 123});
            ImGui::StyledSliderFloat("##rightStickDeadZone", &setting_rightStickDeadZone, 0.01f, 0.5f, IM_COL32(206, 140, 71, 255), IM_COL32(77, 70, 61, 255));
            SetDescription(OPT_DEADZONE_R);
    
            ImGui::PopItemWidth();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0,0});

        // base color
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, IM_COL32(77, 70, 61, 255));

        // elements color
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(206, 140, 71, 255));

        // hovered elements color
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(206, 140, 71, 255));

        {
            ImGui::SetCursorPos({73, 160});
            ImGui::StyledLabel("FPS Lock");

            ImGui::SetCursorPos({73, 182});
            if (ImGui::SelectableCentered("No Lock", setting_fpsLock == 0, 0, {120,20}))
                setting_fpsLock = 0;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({209, 182});
            if (ImGui::SelectableCentered("40 FPS", setting_fpsLock == 40, 0, {72,20}))
                setting_fpsLock = 40;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({298, 182});
            if (ImGui::SelectableCentered("30 FPS", setting_fpsLock == 30, 0, {72,20}))
                setting_fpsLock = 30;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({386, 182});
            if (ImGui::SelectableCentered("25 FPS", setting_fpsLock == 25, 0, {72,20}))
                setting_fpsLock = 25;
            SetDescription(OPT_FPSLOCK);
        }


        {
            ImGui::SetCursorPos({73, 220});
            ImGui::StyledLabel("GFX Detail");

            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {3,10});
            ImGui::SetCursorPos({73, 243});
            if (ImGui::SelectableCentered("Low##gfx", setting_gfxDetail == 0, 0, {60,20}))
                setting_gfxDetail = 0;
            SetDescription(OPT_GFXDETAIL);
            ImGui::PopStyleVar(1);

            ImGui::SetCursorPos({150, 243});
            if (ImGui::SelectableCentered("High##gfx", setting_gfxDetail == 1, 0, {60,20}))
                setting_gfxDetail = 1;
            SetDescription(OPT_GFXDETAIL);
        }

        {
            ImGui::SetCursorPos({245, 220});
            ImGui::StyledLabel("Geometry Detail");

            ImGui::SetCursorPos({246, 243});
            if (ImGui::SelectableCentered("Low##geom", setting_geometryDetail == 0, 0, {60,20}))
                setting_geometryDetail = 0;
            SetDescription(OPT_GEOMETRYDETAIL);

            ImGui::SetCursorPos({323, 243});
            if (ImGui::SelectableCentered("Med##geom", setting_geometryDetail == 1, 0, {60,20}))
                setting_geometryDetail = 1;
            SetDescription(OPT_GEOMETRYDETAIL);

            ImGui::SetCursorPos({400, 243});
            if (ImGui::SelectableCentered("High##geom", setting_geometryDetail == 2, 0, {58,20}))
                setting_geometryDetail = 2;
            SetDescription(OPT_GEOMETRYDETAIL);
        }

        {
            ImGui::SetCursorPos({73, 281});
            ImGui::StyledLabel("MIP Maps");

            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {3,10});
            ImGui::SetCursorPos({73, 305});
            if (ImGui::SelectableCentered("Disable##mip", setting_enableMipMaps == false, 0, {60,20}))
                setting_enableMipMaps = false;
            SetDescription(OPT_ENABLEMIPMAPS);
            ImGui::PopStyleVar(1);

            ImGui::SetCursorPos({150, 305});
            if (ImGui::SelectableCentered("Enable##mip", setting_enableMipMaps == true, 0, {60,20}))
                setting_enableMipMaps = true;
            SetDescription(OPT_ENABLEMIPMAPS);
        }

        {
            ImGui::PushItemWidth(212);

            ImGui::SetCursorPos({246, 281});
            ImGui::StyledLabel("View Distance");

            ImGui::SetCursorPos({246, 305});
            ImGui::StyledSliderFloat("##viewDistance", &setting_viewDistance, 0.4f, 3.0f, IM_COL32(206, 140, 71, 255), IM_COL32(77, 70, 61, 255));
            SetDescription(OPT_VIEWDISTANCE);

            ImGui::PopItemWidth();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (desc) {
            ImGui::SetCursorPos({72, 418});
            ImGui::PushTextWrapPos(464);
            ImGui::PushFont(FontMd);
            ImGui::Text(desc);
            ImGui::PopFont();
            ImGui::PopTextWrapPos();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::PushFont(FontLg);
        // first color is label and border, second is hover
        if (FancyButton("Save and Launch", {523, 315}, {187, 40}, IM_COL32(206,140,71,255), IM_COL32(206,140,71,20))) {
            settings_save();
            exit_code = 1;
        }

        if (FancyButton("Reset Settings", {724, 315}, {187, 40}, IM_COL32(135,107,65,255), IM_COL32(206,140,71,20))) {
            settings_reset();
        }
        ImGui::PopFont();

        ImGui::EndGroup();

        ImGui::PopStyleVar();
        ImGui::End();
        ImGui::PopStyleVar(2);

        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);
    }

    if (exit_code < 2) // Save
        settings_save();

    if (exit_code % 2 == 1) // Launch
        sceAppMgrLoadExec("app0:/eboot.bin", NULL, NULL);

    return 0;
}
