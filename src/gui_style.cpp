#define NOMINMAX

#include "gui_style.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <string>
#include <windows.h>
#include <ShlObj.h>

#pragma comment(lib, "Shell32.lib")

namespace kx::Gui {
namespace {

ImVec4 Rgb(int r, int g, int b) {
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, 1.f);
}

ImVec4 ScaleRgb(const ImVec4& c, float factor) {
    return ImVec4(std::clamp(c.x * factor, 0.f, 1.f),
                  std::clamp(c.y * factor, 0.f, 1.f),
                  std::clamp(c.z * factor, 0.f, 1.f),
                  1.f);
}

}

void loadFont(float fontSize) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    char fontsPath[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_FONTS, nullptr, 0, fontsPath))) {
        const std::string fontPath = std::string(fontsPath) + "\\bahnschrift.ttf";
        if (ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize))
            io.FontDefault = font;
    }
}

void applyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    const ImVec4 richBlack = Rgb(17, 19, 37);
    const ImVec4 oxfordBlue = Rgb(26, 31, 52);
    const ImVec4 spaceCadet = Rgb(37, 43, 69);
    const ImVec4 coolGray = Rgb(128, 138, 184);
    const ImVec4 neonBlue = Rgb(0, 98, 255);
    const ImVec4 azure = Rgb(51, 129, 255);
    const ImVec4 aliceBlue = Rgb(229, 236, 244);

    const ImVec4 spaceCadetHover = ScaleRgb(spaceCadet, 1.3f);
    const ImVec4 spaceCadetActive = ScaleRgb(spaceCadet, 0.9f);
    const ImVec4 neonBlueActive = ScaleRgb(neonBlue, 0.9f);

    style.WindowPadding = ImVec2(8.f, 8.f);
    style.FramePadding = ImVec2(5.f, 4.f);
    style.ItemSpacing = ImVec2(6.f, 4.f);
    style.ItemInnerSpacing = ImVec2(4.f, 4.f);
    style.ScrollbarSize = 14.f;
    style.GrabMinSize = 12.f;
    style.WindowRounding = 4.f;
    style.ChildRounding = 2.f;
    style.FrameRounding = 3.f;
    style.ScrollbarRounding = 9.f;
    style.GrabRounding = 3.f;
    style.TabRounding = 4.f;

    colors[ImGuiCol_Text] = aliceBlue;
    colors[ImGuiCol_TextDisabled] = coolGray;
    colors[ImGuiCol_WindowBg] = richBlack;
    colors[ImGuiCol_ChildBg] = oxfordBlue;
    colors[ImGuiCol_PopupBg] = richBlack;
    colors[ImGuiCol_Border] = spaceCadet;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
    colors[ImGuiCol_FrameBg] = spaceCadet;
    colors[ImGuiCol_FrameBgHovered] = spaceCadetHover;
    colors[ImGuiCol_FrameBgActive] = spaceCadetActive;
    colors[ImGuiCol_TitleBg] = richBlack;
    colors[ImGuiCol_TitleBgActive] = oxfordBlue;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(richBlack.x, richBlack.y, richBlack.z, 0.85f);
    colors[ImGuiCol_MenuBarBg] = oxfordBlue;
    colors[ImGuiCol_ScrollbarBg] = richBlack;
    colors[ImGuiCol_ScrollbarGrab] = coolGray;
    colors[ImGuiCol_ScrollbarGrabHovered] = aliceBlue;
    colors[ImGuiCol_ScrollbarGrabActive] = azure;
    colors[ImGuiCol_CheckMark] = neonBlue;
    colors[ImGuiCol_SliderGrab] = neonBlue;
    colors[ImGuiCol_SliderGrabActive] = azure;
    colors[ImGuiCol_Button] = neonBlue;
    colors[ImGuiCol_ButtonHovered] = azure;
    colors[ImGuiCol_ButtonActive] = neonBlueActive;
    colors[ImGuiCol_Header] = spaceCadet;
    colors[ImGuiCol_HeaderHovered] = spaceCadetHover;
    colors[ImGuiCol_HeaderActive] = spaceCadetHover;
    colors[ImGuiCol_Separator] = spaceCadet;
    colors[ImGuiCol_SeparatorHovered] = azure;
    colors[ImGuiCol_SeparatorActive] = neonBlue;
    colors[ImGuiCol_ResizeGrip] = ImVec4(coolGray.x, coolGray.y, coolGray.z, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = coolGray;
    colors[ImGuiCol_ResizeGripActive] = neonBlue;
    colors[ImGuiCol_Tab] = oxfordBlue;
    colors[ImGuiCol_TabHovered] = azure;
    colors[ImGuiCol_TabActive] = neonBlue;
    colors[ImGuiCol_TabUnfocused] = ImVec4(oxfordBlue.x, oxfordBlue.y, oxfordBlue.z, 0.8f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(neonBlue.x, neonBlue.y, neonBlue.z, 0.6f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(azure.x, azure.y, azure.z, 0.4f);
    colors[ImGuiCol_NavHighlight] = azure;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(richBlack.x, richBlack.y, richBlack.z, 0.75f);
}

} // namespace kx::Gui
