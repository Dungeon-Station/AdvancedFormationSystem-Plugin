#pragma once
#include "Toolkits/AssetEditorToolkit.h"

class UFormationAsset;
class SFormationEditorViewport;
class IDetailsView;

class FFormationEditorToolkit : public FAssetEditorToolkit
{
public:
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

    virtual const TSharedRef<FTabManager::FLayout> GetLayout() const;

    void Initialize(
        const EToolkitMode::Type Mode,
        const TSharedPtr<IToolkitHost>& InitToolkitHost,
        UFormationAsset* FormationAsset);

    virtual FName GetToolkitFName() const override { return FName("FormationEditor"); }
    virtual FText GetBaseToolkitName() const override { return FText::FromString("Formation Editor"); }
    virtual FString GetWorldCentricTabPrefix() const override { return "Formation "; }
    virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.3f, 0.2f, 0.5f); }
    
    TSharedRef<SDockTab> SpawnMainTab(const FSpawnTabArgs& Args);

    static const FName MainTabID;
    static const FName ViewportTabID;
    static const FName PreviewSettingsTabID;
private:
    UFormationAsset* EditedAsset = nullptr;
    TSharedPtr<SFormationEditorViewport> ViewportWidget;

    TSharedPtr<IDetailsView> DetailsView;

    void OnPropertySelectedInDetailsView(const FProperty* InProperty);

protected:
    TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnPreviewSettingsTab(const FSpawnTabArgs& Args);
};
