#pragma once
#include "IW3D.hpp"

typedef void Font;
typedef Font*(__cdecl * CL_RegisterFont_t)(const char* fontName, int imageTrack);
extern CL_RegisterFont_t CL_RegisterFont;

typedef Material*(__cdecl * Material_RegisterHandle_t)(const char* name, int imageTrack);
extern Material_RegisterHandle_t Material_RegisterHandle;

extern uint32_t CG_Draw2D_Address;

extern void UI_DrawString(const char* text, float x, float y, float width, float height, Font* font, vec4_t color);

extern void UI_DrawRectangle(float x, float y, float width, float height, Material* material, vec4_t color);

struct ButtonColorInfo {
	vec4_t pause { 1, 1, 1, 1 };
	vec4_t slower { 1, 1, 1, 1 };
	vec4_t faster { 1, 1, 1, 1 };
	vec4_t back { 1, 1, 1, 1 };
	vec4_t forward { 1, 1, 1, 1 };
	vec4_t dollyadd { 1, 1, 1, 1 };
	vec4_t dollyplay { 1, 1, 1, 1 };
	vec4_t dollydel { 1, 1, 1, 1 };
	vec4_t greenscreen { 1, 1, 1, 1 };
	vec4_t depth { 1, 1, 1, 1 };
};

extern ButtonColorInfo buttonColorInfo;

enum class MenuMode {
	Primary, Secondary
};

extern MenuMode currentMenu;

extern bool demoDrawMenu;

extern uint64_t frameCount;

extern void Patch_Drawing();