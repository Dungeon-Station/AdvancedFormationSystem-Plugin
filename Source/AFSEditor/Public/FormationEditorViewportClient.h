/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "FormationAsset.h"
#include "Templates/SharedPointer.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorComponents.h"
#include "Components/PostProcessComponent.h"

class FFormationEditorToolkit;

UENUM()
enum class EFormationShowFlags : uint8
{
    None            = 0,      
    ForwardArrow    = 1 << 0,
    VirtualLeader   = 1 << 1,
    PriorityNumbers = 1 << 2,
    FormationRadius = 1 << 3,
    DebugSpheres    = 1 << 4,
    Grid            = 1 << 5,
    All = ForwardArrow | VirtualLeader | PriorityNumbers | FormationRadius | DebugSpheres | Grid
};

ENUM_CLASS_FLAGS(EFormationShowFlags);

class FFormationEditorViewportClient : public FEditorViewportClient, public TSharedFromThis<FFormationEditorViewportClient> 
{
public:
    FFormationEditorViewportClient(UFormationAsset* InAsset, FPreviewScene* InPreviewScene);
    ~FFormationEditorViewportClient();

    void DrawPriorityNumbers(FViewport* InViewport, FCanvas* Canvas);
    bool CheckGroupChanged();
    virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override; 
    virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
    void DrawCustomGrid(FPrimitiveDrawInterface* PDI);
    virtual void UpdateMouseDelta() override;

     // Input And Select Actor Functions
    virtual bool CanSelectWidget() const { return true; }
    virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
    virtual FVector GetWidgetLocation() const override;
    virtual void SetWidgetMode(UE::Widget::EWidgetMode NewMode) override;
    virtual bool UsesTransformWidget() const  { return true; }
    virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual void TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge ) override;
    virtual void TrackingStopped() override;
    virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
    
    TArray<int32> RefreshPreviewActors();
    void SelectActor(AActor* NewActor, bool bToggle = false);
	void DeselectActor(AActor* ActorToDeselect);
    void ClearSelection(bool bNotify = true);
    bool IsActorSelected(AActor* InActor) const;
    void SelectActorByIndex(int32 Index);
	const TArray<int32>& GetSelectedIndices() const { return SelectedIndices; }

    void OnPostUndoRedo();
    void ForceDestroyPreviewActors();

    void SetEditorToolkit(TSharedPtr<FFormationEditorToolkit> InToolkit);
private:
    FDelegateHandle PostUndoRedoHandle;
    UFormationAsset* EditedFormation;
    TArray<TWeakObjectPtr<AActor>> PreviewActors;
    bool bInitialized = false;

    TArray<TWeakObjectPtr<AActor>> SelectedActors;
    TArray<int32> SelectedIndices;

    UE::Widget::EWidgetMode CurrentWidgetMode = UE::Widget::WM_Translate;
    bool bIsDragging = false;
    bool bIsDuplicating = false;
    bool bBoxSelecting = false;
	FIntPoint BoxSelectStart;
    FIntPoint BoxSelectEnd;

    TArray<FAgentData> CopiedAgentDataClipboard;

    TWeakObjectPtr<AActor> VirtualLeaderActor;
    bool bIsVirtualLeaderSelected = false;

    TObjectPtr<UStaticMesh> LeaderMesh;
    TObjectPtr<UMaterial> LeaderMaterial;

    TWeakPtr<FFormationEditorToolkit> EditorToolkit;

    FVector AccumulatedDrag = FVector::ZeroVector;

    UPROPERTY()
    TObjectPtr<UPostProcessComponent> OutlinePostProcessComponent;

    static constexpr int32 OutlineStencilValue = 1;
private:
    void SpawnPreviewActors();
    void UpdatePreviewActors();
    void DestroyPreviewActors();
    void DrawDebugSpheres(FPrimitiveDrawInterface* PDI) const;
    void DrawFormationRadius(FPrimitiveDrawInterface* PDI);
    void DrawVirtualLeader(FPrimitiveDrawInterface* PDI);
    void DrawForwardArrow(FPrimitiveDrawInterface* PDI);
    
    bool IsSnappingKeyPressed() const;
    void SwitchToOrthographicView(const FVector& NewDirection, ELevelViewportType NewViewportType);

    void SetHighlight(AActor* Actor, bool bEnable);

    void NotifySelectionChanged();

    void HandleAgentCountChanged();
    void HandleAgentsDataChanged(const TArray<int32>& AgentIndices);
    void HandleGroupPresetsChanged();
public: // Set ViewMode Functions
    void SetTopView();
    void SetBottomView();
    void SetLeftView();
    void SetRightView();
    void SetFrontView();
    void SetBackView();
    void SetPerspectiveView();
    static FRotator LookAtOrigin(const FVector& CameraPos)
    {
        return (FVector(0,0,0) - CameraPos).Rotation();
    }

public: // Snapping Valuse; 
    float SnapValue = 10.0f;
    bool bEnableSnapping = true;

    // Manage View Mode
public:
    void SetAFSShowFlags(EFormationShowFlags NewMode);
    EFormationShowFlags GetShowFlags() const { return CurrentShowFlags; }

private:
    EFormationShowFlags CurrentShowFlags = EFormationShowFlags::All;

};
