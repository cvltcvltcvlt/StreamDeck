#include "style.hpp"

void ButtonStyle::SetStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.204f, 0.247f, 0.82f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.102f, 0.120f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.104f, 0.147f, 0.72f, 1.0f));
}

void ButtonStyle::ResetStyle()
{
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}
