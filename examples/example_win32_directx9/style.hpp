#ifndef STYLE_HPP
#define STYLE_HPP

#include "imgui.h"

class ButtonStyle
{
public:
    static void SetStyle();
    static void ResetStyle();
    

private:
    static ImVec4 normalColor;
    static ImVec4 hoverColor;
    static ImVec4 activeColor;
};

#endif // STYLE_HPP
