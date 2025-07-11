#pragma once

#include "CoreMinimal.h"
#include "FormationEditorViewportClient.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FFormationEditorViewportClient;
class UFormationAsset;

struct FViewOption
{
    FText Name;
    TFunction<void()> Action;
    
    FViewOption(const FText& InName, TFunction<void()> InAction)
        : Name(InName), Action(InAction) {}
};

class SFormationEditorViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SFormationEditorViewport) {}
        SLATE_ARGUMENT(UFormationAsset*, FormationAsset)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

    TSharedPtr<FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }

    FText GetSnapValueText() const;
    void OnSnapValueSelected(TSharedPtr<float> NewValue, ESelectInfo::Type SelectType);
    
    ECheckBoxState IsSnapEnabled() const;
    void OnSnapToggled(ECheckBoxState NewState);
private:
    TSharedPtr<FFormationEditorViewportClient> ViewportClient;
    UFormationAsset* FormationAsset = nullptr;
    TSharedPtr<FAdvancedPreviewScene> PreviewScene;

    TArray<TSharedPtr<float>> SnapValues;
    TSharedPtr<SComboBox<TSharedPtr<float>>> SnapComboBox;

    TSharedRef<SWidget> GenerateSnapValueWidget(TSharedPtr<float> InValue);
    void InitializeSnapValues();
    const float MinSnap = 1.0f;
    const float MaxSnap = 100.0f;

private:
    TArray<TSharedPtr<FViewOption>> ViewOptions;
    TSharedPtr<SComboBox<TSharedPtr<FViewOption>>> ViewComboBox;
    TSharedPtr<FViewOption> CurrentViewOption;
    
    void InitializeViewOptions();
    TSharedRef<SWidget> GenerateViewOptionWidget(TSharedPtr<FViewOption> InOption);
    void OnViewOptionSelected(TSharedPtr<FViewOption> NewOption, ESelectInfo::Type SelectType);
    FText GetCurrentViewName() const;
    
    //ViewMode
private:
    TArray<TSharedPtr<EFormationShowFlags>> ShowFlagsOptions;
    TSet<EFormationShowFlags> SelectedViewModes;
    TSharedPtr<SComboBox<TSharedPtr<EFormationShowFlags>>> ViewModeComboBox;

    void InitializeShowFlagsOptions();
    
    TSharedRef<ITableRow> GenerateShowFlagsRow(TSharedPtr<EFormationShowFlags> InMode, const TSharedRef<STableViewBase>& OwnerTable);
    TSharedRef<SWidget> GenerateShowFlagsWidget(TSharedPtr<EFormationShowFlags> InMode);
    
    FText GetSelectedViewModesText() const;

    void OnViewModeCheckChanged(ECheckBoxState NewState, EFormationShowFlags Mode);
    void OnComboBoxSelectionChanged(TSharedPtr<EFormationShowFlags> /*Unused*/, ESelectInfo::Type) {}
    
};
