#include "../Public/AFSEditorModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"  // Asset Tools 모듈
#include "AssetTypeAction_Formation.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "IAssetTools.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"

IMPLEMENT_MODULE(FAFSEditorModule, AFSEditor);

void FAFSEditorModule::StartupModule()
{
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    
    auto Action = MakeShared<FAssetTypeActions_Formation>();
    AssetToolsModule.Get().RegisterAssetTypeActions(Action);
    RegisteredAssetTypeActions.Add(Action);
}

void FAFSEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
        for (auto Action : RegisteredAssetTypeActions)
        {
            AssetToolsModule.Get().UnregisterAssetTypeActions(Action);
        }
    }
}
