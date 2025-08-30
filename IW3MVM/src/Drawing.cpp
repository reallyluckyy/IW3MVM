#include "stdafx.hpp"
#include "Math.hpp"
#include "Drawing.hpp"

#include "Console.hpp"
#include "Game.hpp"
#include "Utility.hpp"
#include "Freecam.hpp"
#include "Dollycam.hpp"
#include "Demo.hpp"
#include "IW3D.hpp"
#include "Streams.hpp"

CL_RegisterFont_t CL_RegisterFont;
Material_RegisterHandle_t Material_RegisterHandle;

uint32_t UI_TextWidth_Internal_Address;
uint32_t R_AddCmdDrawStretchPic_Address;
uint32_t R_AddCmdDrawText_Address;

uint32_t CG_Draw2D_Address;

ButtonColorInfo buttonColorInfo;
MenuMode currentMenu;
bool demoDrawMenu = true;

uint64_t frameCount = 0;

/* GAME-ENGINE DRAWING WRAPPER FUNCTIONS */

void UI_DrawString(const char* text, float x, float y, float width, float height, Font* font, vec4_t color)
{
	if (x > refdef->windowSize[0] || x < 0)
		return;

	if (y > refdef->windowSize[1] || y < 0)
		return;

	width /= 1280.0f;
	width *= refdef->windowSize[0];
	height /= 720.0f;
	height *= refdef->windowSize[1];

	__asm
	{
		push 0.0f
		push 0.0f
		push height
		push width
		push y
		push x
		push font
		push 0x7FFFFFFF
		push text
		mov	ecx, color
		call [R_AddCmdDrawText_Address]
		add esp, 0x24
	}
}

void UI_DrawRectangle(float x, float y, float width, float height, Material* material, vec4_t color)
{
	__asm
	{
		push color
		push 1.0f
		push 1.0f
		push 0.0f
		push 0.0f
		push height
		push width
		push y
		push x
		mov	eax, material
		call [R_AddCmdDrawStretchPic_Address]
		add	esp, 0x24
	}
}

int UI_TextWidth_Internal(const char* text, Font* font, float scale)
{
	__asm
	{
		push scale
		push 0
		push text
		mov eax, font
		mov ecx, 0
		call[UI_TextWidth_Internal_Address]
		add esp, 0xC
	}
}

int __declspec(noinline) UI_TextWidth(const char* text, Font* font, float scale)
{
	// No idea why we have to scale things by this odd factor, but this works
	scale *= (((float)refdef->windowSize[0]) / 640.0f) * 0.365;

	// Remove ^3 etc from string since those dont contribute to the length
	std::string sanitizedText = text;
	for (int i = 0; i < 10; i++)
	{
		auto pos = sanitizedText.find(va("^%d", i));
		while(pos != std::string::npos)
		{
			sanitizedText.erase(pos, 2);
			pos = sanitizedText.find(va("^%d", i));
		}
	}
	
	return UI_TextWidth_Internal(sanitizedText.c_str(), font, scale);

}


// called once per game-frame
void CG_Draw2D()
{

	Font* objectiveFont = CL_RegisterFont("fonts/objectiveFont", 1);
	Material* white = Material_RegisterHandle("white", 7);

	if (Demo_IsPaused()
		&& currentDollycamState == DollycamState::Playing
		&& !mvm_dolly_frozen->current.enabled
		&& recordingStatus != RecordingStatus::Running)
	{
#ifndef IW3D_EXPORT
		if (!(is_recording_streams || is_recording_avidemo))
		{
			std::string	cine_message;

			if (mvm_streams_fps->current.integer || mvm_avidemo_fps->current.integer)
				cine_message = "PRESS ^3R^7 TO PLAY AND RECORD THE CINEMATIC";
			else
				cine_message = "PRESS ^3SPACE^7 TO PLAY THE CINEMATIC";

			auto width = UI_TextWidth(cine_message.c_str(), objectiveFont, 0.8f);
			auto paddingX = 14;
			auto paddingY = 8;
			UI_DrawRectangle((refdef->windowSize[0] / 2.0f) - width * 0.5f - paddingX, refdef->windowSize[1] * 0.5f - (paddingY + 28), width + paddingX * 2, 28 + paddingY * 2, white, vec4_t{ 0.15f, 0.15f, 0.15f, 0.75f });
			UI_DrawString(cine_message.c_str(), (refdef->windowSize[0] / 2.0f) - width * 0.5f, refdef->windowSize[1] * 0.5f, 0.8f, 0.8f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });
		}
#else
		UI_DrawString("PRESS ^3SPACE^7 TO PLAY THE CINEMATIC", (refdef->windowSize[0] / 2.0f) * 0.50F, refdef->windowSize[1] * 0.50f, 1.0f, 1.0f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });
#endif
	}

#ifdef IW3D_EXPORT

	if (iw3d_fps->current.integer)
	{
		DrawIW3DUI();
	}

#endif

	uint32_t requiredNodeCount = mvm_dolly_linear->current.enabled ? 2 : 4;

	if (demoDrawMenu)
	{
		if (!currentView.isPOV)
		{
			for (std::pair<double, COSValue> node : nodes)
			{
				vec2_t screenPosition{0, 0};

				vec3_t nodepos{ (float)node.second.T[0], (float)node.second.T[1], (float)node.second.T[2] };
				vec3_t campos{ refdef->position[0], refdef->position[1], refdef->position[2] };

				if (WorldToScreen(vec3_t{ (float)node.second.T[0], (float)node.second.T[1], (float)node.second.T[2] }, screenPosition))
				{
					float dist = VectorDistance(nodepos, campos);

					float size = 10.0f / dist * (1000.0f);

					if (mvm_dolly_frozen->current.enabled)
						UI_DrawRectangle(screenPosition[0] - size * 0.25f, screenPosition[1] - size * 0.25f, size, size, white, vec4_t{ 0.5f, 0.7f, 1.0f, 1.0f });
					else
						UI_DrawRectangle(screenPosition[0] - size * 0.25f, screenPosition[1] - size * 0.25f, size, size, white, vec4_t{ 1.0f, 0.75f, 0.0f, 1.0f });
				}
			}

			if (nodes.size() >= requiredNodeCount)
			{
				for (double i = (nodes.begin()->first); i < (nodes.rbegin()->first); i += (nodes.rbegin()->first - nodes.begin()->first) / (nodes.size() * 10))
				{
					Node node = GetNode(i);

					vec2_t screenPosition{ 0, 0 };

					vec3_t nodepos{ (float)node.x, (float)node.y, (float)node.z };
					vec3_t campos{ refdef->position[0], refdef->position[1], refdef->position[2] };

					if (WorldToScreen(vec3_t{ (float)node.x, (float)node.y, (float)node.z }, screenPosition))
					{
						float dist = VectorDistance(nodepos, campos);

						float size = 5.0f / dist * (1000.0f);

						if (mvm_dolly_frozen->current.enabled)
							UI_DrawRectangle(screenPosition[0], screenPosition[1], size, size, white, vec4_t{ 0.5f, 0.725f, 1.0f, 1.0f });
						else
							UI_DrawRectangle(screenPosition[0], screenPosition[1], size, size, white, vec4_t{ 1.0f, 0.15f, 0.0f, 1.0f });
					}
				}
			}
		}

		uint32_t requiredNodeCount = mvm_dolly_linear->current.enabled ? 2 : 4;
		bool canPlayFrozenCinematic = currentDollycamState == DollycamState::Disabled && mvm_dolly_frozen->current.enabled && nodes.size() >= requiredNodeCount;

		if ((mvm_avidemo_fps->current.integer || mvm_streams_fps->current.integer) 
			&& (!is_recording_avidemo && !is_recording_streams) 
			&& (currentView.isPOV || canPlayFrozenCinematic))
		{
			std::string displayText;
			if (currentView.isPOV)
			{
				displayText = "PRESS ^3R^7 TO START/STOP RECORDING";
			}
			else if (canPlayFrozenCinematic)
			{
				displayText = "PRESS ^3R^7 TO RECORD YOUR FROZEN CINEMATIC";
			}

			auto textScale = 0.5f;
			auto width = UI_TextWidth(displayText.c_str(), objectiveFont, textScale);
			auto paddingX = 14;
			auto paddingY = 8;
			UI_DrawRectangle(10, 10, width + paddingX * 2, 20 + paddingY * 2, white, vec4_t{ 0.15f, 0.15f, 0.15f, 0.75f });
			UI_DrawString(displayText.c_str(), 10 + paddingX, 10 + 20 + paddingY, textScale, textScale, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		/* BASE FRAME */
		float baseFrameWidth = refdef->windowSize[0] / 2.0f; // 640
		float baseFrameHeight = refdef->windowSize[1] / 4.5f; // 160
		float baseFramePositionX = (refdef->windowSize[0] / 2.0f) - baseFrameWidth * 0.5f;
		float baseFramePositionY = refdef->windowSize[1] * 0.75f;
		UI_DrawRectangle(baseFramePositionX, baseFramePositionY, baseFrameWidth, baseFrameHeight, white, vec4_t{ 0.15f, 0.15f, 0.15f, 0.55f });

		UI_DrawString("^3F1^7 Toggle Menu", baseFramePositionX, baseFramePositionY - refdef->windowSize[1] * 0.01f, 0.45f, 0.50f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });
		UI_DrawString("^3F2^7 Toggle View", baseFramePositionX + refdef->windowSize[0] / 2.45f, baseFramePositionY - refdef->windowSize[1] * 0.01f, 0.45f, 0.50f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });


		if (currentMenu == MenuMode::Primary)
		{
			std::string demoName = Demo_GetName();
			if (demoName.size() > 34)
			{
				demoName = demoName.substr(0, 34);
				demoName += "...";
			}

			UI_DrawString(demoName.c_str(), baseFramePositionX + baseFrameWidth * 0.5f - UI_TextWidth(demoName.c_str(), objectiveFont, 0.65f) * 0.5f, baseFramePositionY + baseFrameHeight * 0.225f, 0.65f, 0.65f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });

			/* DEMO PROGRESS BAR */

			float percentage = (float)((float)(Demo_GetCurrentTick() - Demo_GetStartTick()) / (float)(Demo_GetEndTick() - Demo_GetStartTick()));
			if (percentage < 0)
				percentage = 0;
			else if (percentage > 1)
				percentage = 1;

			float baseBarPositionX = baseFramePositionX + (refdef->windowSize[0] / 36.5714285714f); // 35.0f
			float baseBarPositionY = baseFramePositionY + (refdef->windowSize[1] / 16.0f); // 35.0f
			float baseBarWidth = refdef->windowSize[0] / 2.25352112676f; // 568.0f
			float baseBarHeight = refdef->windowSize[1] / 20.0f; // 36.0f
			float baseInnerBarOffset = refdef->windowSize[0] / 256.0f;

			// background
			UI_DrawRectangle(baseBarPositionX, baseBarPositionY, baseBarWidth, baseBarHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.4f });

			UI_DrawString(va("%i", Demo_GetEndTick() - Demo_GetStartTick()), baseBarPositionX + baseBarWidth - UI_TextWidth(va("%i", Demo_GetEndTick() - Demo_GetStartTick()), objectiveFont, 0.5f), baseBarPositionY * 0.995f, 0.5f, 0.5f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });

			// bar itself
			UI_DrawRectangle(baseBarPositionX + baseInnerBarOffset, baseBarPositionY + baseInnerBarOffset, (baseBarWidth * percentage) - baseInnerBarOffset * 2, baseBarHeight - baseInnerBarOffset * 2, white, vec4_t{ 0.25f, 0.25f, 0.25f, 1.0f });

			UI_DrawString(va("%i", Demo_GetCurrentTick() - Demo_GetStartTick()), baseBarPositionX, baseBarPositionY * 0.995f, 0.5f, 0.5f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });

			// marker
			UI_DrawRectangle(baseBarPositionX + (baseBarWidth * percentage) - baseInnerBarOffset, baseBarPositionY + baseInnerBarOffset, 2, baseBarHeight - baseInnerBarOffset * 2, white, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });


			/* draw time markers for dolly nodes */
			for (std::pair<double, COSValue> node : nodes)
			{
				float percent = (float)((float)(node.first - Demo_GetStartTick()) / (float)(Demo_GetEndTick() - Demo_GetStartTick()));
				if (percent <= 1.0f)
					UI_DrawRectangle(baseBarPositionX + (baseBarWidth * percent) - baseInnerBarOffset, baseBarPositionY + baseInnerBarOffset, 2, baseBarHeight - baseInnerBarOffset * 2, white, vec4_t{ 1.0f, 0.7f, 0.0f, 1.0f });
			}
			
			/* draw time markers for orientation*/
			int markerStep = snapshotIntervalTime;
			const int MAX_TIME_MARKERS = 100;

			while ((Demo_GetCurrentTick() - Demo_GetStartTick()) / markerStep > MAX_TIME_MARKERS)
					markerStep *= 2;

			int currentTimeMarkers = 0;
			for (float offset = (float)markerStep; offset < (Demo_GetCurrentTick() - Demo_GetStartTick()); offset += markerStep)
			{
				currentTimeMarkers++;
				if (currentTimeMarkers > MAX_TIME_MARKERS)
					break;
			
				float percent = (float)((float)((Demo_GetStartTick() + offset) - Demo_GetStartTick()) / (float)(Demo_GetEndTick() - Demo_GetStartTick()));
				if (percent <= 0.99f)
					UI_DrawRectangle(baseBarPositionX + (baseBarWidth * percent), baseBarPositionY + baseInnerBarOffset, 2, (baseBarHeight - baseInnerBarOffset * 2), white, vec4_t{ 0.38f, 0.38f, 0.38f, 1.0f });
			}


			float baseButtonPositionX = baseBarPositionX;
			float baseButtonPositionY = baseBarPositionY + baseBarHeight + (refdef->windowSize[1] / 42.3529411765f); // + 17
			float mediumButtonWidth = refdef->windowSize[0] / 16.4102564103f; // 78
			float mediumButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float smallButtonWidth = refdef->windowSize[0] / 36.5714285714f; // 35
			float smallButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float bigButtonWidth = refdef->windowSize[0] / 10.6666666667f; // 120
			float bigButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float buttonDistance = refdef->windowSize[0] / 116.363636364f; // 11
			float keyDisplayY = baseButtonPositionY + (refdef->windowSize[1] / 13.0909090909f); // + 55

			// dolly mode button
			UI_DrawRectangle(baseButtonPositionX, baseButtonPositionY, mediumButtonWidth, mediumButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			if (!mvm_dolly_frozen->current.enabled)
				UI_DrawString("NORMAL", baseButtonPositionX + mediumButtonWidth * 0.1075f, baseButtonPositionY + mediumButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });
			else
				UI_DrawString("FROZEN", baseButtonPositionX + mediumButtonWidth * 0.1075f, baseButtonPositionY + mediumButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, vec4_t{ 0.0f, 0.8f, 1.0f, 1.0f });

			UI_DrawString("F4", baseButtonPositionX + mediumButtonWidth * 0.5f - (refdef->windowSize[0] / 142.222222222f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.back[2] < 1.0f)
			{
				buttonColorInfo.back[2] += 0.05f; // 20 frames
				buttonColorInfo.back[1] += 0.015f; // 20 frames
			}

			float backButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance;
			UI_DrawRectangle(backButtonX, baseButtonPositionY, mediumButtonWidth, mediumButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString("<<", backButtonX + mediumButtonWidth * 0.3f, baseButtonPositionY + mediumButtonHeight * 0.9f, 1.0f, 0.9f, objectiveFont, buttonColorInfo.back);

			UI_DrawString("LEFT", backButtonX + mediumButtonWidth * 0.5f - refdef->windowSize[0] / 71.1111111111f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.slower[2] < 1.0f)
			{
				buttonColorInfo.slower[2] += 0.05f; // 20 frames
				buttonColorInfo.slower[1] += 0.015f; // 20 frames
			}

			float slowerButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance;
			UI_DrawRectangle(slowerButtonX, baseButtonPositionY, smallButtonWidth, smallButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			if (timescale->current.value >= 1.0f)
				UI_DrawString("-", slowerButtonX + smallButtonWidth * 0.2f, baseButtonPositionY + smallButtonHeight * 1.1f, 1.2f, 1.2f, objectiveFont, buttonColorInfo.slower);
			else
				UI_DrawString(va("%.1f", timescale->current.value), slowerButtonX + smallButtonWidth * 0.12f, baseButtonPositionY + smallButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, buttonColorInfo.slower);

			UI_DrawString("DOWN", slowerButtonX + smallButtonWidth * 0.5f - refdef->windowSize[0] / 58.1818181818f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.pause[2] < 1.0f)
			{
				buttonColorInfo.pause[2] += 0.05f; // 20 frames
				buttonColorInfo.pause[1] += 0.015f; // 20 frames
			}

			float pauseButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance + smallButtonWidth + buttonDistance;
			UI_DrawRectangle(pauseButtonX, baseButtonPositionY, bigButtonWidth, bigButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			if (!Demo_IsPaused())
			{
				float pauseBarDistance = refdef->windowSize[0] / 160.0f; // 8
				float pauseBarWidth = bigButtonHeight * 0.4f - pauseBarDistance;
				float pauseBarHeight = bigButtonHeight * 0.6f;
				UI_DrawRectangle(pauseButtonX + bigButtonWidth * 0.5f - (pauseBarDistance * 0.5f + pauseBarWidth), baseButtonPositionY + bigButtonHeight * 0.2f, pauseBarWidth, pauseBarHeight, white, buttonColorInfo.pause);
				UI_DrawRectangle(pauseButtonX + bigButtonWidth * 0.5f + (pauseBarDistance * 0.5f), baseButtonPositionY + bigButtonHeight * 0.2f, pauseBarWidth, pauseBarHeight, white, buttonColorInfo.pause);
			}
			else
			{
				UI_DrawString(">", pauseButtonX + bigButtonWidth * 0.43f, baseButtonPositionY + bigButtonHeight * 0.975f, 1.2f, 1.12f, objectiveFont, buttonColorInfo.pause);
			}

			UI_DrawString("SPACE", pauseButtonX + bigButtonWidth * 0.5f - refdef->windowSize[0] / 51.2f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.faster[2] < 1.0f)
			{
				buttonColorInfo.faster[2] += 0.05f; // 20 frames
				buttonColorInfo.faster[1] += 0.015f; // 20 frames
			}

			float fasterButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + bigButtonWidth + buttonDistance;
			UI_DrawRectangle(fasterButtonX, baseButtonPositionY, smallButtonWidth, smallButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			if (timescale->current.value <= 1.0f)
				UI_DrawString("+", fasterButtonX + smallButtonWidth * 0.155f, baseButtonPositionY + smallButtonHeight * 1.1f, 1.2f, 1.2f, objectiveFont, buttonColorInfo.faster);
			else
				UI_DrawString(va("%.1f", timescale->current.value), fasterButtonX + smallButtonWidth * 0.11f, baseButtonPositionY + smallButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, buttonColorInfo.faster);

			UI_DrawString("UP", fasterButtonX + smallButtonWidth * 0.5f - refdef->windowSize[0] / 142.222222222f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.forward[2] < 1.0f)
			{
				buttonColorInfo.forward[2] += 0.05f; // 20 frames
				buttonColorInfo.forward[1] += 0.015f; // 20 frames
			}

			float forwardButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + bigButtonWidth + buttonDistance + smallButtonWidth + buttonDistance;
			UI_DrawRectangle(forwardButtonX, baseButtonPositionY, mediumButtonWidth, mediumButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString(">>", forwardButtonX + mediumButtonWidth * 0.3f, baseButtonPositionY + mediumButtonHeight * 0.9f, 1.0f, 0.9f, objectiveFont, buttonColorInfo.forward);

			UI_DrawString("RIGHT", forwardButtonX + mediumButtonWidth * 0.5f - refdef->windowSize[0] / 53.3333333333f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			// misc button
			float miscButtonX = baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + bigButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance;
			UI_DrawRectangle(baseButtonPositionX + mediumButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + bigButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + mediumButtonWidth + buttonDistance, baseButtonPositionY, mediumButtonWidth, mediumButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString("MISC", miscButtonX + mediumButtonWidth * 0.25f, baseButtonPositionY + mediumButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			UI_DrawString("CTRL", miscButtonX + mediumButtonWidth * 0.5f - refdef->windowSize[0] / 67.3684210526f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });
		}
		else if (currentMenu == MenuMode::Secondary)
		{

			float baseBarPositionX = baseFramePositionX + (refdef->windowSize[0] / 36.5714285714f); // 35.0f
			float baseBarPositionY = baseFramePositionY + (refdef->windowSize[1] / 16.0f); // 35.0f
			float baseBarWidth = refdef->windowSize[0] / 2.25352112676f; // 568.0f
			float baseBarHeight = refdef->windowSize[1] / 20.0f; // 36.0f
			float baseInnerBarOffset = refdef->windowSize[0] / 256.0f;

			float baseButtonPositionX = baseBarPositionX;
			float baseButtonPositionY = baseBarPositionY + refdef->windowSize[1] / 20.0f + (refdef->windowSize[1] / 42.3529411765f); // + 17
			float mediumButtonWidth = refdef->windowSize[0] / 16.4102564103f; // 78
			float mediumButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float smallButtonWidth = refdef->windowSize[0] / 36.5714285714f; // 35
			float smallButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float bigButtonWidth = refdef->windowSize[0] / 10.6666666667f; // 120
			float bigButtonHeight = refdef->windowSize[1] / 20.5714285714f; // 35
			float buttonDistance = refdef->windowSize[0] / 116.363636364f; // 11
			float keyDisplayY = baseButtonPositionY + (refdef->windowSize[1] / 13.0909090909f); // + 55

			UI_DrawString("IW3MVM", baseFramePositionX + baseFrameWidth * 0.5f - UI_TextWidth("IW3MVM", objectiveFont, 0.65f) * 0.5f, baseFramePositionY + baseFrameHeight * 0.225f, 0.65f, 0.65f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });

			std::string nodesPlacedString(va("%i NODES PLACED", nodes.size()));

			if (nodes.size() < requiredNodeCount)
				UI_DrawString(nodesPlacedString.c_str(), baseFramePositionX + baseFrameWidth * 0.5f - UI_TextWidth(nodesPlacedString.c_str(), objectiveFont, 0.5f) * 0.5f, baseFramePositionY + baseFrameHeight * 0.4f, 0.5f, 0.55f, objectiveFont, vec4_t{ 1.0f, 0.2f, 0.2f, 1.0f });
			else
				UI_DrawString(nodesPlacedString.c_str(), baseFramePositionX + baseFrameWidth * 0.5f - UI_TextWidth(nodesPlacedString.c_str(), objectiveFont, 0.5f) * 0.5f, baseFramePositionY + baseFrameHeight * 0.4f, 0.5f, 0.55f, objectiveFont, vec4_t{ 0.2f, 1.0f, 0.2f, 1.0f });


			/* GREENSCREEN */

			if (buttonColorInfo.greenscreen[2] < 1.0f)
			{
				buttonColorInfo.greenscreen[2] += 0.05f; // 20 frames
				buttonColorInfo.greenscreen[1] += 0.015f; // 20 frames
			}

			UI_DrawRectangle(baseButtonPositionX, baseButtonPositionY, bigButtonWidth, bigButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString("GREENSCREEN", baseButtonPositionX + bigButtonWidth * 0.5f - refdef->windowSize[0] / 23.2727273f, baseButtonPositionY + bigButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, buttonColorInfo.greenscreen);

			UI_DrawString("F3", baseButtonPositionX + bigButtonWidth * 0.5f - (refdef->windowSize[0] / 142.222222222f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });


			/* DEPTHMAP */

			if (buttonColorInfo.depth[2] < 1.0f)
			{
				buttonColorInfo.depth[2] += 0.05f; // 20 frames
				buttonColorInfo.depth[1] += 0.015f; // 20 frames
			}

			UI_DrawRectangle(baseButtonPositionX + bigButtonWidth + buttonDistance, baseButtonPositionY, bigButtonWidth, bigButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString("DEPTHMAP", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth * 0.5f - refdef->windowSize[0] / 31.2195121951f, baseButtonPositionY + bigButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, buttonColorInfo.depth);

			UI_DrawString("F5", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth * 0.5f - (refdef->windowSize[0] / 142.222222222f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });


			/* DOLLYCAM BINDS */

			if (buttonColorInfo.dollyadd[2] < 1.0f)
			{
				buttonColorInfo.dollyadd[2] += 0.05f; // 20 frames
				buttonColorInfo.dollyadd[1] += 0.015f; // 20 frames
			}

			UI_DrawRectangle(baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3, baseButtonPositionY, smallButtonWidth, smallButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });
			UI_DrawString("ADD", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth * 0.5f - refdef->windowSize[0] / 106.666666667f, baseButtonPositionY + bigButtonHeight * 0.7f, 0.4f, 0.4f, objectiveFont, buttonColorInfo.dollyadd);
			UI_DrawString("K", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth * 0.5f - (refdef->windowSize[0] / 320.0f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.dollyplay[2] < 1.0f)
			{
				buttonColorInfo.dollyplay[2] += 0.05f; // 20 frames
				buttonColorInfo.dollyplay[1] += 0.015f; // 20 frames
			}

			UI_DrawRectangle(baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance, baseButtonPositionY, smallButtonWidth, smallButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });
			UI_DrawString("PLAY", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth * 0.5f - refdef->windowSize[0] / 80.0f, baseButtonPositionY + bigButtonHeight * 0.7f, 0.4f, 0.4f, objectiveFont, buttonColorInfo.dollyplay);
			UI_DrawString("J", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth * 0.5f - (refdef->windowSize[0] / 320.0f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			if (buttonColorInfo.dollydel[2] < 1.0f)
			{
				buttonColorInfo.dollydel[2] += 0.05f; // 20 frames
				buttonColorInfo.dollydel[1] += 0.015f; // 20 frames
			}

			UI_DrawRectangle(baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance, baseButtonPositionY, smallButtonWidth, smallButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });
			UI_DrawString("DEL", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + smallButtonWidth * 0.5f - refdef->windowSize[0] / 116.363636364f, baseButtonPositionY + bigButtonHeight * 0.7f, 0.4f, 0.4f, objectiveFont, buttonColorInfo.dollydel);
			UI_DrawString("L", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + smallButtonWidth * 0.5f - (refdef->windowSize[0] / 320.0f), keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });


			/* CAMERA ROLL */

			UI_DrawRectangle(baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance * 3, baseButtonPositionY, bigButtonWidth, bigButtonHeight, white, vec4_t{ 0.1f, 0.1f, 0.1f, 0.75f });

			UI_DrawString("CAMERA ROLL", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance * 3 + bigButtonWidth * 0.5f - refdef->windowSize[0] / 24.1509433962f, baseButtonPositionY + bigButtonHeight * 0.75f, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });

			UI_DrawString("MWHEEL", baseButtonPositionX + bigButtonWidth + buttonDistance + bigButtonWidth + buttonDistance * 3 + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance + smallButtonWidth + buttonDistance * 3 + bigButtonWidth * 0.5f - refdef->windowSize[0] / 42.6666666667f, keyDisplayY, 0.5f, 0.5f, objectiveFont, vec4_t{ 1, 1, 1, 1 });
		}
	}
}

uint32_t CG_Draw2D_tramp;
void __declspec(naked) hkCG_Draw2D()
{
	__asm pushad

	CG_Draw2D();

	__asm mov al, currentView.isPOV
	__asm test al, al
	__asm popad
	__asm jne Draw2D
	__asm ret
	Draw2D:
		  __asm jmp CG_Draw2D_tramp
}

void Patch_Drawing()
{
	// hook the game's main 2D drawing function so we can draw our own menu
	CG_Draw2D_tramp = Hook<uint32_t>(CG_Draw2D_Address, hkCG_Draw2D);
}