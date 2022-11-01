#pragma once

#include "ProjectInfo.h"

int compile(TranslationUnit tu, ProjectInfo *p_Proj_Info);

int link_App(ProjectInfo *p_Proj_Info);

int link_Lib(ProjectInfo *p_Proj_Info);