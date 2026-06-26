#include "log.h"

#include "imgui/imgui.h"

#include <mutex>
#include <string>
#include <vector>

namespace kx::Log {
namespace {

std::vector<std::string> g_messages;
std::mutex g_mutex;
constexpr size_t kMaxMessages = 50;

ImVec4 colorFor(const std::string& msg) {
    if (msg.rfind("ERROR:", 0) == 0) return ImVec4(1.f, 0.3f, 0.3f, 1.f);
    if (msg.rfind("WARN:", 0) == 0)  return ImVec4(1.f, 1.f, 0.3f, 1.f);
    if (msg.rfind("INFO:", 0) == 0)  return ImVec4(0.5f, 1.f, 0.5f, 1.f);
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

}

void add(const std::string& message) {
    std::lock_guard lock(g_mutex);
    g_messages.push_back(message);
    if (g_messages.size() > kMaxMessages)
        g_messages.erase(g_messages.begin());
}

void clear() {
    std::lock_guard lock(g_mutex);
    g_messages.clear();
}

void copyToClipboard() {
    std::string text;
    {
        std::lock_guard lock(g_mutex);
        for (size_t i = 0; i < g_messages.size(); ++i) {
            if (i > 0)
                text += '\n';
            text += g_messages[i];
        }
    }
    if (!text.empty())
        ImGui::SetClipboardText(text.c_str());
}

void renderInline(float height) {
    const float h = height > 0.f ? height : ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("Log", ImVec2(0, h), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard lock(g_mutex);
        for (const auto& msg : g_messages)
            ImGui::TextColored(colorFor(msg), "%s", msg.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2)
        ImGui::SetScrollHereY(1.f);
    ImGui::EndChild();
}

}
