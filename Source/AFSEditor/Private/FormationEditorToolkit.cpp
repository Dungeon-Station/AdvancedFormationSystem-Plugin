// FormationEditorToolkit.cpp

#include "FormationEditorToolkit.h"
#include "Widgets/Docking/SDockTab.h"
#include "AFS/Public/FormationAsset.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "FormationEditorViewport.h"
#include "Widgets/Docking/SDockTab.h"
#include "SAdvancedPreviewDetailsTab.h"
#include "AdvancedPreviewScene.h"
#include "IDetailsView.h"


const FName FFormationEditorToolkit::MainTabID = TEXT("AFS_MainTab");
const FName FFormationEditorToolkit::ViewportTabID = TEXT("AFS_ViewportTab");
const FName FFormationEditorToolkit::PreviewSettingsTabID(TEXT("AFS_PreviewSettings"));

void FFormationEditorToolkit::Initialize(
    const EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& InitToolkitHost,
    UFormationAsset* FormationAsset)
{

    EditedAsset = FormationAsset;
    
    /*const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("FormationEditor_Layout")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->Split(FTabManager::NewStack()
                ->AddTab(FName(MainTabID), ETabState::OpenedTab)
                ->AddTab(FName(ViewportTabID), ETabState::OpenedTab)
            )
        );*/

    const TSharedRef<FTabManager::FLayout> Layout = GetLayout();

    TArray<UObject*> ObjectsToEdit;
    if (FormationAsset && FormationAsset->IsValidLowLevel())
    {
        ObjectsToEdit.Add(FormationAsset);
    }
    else
    {
        return;
    }
    
    InitAssetEditor(
        Mode,
        InitToolkitHost,
        "FormationEditor",
        Layout,
        true,
        true,
        ObjectsToEdit
    );
}

void FFormationEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    
    /*if (!InTabManager->HasTabSpawner(MainTabID))
    {
        InTabManager->RegisterTabSpawner(MainTabID,
            FOnSpawnTab::CreateSP(this, &FFormationEditorToolkit::SpawnMainTab));
    }
    
    if (!InTabManager->HasTabSpawner(ViewportTabID))
    {
        InTabManager->RegisterTabSpawner(ViewportTabID,
            FOnSpawnTab::CreateSP(this, &FFormationEditorToolkit::SpawnViewportTab));
    }*/

    // WorkspaceMenuStructure를 설정하여 Window 메뉴에 에디터를 등록합니다.
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(FText::FromString("Formation Editor"));
    auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

    // 뷰포트 탭 등록
    InTabManager->RegisterTabSpawner(ViewportTabID, FOnSpawnTab::CreateSP(this, &FFormationEditorToolkit::SpawnViewportTab))
        .SetDisplayName(FText::FromString("Viewport"))
        .SetGroup(WorkspaceMenuCategoryRef);

    // 메인 탭 등록
    InTabManager->RegisterTabSpawner(MainTabID, FOnSpawnTab::CreateSP(this, &FFormationEditorToolkit::SpawnMainTab))
        .SetDisplayName(FText::FromString("Main"))
        .SetGroup(WorkspaceMenuCategoryRef);

    // [추가] Preview Settings 탭 등록
    InTabManager->RegisterTabSpawner(PreviewSettingsTabID, FOnSpawnTab::CreateSP(this, &FFormationEditorToolkit::SpawnPreviewSettingsTab))
        .SetDisplayName(FText::FromString("Preview Scene Settings"))
        .SetGroup(WorkspaceMenuCategoryRef);
}

TSharedRef<SDockTab> FFormationEditorToolkit::SpawnMainTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == MainTabID);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.bLockable = false;
    DetailsViewArgs.bUpdatesFromSelection = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
    
    DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);

    

    if (EditedAsset)
    {
        TArray<UObject*> ObjectsToEdit;
        ObjectsToEdit.Add(EditedAsset);
        DetailsView->SetObjects(ObjectsToEdit);
    }

    return SNew(SDockTab)
        .Label(FText::FromString("Formation Details"))
        [
            DetailsView.ToSharedRef()
        ];
}

void FFormationEditorToolkit::OnPropertySelectedInDetailsView(const FProperty* InProperty)
{
}

TSharedRef<SDockTab> FFormationEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
    if (!EditedAsset)
    {
        return SNew(SDockTab)
            .Label(FText::FromString("Error"))
            [
                SNew(STextBlock)
                    .Text(FText::FromString("Invalid asset"))
            ];
    }

    ViewportWidget = SNew(SFormationEditorViewport)
        .FormationAsset(EditedAsset);

    return SNew(SDockTab)
        .Label(FText::FromString("Formation Eidtor"))
        [
            ViewportWidget.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FFormationEditorToolkit::SpawnPreviewSettingsTab(const FSpawnTabArgs& Args)
{
    check(ViewportWidget.IsValid());

    // 뷰포트 위젯으로부터 PreviewScene을 가져옵니다.
    TSharedRef<FAdvancedPreviewScene> SceneRef = ViewportWidget->GetPreviewScene().ToSharedRef();
    // SAdvancedPreviewDetailsTab 위젯을 생성하고, 가져온 Scene을 넘겨줍니다.
    TSharedRef<SWidget> PreviewSettingsWidget = SNew(SAdvancedPreviewDetailsTab, SceneRef);

    return SNew(SDockTab)
        .Label(FText::FromString("Preview Scene Settings"))
        [
            PreviewSettingsWidget
        ];
}

void FFormationEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    if (InTabManager->HasTabSpawner(MainTabID)) {
        InTabManager->UnregisterTabSpawner(MainTabID);
    }
    
    if (InTabManager->HasTabSpawner(ViewportTabID)) {
        InTabManager->UnregisterTabSpawner(ViewportTabID);
    }
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

// 에디터 레이아웃 정의 함수 구현
const TSharedRef<FTabManager::FLayout> FFormationEditorToolkit::GetLayout() const
{
    // 에디터의 기본 레이아웃을 정의합니다.
    return FTabManager::NewLayout("FormationEditor_Layout_Test")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                // 왼쪽 영역: 뷰포트가 70% 차지
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.7f)
                ->AddTab(ViewportTabID, ETabState::OpenedTab)
                ->SetHideTabWell(true) // 탭이 하나일 때 탭바를 숨기는 옵션
            )
            ->Split
            (
                // 오른쪽 영역: 메인(디테일) 탭이 30% 차지
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.3f)
                ->AddTab(MainTabID, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
        );
}