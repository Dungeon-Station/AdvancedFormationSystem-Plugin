/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "AFS.h"

#define LOCTEXT_NAMESPACE "FAFSModule"

void FAFSModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FAFSModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_GAME_MODULE(FAFSModule, AFS)