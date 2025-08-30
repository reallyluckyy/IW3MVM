#pragma once

#include "stdafx.hpp"
#include "game.hpp"
#include "gfxworld.hpp"
#include "util.hpp"

extern std::map<char, std::string> semanticNames;

void WriteCodm(std::ofstream& ofile, std::vector<codm_Object> objects, std::vector<codm_XModel> xmodels);
void WriteObj(std::ofstream& ofile, GfxWorld gfxWorld);
void WriteMmf(std::ofstream& ofile, GfxWorld gfxWorld);
void WriteMmat(std::ofstream& ofile, GfxWorld gfxWorld);
