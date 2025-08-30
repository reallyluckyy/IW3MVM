#pragma once

#include "stdafx.hpp"
#include "game.hpp"
#include "util.hpp"
#include "codm.hpp"

extern std::vector<codm_Object> LoadMapObjects(GfxWorld& gfxWorld);
extern void Merge(std::vector<codm_Object>& objects);
