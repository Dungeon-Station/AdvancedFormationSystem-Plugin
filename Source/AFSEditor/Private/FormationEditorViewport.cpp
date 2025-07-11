
#include "FormationEditorViewport.h"

#include "AdvancedPreviewScene.h"
#include "EditorStyleSet.h"
#include "FormationEditorViewportClient.h"
#include "Widgets/Input/SSlider.h"

void SFormationEditorViewport::Construct(const FArguments& InArgs)
{
    FormationAsset = InArgs._FormationAsset;
    PreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));
    ViewportClient = MakeShareable(new FFormationEditorViewportClient(FormationAsset, PreviewScene.Get()));

    SEditorViewport::Construct(SEditorViewport::FArguments());

    if (PreviewScene.IsValid())
    {
        if (const UStaticMeshComponent* Floor = PreviewScene->GetFloorMeshComponent())
        {
            UStaticMeshComponent* MutableFloor = const_cast<UStaticMeshComponent*>(Floor);

            if (MutableFloor)
            {
                MutableFloor->SetRelativeScale3D(FVector(10.0f, 10.0f, 1.0f));
            }
        }
    }


    if (ViewOptions.Num() > 0)
    {
        CurrentViewOption = ViewOptions[ViewOptions.Num() - 1];
        ViewComboBox->SetSelectedItem(CurrentViewOption);
    }

    InitializeSnapValues();
    InitializeViewOptions();
    InitializeShowFlagsOptions();
    
    this->ChildSlot
        [
            SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0, 8, 0, 8))
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
                        [
                            SNew(STextBlock)
                                .Text(NSLOCTEXT("FormationEditor", "ShowFlagsLabel", "Show Flags"))
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
                        [
                            SNew(SComboButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(2, 2))
                                .ButtonContent()
                                [
                                    SNew(SImage)
                                        .Image(FAppStyle::Get().GetBrush("Level.VisibleIcon16x"))
                                        .ColorAndOpacity(FLinearColor::White)
                                ]
                                .MenuContent()
                                [
                                    SNew(SListView<TSharedPtr<EFormationShowFlags>>)
                                        .ListItemsSource(&ShowFlagsOptions)
                                        .OnGenerateRow(this, &SFormationEditorViewport::GenerateShowFlagsRow)
                                        .SelectionMode(ESelectionMode::None)
                                ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
                        [
                            SNew(STextBlock)
                                .Text(NSLOCTEXT("FormationEditor", "ViewModeLabel", "View Modes "))
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
                        [
                            SAssignNew(ViewComboBox, SComboBox<TSharedPtr<FViewOption>>)
                                .OptionsSource(&ViewOptions)
                                .OnGenerateWidget(this, &SFormationEditorViewport::GenerateViewOptionWidget)
                                .OnSelectionChanged(this, &SFormationEditorViewport::OnViewOptionSelected)
                                .Content()
                                [
                                    SNew(STextBlock)
                                        .Text(this, &SFormationEditorViewport::GetCurrentViewName)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                                ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 4, 0)
                        [
                            SNew(STextBlock)
                                .Text(NSLOCTEXT("FormationEditor", "SnapLabel", "Snap Interval"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
                        [
                            SNew(SBox)
                                .WidthOverride(80)
                                [
                                    SAssignNew(SnapComboBox, SComboBox<TSharedPtr<float>>)
                                        .OptionsSource(&SnapValues)
                                        .OnGenerateWidget(this, &SFormationEditorViewport::GenerateSnapValueWidget)
                                        .OnSelectionChanged(this, &SFormationEditorViewport::OnSnapValueSelected)
                                        .ContentPadding(FMargin(4, 2))
                                        .Content()
                                        [
                                            SNew(STextBlock)
                                                .Text(this, &SFormationEditorViewport::GetSnapValueText)
                                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                                        ]
                                ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
                        [
                            SNew(SCheckBox)
                                .IsChecked(this, &SFormationEditorViewport::IsSnapEnabled)
                                .OnCheckStateChanged(this, &SFormationEditorViewport::OnSnapToggled)
                                .ToolTipText(NSLOCTEXT("FormationEditor", "SnapToggle", "Enable/Disable Snapping"))
                                [
                                    SNew(STextBlock)
                                        .Text(NSLOCTEXT("FormationEditor", "SnapEnable", "Snap"))
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                                ]
                        ]
                ]
                + SVerticalBox::Slot().FillHeight(1.0f)
                [
                    ViewportWidget.ToSharedRef()
                ]
        ];

    
    if (SnapComboBox.IsValid() && ViewportClient.IsValid())
    {
        for (auto& Value : SnapValues)
        {
            if (FMath::IsNearlyEqual(*Value, ViewportClient->SnapValue, 0.01f))
            {
                SnapComboBox->SetSelectedItem(Value);
                break;
            }
        }
    }

    if (ViewModeComboBox.IsValid())
    {
        ViewModeComboBox->Invalidate(EInvalidateWidget::Layout);
        ViewModeComboBox->Invalidate(EInvalidateWidget::Paint);
    }
    
}

TSharedRef<ITableRow> SFormationEditorViewport::GenerateShowFlagsRow(
    TSharedPtr<EFormationShowFlags> InMode,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<EFormationShowFlags>>, OwnerTable)
        [
            GenerateShowFlagsWidget(InMode)
        ];
}

void SFormationEditorViewport::InitializeSnapValues()
{
    SnapValues.Empty();
    
    TArray<float> DefaultSnapSizes = {
        0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 25.0f, 50.0f, 100.0f
    };
    
    for (float Size : DefaultSnapSizes)
    {
        SnapValues.Add(MakeShareable(new float(Size)));
    }
}

TSharedRef<FEditorViewportClient> SFormationEditorViewport::MakeEditorViewportClient()
{
    return ViewportClient.ToSharedRef();
}


void SFormationEditorViewport::OnSnapValueSelected(TSharedPtr<float> NewValue, ESelectInfo::Type SelectType)
{
    if (NewValue.IsValid() && ViewportClient.IsValid())
    {
        ViewportClient->SnapValue = *NewValue;
    }
}

ECheckBoxState SFormationEditorViewport::IsSnapEnabled() const
{
    return ViewportClient.IsValid() && ViewportClient->bEnableSnapping ? 
    ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFormationEditorViewport::OnSnapToggled(ECheckBoxState NewState)
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->bEnableSnapping = (NewState == ECheckBoxState::Checked);
    }
}

TSharedRef<SWidget> SFormationEditorViewport::GenerateSnapValueWidget(TSharedPtr<float> InValue)
{
    FString ValueText;
    if (*InValue < 1.0f)
    {
        ValueText = FString::Printf(TEXT("%.1f"), *InValue);
    }
    else if (*InValue >= 1000.0f)
    {
        ValueText = FString::Printf(TEXT("%.0f"), *InValue / 1000.0f) + TEXT("K");
    }
    else
    {
        ValueText = FString::Printf(TEXT("%.0f"), *InValue);
    }
    
    return SNew(STextBlock)
        .Text(FText::FromString(ValueText))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10));
}

FText SFormationEditorViewport::GetSnapValueText() const
{
    if (ViewportClient.IsValid())
    {
        float Value = ViewportClient->SnapValue;
        FString ValueText;
        
        if (Value < 1.0f)
        {
            ValueText = FString::Printf(TEXT("%.1f"), Value);
        }
        else if (Value >= 1000.0f)
        {
            ValueText = FString::Printf(TEXT("%.0f"), Value / 1000.0f) + TEXT("K");
        }
        else
        {
            ValueText = FString::Printf(TEXT("%.0f"), Value);
        }
        
        return FText::FromString(ValueText);
    }
    return FText::FromString(TEXT("10"));
}
void SFormationEditorViewport::InitializeViewOptions()
{
    ViewOptions.Empty();
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Top"), 
        [this]() { ViewportClient->SetTopView(); }
    )));

    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Bottom"),
        [this]() { ViewportClient->SetBottomView(); }
    )));
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Left"), 
        [this]() { ViewportClient->SetLeftView(); }
    )));
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Right"), 
        [this]() { ViewportClient->SetRightView(); }
    )));
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Front"), 
        [this]() { ViewportClient->SetFrontView(); }
    )));
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Back"), 
        [this]() { ViewportClient->SetBackView(); }
    )));
    
    ViewOptions.Add(MakeShareable(new FViewOption(
        FText::FromString("Perspective"), 
        [this]() { ViewportClient->SetPerspectiveView(); }
    )));
}

TSharedRef<SWidget> SFormationEditorViewport::GenerateViewOptionWidget(TSharedPtr<FViewOption> InOption)
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .VAlign(VAlign_Center)
        .AutoWidth()
        + SHorizontalBox::Slot()
        .VAlign(VAlign_Center)
        .AutoWidth()
        [
            SNew(STextBlock)
                .Text(InOption->Name)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
        ];
}

void SFormationEditorViewport::OnViewOptionSelected(TSharedPtr<FViewOption> NewOption, ESelectInfo::Type SelectType)
{
    if (NewOption.IsValid())
    {
        CurrentViewOption = NewOption;
        
        NewOption->Action();
        
        ViewComboBox->SetSelectedItem(NewOption);
    }
}

FText SFormationEditorViewport::GetCurrentViewName() const
{
    if (CurrentViewOption.IsValid())
    {
        return CurrentViewOption->Name;
    }
    return NSLOCTEXT("FormationEditor", "ViewDefault", "Perspective");
}

void SFormationEditorViewport::InitializeShowFlagsOptions()
{
    ShowFlagsOptions.Empty();
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::ForwardArrow));
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::VirtualLeader));
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::PriorityNumbers));
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::FormationRadius));
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::DebugSpheres));
    ShowFlagsOptions.Add(MakeShared<EFormationShowFlags>(EFormationShowFlags::Grid));

    SelectedViewModes.Empty();

    for (const auto& ModePtr : ShowFlagsOptions)
    {
        SelectedViewModes.Add(*ModePtr);
    }

    if (ViewportClient.IsValid())
    {
        EFormationShowFlags Result = EFormationShowFlags::None;

        for (EFormationShowFlags M : SelectedViewModes)
            Result = static_cast<EFormationShowFlags>(static_cast<uint8>(Result) | static_cast<uint8>(M));
        
        ViewportClient->SetAFSShowFlags(Result);
        ViewportClient->Invalidate();
    }
}

TSharedRef<SWidget> SFormationEditorViewport::GenerateShowFlagsWidget(TSharedPtr<EFormationShowFlags> InMode)
{
    FString ModeName;
    FName IconName;

    switch (*InMode)
    {
    case EFormationShowFlags::ForwardArrow:
        ModeName = TEXT("Arrow");
        IconName = "Icons.ArrowRight";
        break;
    case EFormationShowFlags::VirtualLeader:
        ModeName = TEXT("Leader");
        IconName = "ClassIcon.Pawn";
        break;
    case EFormationShowFlags::PriorityNumbers:
        ModeName = TEXT("Priority");
        IconName = "Icons.SortNumeric";
        break;
    case EFormationShowFlags::FormationRadius:
        ModeName = TEXT("Radius");
        IconName = "EditorViewport.ScaleMode";
        break;
    case EFormationShowFlags::DebugSpheres:
        ModeName = TEXT("Spheres");
        IconName = "ClassIcon.SphereReflectionCapture";
        break;
    case EFormationShowFlags::Grid:
        ModeName = TEXT("Grid");
        IconName = "EditorViewport.Grid";
        break;
    default:
        ModeName = TEXT("Unknown");
        IconName = "Icons.WarningWithColor";
        break;
    }

    return SNew(SBox)
        .MinDesiredWidth(180.f)
        .Padding(8, 2, 0, 2)
        [
            SNew(SCheckBox)
                .Style(FAppStyle::Get(), "Menu.CheckBox")
                .IsChecked_Lambda([this, InMode]() {
                return SelectedViewModes.Contains(*InMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                    })
                .OnCheckStateChanged_Lambda([this, InMode](ECheckBoxState NewState) {
                OnViewModeCheckChanged(NewState, *InMode);
                    })
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                        [
                            SNew(SImage)
                                .Image(FAppStyle::Get().GetBrush(IconName))
                                .ColorAndOpacity(FLinearColor::White)
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(ModeName))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                                .ColorAndOpacity(FSlateColor::UseForeground())
                        ]
                ]
        ];
}



void SFormationEditorViewport::OnViewModeCheckChanged(ECheckBoxState NewState, EFormationShowFlags Mode)
{
    if (NewState == ECheckBoxState::Checked)
        SelectedViewModes.Add(Mode);
    else
        SelectedViewModes.Remove(Mode);

    if (ViewportClient.IsValid())
    {
        EFormationShowFlags Result = EFormationShowFlags::None;
        for (EFormationShowFlags M : SelectedViewModes)
            Result = static_cast<EFormationShowFlags>(static_cast<uint8>(Result) | static_cast<uint8>(M));
        ViewportClient->SetAFSShowFlags(Result);
        ViewportClient->Invalidate();
    }
}

FText SFormationEditorViewport::GetSelectedViewModesText() const
{
    if (SelectedViewModes.Num() == 0)
        return NSLOCTEXT("FormationEditor", "NoneSelected", "None");
    FString Result;
    for (EFormationShowFlags Mode : SelectedViewModes)
    {
        if (!Result.IsEmpty()) Result += TEXT(", ");
        switch (Mode)
        {
        case EFormationShowFlags::ForwardArrow:    Result += TEXT("Arrow"); break;
        case EFormationShowFlags::VirtualLeader:   Result += TEXT("Leader"); break;
        case EFormationShowFlags::PriorityNumbers: Result += TEXT("Priority"); break;
        case EFormationShowFlags::FormationRadius: Result += TEXT("Radius"); break;
        case EFormationShowFlags::DebugSpheres:    Result += TEXT("Spheres"); break;
        case EFormationShowFlags::Grid:            Result += TEXT("Grid"); break;
        default:                                   Result += TEXT("Unknown"); break;
        }
    }
    return FText::FromString(Result);
}