/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "Modules/ModuleManager.h"

class FAFSModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
