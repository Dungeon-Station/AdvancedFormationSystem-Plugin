#include "AssetTypeAction_Formation.h"
#include "FormationAsset.h"
#include "FormationEditorToolkit.h" 
#include "Toolkits/AssetEditorToolkit.h"

UClass* FAssetTypeActions_Formation::GetSupportedClass() const
{
    return UFormationAsset::StaticClass();
}

void FAssetTypeActions_Formation::OpenAssetEditor(
    const TArray<UObject*>& InObjects,
    TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ?
        EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (UObject* Object : InObjects)
    {
        if (UFormationAsset* FormationAsset = Cast<UFormationAsset>(Object))
        {
            TSharedRef<FFormationEditorToolkit> EditorToolkit = MakeShared<FFormationEditorToolkit>();
            EditorToolkit->Initialize(Mode, EditWithinLevelEditor, FormationAsset);
        }
    }
}

