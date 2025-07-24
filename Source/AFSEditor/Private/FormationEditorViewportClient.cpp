/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "FormationEditorViewportClient.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "FormationAsset.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "HitProxies.h"
#include "UnrealWidget.h"
#include "ScopedTransaction.h"
#include "FormationEditorToolkit.h"

FFormationEditorViewportClient::FFormationEditorViewportClient(UFormationAsset* InAsset,
   FPreviewScene* InPreviewScene) : FEditorViewportClient(nullptr, InPreviewScene), EditedFormation(InAsset)
{
    SetRealtime(true);
    SetViewLocation(FVector(0, 0, 500));
    SetViewRotation(FRotator(-30, 45, 0));

    OutlinePostProcessComponent = NewObject<UPostProcessComponent>();
    if (OutlinePostProcessComponent && InPreviewScene)
    {
        OutlinePostProcessComponent->bUnbound = true;
        OutlinePostProcessComponent->bEnabled = true;
        OutlinePostProcessComponent->BlendWeight = 1.0f;
        OutlinePostProcessComponent->Priority = 1.0f;

        UMaterialInstance* OutlineMaterial = LoadObject<UMaterialInstance>(nullptr, TEXT("/Game/TechLab/PostProcess/PPI_OutlineShader.PPI_OutlineShader"));

        if (OutlineMaterial)
        {
            OutlinePostProcessComponent->Settings.AddBlendable(OutlineMaterial, 1.0f);
        }

        InPreviewScene->AddComponent(OutlinePostProcessComponent, FTransform::Identity);
    }
    
    /*if (EditedFormation)
    {
        EditedFormation->OnAgentPositionsChanged.AddRaw(
            this, &FFormationEditorViewportClient::RefreshPreviewActors
        );
    }*/

    if (EditedFormation)
    {
        EditedFormation->OnAgentCountChanged.AddRaw(this, &FFormationEditorViewportClient::HandleAgentCountChanged);
        EditedFormation->OnAgentsDataChanged.AddRaw(this, &FFormationEditorViewportClient::HandleAgentsDataChanged);
        EditedFormation->OnGroupPresetsChanged.AddRaw(this, &FFormationEditorViewportClient::HandleGroupPresetsChanged);
    }

    bShowWidget = true; 

    PostUndoRedoHandle = FEditorDelegates::PostUndoRedo.AddLambda([this]()
    {
        OnPostUndoRedo();
    });

    LeaderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!LeaderMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load Sphere mesh for Virtual Leader."));
    }

    LeaderMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorMaterials/WidgetMaterial_Z.WidgetMaterial_Z"));
    if (!LeaderMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load material for Virtual Leader."));
    }
}

FFormationEditorViewportClient::~FFormationEditorViewportClient()
{
    ClearSelection();
    DestroyPreviewActors();

    if (OutlinePostProcessComponent && PreviewScene)
    {
        //PreviewScene->RemoveComponent(OutlinePostProcessComponent);
        //OutlinePostProcessComponent = nullptr;
    }

    if (EditedFormation)
    {
        EditedFormation->OnAgentCountChanged.RemoveAll(this);
        EditedFormation->OnAgentsDataChanged.RemoveAll(this);
        EditedFormation->OnGroupPresetsChanged.RemoveAll(this);
    }

    if (PostUndoRedoHandle.IsValid())
    {
        FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoHandle);
    }
}

void FFormationEditorViewportClient::OnPostUndoRedo()
{
    ForceDestroyPreviewActors();

    RefreshPreviewActors();
    Invalidate();
}

void FFormationEditorViewportClient::ForceDestroyPreviewActors()
{
    if (!PreviewScene) return;

    UWorld* World = PreviewScene->GetWorld();
    if (!World) return;

    for (TWeakObjectPtr<AActor> ActorPtr : PreviewActors)
    {
        if (ActorPtr.IsValid())
        {
            AActor* Actor = ActorPtr.Get();
            if (Actor && IsValid(Actor))
            {
                TArray<TObjectPtr<USceneComponent>> Components = Actor->GetRootComponent()->GetAttachChildren();
                for (UActorComponent* Component : Components)
                {
                    if (Component)
                    {
                        Component->DestroyComponent();
                    }
                }
                
                //Actor->MarkPendingKill();
                Actor->Destroy();


                World->RemoveActor(Actor, true);
            }
        }
    }

    PreviewActors.Empty();
    SelectedActors.Empty();
    SelectedIndices.Empty();
    
    if (PreviewScene)
    {
        FlushRenderingCommands();
    }

    Invalidate();

    UE_LOG(LogTemp, Warning, TEXT("Force destroyed all preview actors and cleared render resources"));
}

void FFormationEditorViewportClient::SetEditorToolkit(TSharedPtr<FFormationEditorToolkit> InToolkit)
{
    EditorToolkit = InToolkit;
}

void FFormationEditorViewportClient::DrawVirtualLeader(FPrimitiveDrawInterface* PDI)
{
    if (!PreviewScene || !EditedFormation) return;

    DrawCircle(PDI, FVector(0,0,0), FVector(1, 0, 0), FVector(0, 1, 0), FColor::Blue, 30.f, 32, SDPG_World, 3);
}

void FFormationEditorViewportClient::DrawForwardArrow(FPrimitiveDrawInterface* PDI)
{
    if (!PreviewScene) return ;
    
    const FVector Start = FVector(30, 0, 0);
    const FVector End = FVector(200, 0, 0);
    const float ArrowSize = 60.0f;
    const FLinearColor ArrowColor = FColor::Blue.ReinterpretAsLinear();
    const float Thickness = 3.0f;
    const uint8 DepthPriority = SDPG_Foreground;

    PDI->DrawLine(Start, End, ArrowColor, DepthPriority, Thickness);

    FVector Dir = (End - Start).GetSafeNormal();
    FVector Up(0, 0, 1);
    FVector Right = FVector::CrossProduct(Dir, Up);
    if (Right.IsNearlyZero()) Right = FVector::CrossProduct(Dir, FVector(0,1,0));
    Right.Normalize();
    Up = FVector::CrossProduct(Right, Dir);
        
    PDI->DrawLine(End, End - Dir * ArrowSize + Right * ArrowSize * 0.5f, ArrowColor, DepthPriority, Thickness);
    PDI->DrawLine(End, End - Dir * ArrowSize - Right * ArrowSize * 0.5f, ArrowColor, DepthPriority, Thickness);
}

void FFormationEditorViewportClient::DrawPriorityNumbers(FViewport* InViewport, FCanvas* Canvas)
{
    if (!EditedFormation || !PreviewScene) return;
    
    UWorld* World = PreviewScene->GetWorld();
    if (!World || !Canvas) return;

    FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
        Canvas->GetRenderTarget(), GetScene(), FEngineShowFlags(ESFIM_Game))
        .SetRealtimeUpdate(true));
        
    FSceneView* SceneView = CalcSceneView(&ViewFamily);
    
    if (!SceneView) return;

    FVector CameraLocation = SceneView->ViewLocation;
    FVector CameraForward = SceneView->GetViewDirection();

    for (const FAgentData& Agent : EditedFormation->AgentDatas)
    {
        const FVector WorldPos = Agent.Position;
        const int32 Priority = Agent.Priority;
               
        FVector ToUnit = (WorldPos - CameraLocation).GetSafeNormal();
        float Dot = FVector::DotProduct(CameraForward, ToUnit);

        if (Dot > 0.f)
        {
            FVector2D ScreenPos;

            if (SceneView->WorldToPixel(WorldPos, ScreenPos))
            {
                FString Text = FString::Printf(TEXT("%d"), Priority);
                FCanvasTextItem TextItem(
                    ScreenPos,
                    FText::FromString(Text),
                    GEngine->GetMediumFont(),
                    FLinearColor::Yellow       
                );

                TextItem.Scale = FVector2D(2.0f, 2.0f);
                TextItem.bCentreX = true;
                TextItem.bCentreY = true;

                TextItem.EnableShadow(FLinearColor::Black);
          
                Canvas->DrawItem(TextItem);
            }
        }
    }
}

bool FFormationEditorViewportClient::CheckGroupChanged()
{
    static TArray<FGroupUnitPreset> LastGroupPresets;

    bool IsDirty = false;
    
    if (EditedFormation->GroupUnitPresets.Num() != LastGroupPresets.Num())
    {
        IsDirty = true;
    }
    else
    {
        for (int32 i = 0; i < EditedFormation->GroupUnitPresets.Num(); ++i)
        {
            const FGroupUnitPreset& CurrentPreset = EditedFormation->GroupUnitPresets[i];
            const FGroupUnitPreset& LastPreset = LastGroupPresets[i];
            
            if (CurrentPreset.GroupName != LastPreset.GroupName || CurrentPreset.UnitPreset != LastPreset.UnitPreset)
            {
                IsDirty = true;
                break;
            }
        }
    }
    
    if (IsDirty)
    {
        LastGroupPresets = EditedFormation->GroupUnitPresets;
    }

    return IsDirty;
}

void FFormationEditorViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
    FEditorViewportClient::Draw(InViewport, Canvas);

    if (bBoxSelecting)
    {
        const FVector2D StartPos(FMath::Min(BoxSelectStart.X, BoxSelectEnd.X), FMath::Min(BoxSelectStart.Y, BoxSelectEnd.Y));
        const FVector2D Size(FMath::Abs(BoxSelectEnd.X - BoxSelectStart.X), FMath::Abs(BoxSelectEnd.Y - BoxSelectStart.Y));

        const FLinearColor FillColor(0.2f, 0.2f, 0.5f, 0.2f); 
        FCanvasTileItem FillItem(StartPos, Size, FillColor);
        FillItem.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(FillItem);

        const FLinearColor BorderColor(1.0f, 1.0f, 1.0f, 1.0f);
        FCanvasBoxItem BorderItem(StartPos, Size);
        BorderItem.SetColor(BorderColor);
        BorderItem.LineThickness = 1.0f; 
        Canvas->DrawItem(BorderItem);
    }

    if (!EditedFormation) return;
    
    if (!bInitialized)
    {
        bInitialized = true;
        SpawnPreviewActors();
    }
    /*static TArray<FAgentData> LastUnitPositions;
    
    if (EditedFormation && EditedFormation->AgentDatas != LastUnitPositions)
    {
        UpdatePreviewActors();
        LastUnitPositions = EditedFormation->AgentDatas;
    }

    if (EditedFormation &&  CheckGroupChanged())
    {
        ClearSelection();
        SpawnPreviewActors();
    }*/

    if (EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::PriorityNumbers)) {
        DrawPriorityNumbers(InViewport, Canvas);
    }
}

void FFormationEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    FEditorViewportClient::Draw(View, PDI);

    if (!EditedFormation) return;

    if (EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::ForwardArrow))
    {
        DrawForwardArrow(PDI);
    }

    if (EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::DebugSpheres))
    {
        DrawDebugSpheres(PDI);
    }

    if (EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::VirtualLeader))
    {
        DrawVirtualLeader(PDI);
    }

    if (EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::FormationRadius))
    {
        DrawFormationRadius(PDI);
    }
}

void FFormationEditorViewportClient::UpdateMouseDelta()
{
    if (bBoxSelecting)
    {
        BoxSelectEnd = FIntPoint(Viewport->GetMouseX(), Viewport->GetMouseY());
        Invalidate();
    }

    FEditorViewportClient::UpdateMouseDelta();
}



UE::Widget::EWidgetMode FFormationEditorViewportClient::GetWidgetMode() const
{
    return SelectedActors.Num() > 0 ? CurrentWidgetMode : UE::Widget::WM_None;
}

FVector FFormationEditorViewportClient::GetWidgetLocation() const
{
    if (SelectedActors.Num() > 0)
    {
        const TWeakObjectPtr<AActor>& LastSelectedActorPtr = SelectedActors.Last();

        if (LastSelectedActorPtr.IsValid())
        {
            return LastSelectedActorPtr->GetActorLocation();
        }
    }

    return FVector::ZeroVector;
}

void FFormationEditorViewportClient::SetWidgetMode(UE::Widget::EWidgetMode NewMode)
{
    CurrentWidgetMode = NewMode;
}

bool FFormationEditorViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
    if (InViewport && InViewport->KeyState(EKeys::RightMouseButton))
    {
        return FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);
    }

    if (bIsDragging && SelectedActors.Num() > 0 && EditedFormation)
    {
        if (GetViewportType() == LVT_Perspective)
        {
            if (GetWidgetMode() == UE::Widget::WM_Translate)
            {
                const bool bShouldSnap = bEnableSnapping || IsSnappingKeyPressed();
                FVector SnappedDrag = Drag;

                if (bShouldSnap)
                {
                    SnappedDrag.X = FMath::GridSnap(SnappedDrag.X, SnapValue);
                    SnappedDrag.Y = FMath::GridSnap(SnappedDrag.Y, SnapValue);
                    SnappedDrag.Z = FMath::GridSnap(SnappedDrag.Z, SnapValue);
                }

                for (int32 i = 0; i < SelectedActors.Num(); ++i)
                {
                    if (SelectedActors[i].IsValid() && SelectedIndices.IsValidIndex(i))
                    {
                        const int32 DataIndex = SelectedIndices[i];

                        if (EditedFormation->AgentDatas.IsValidIndex(DataIndex))
                        {
                            const FVector NewLocation = SelectedActors[i]->GetActorLocation() + SnappedDrag;

                            SelectedActors[i]->SetActorLocation(NewLocation);
                            EditedFormation->AgentDatas[DataIndex].Position = NewLocation;
                        }
                    }
                }
            }
            else if (GetWidgetMode() == UE::Widget::WM_Rotate)
            {
                if (!Rot.IsNearlyZero()) 
                {
                    const FVector PivotPoint = GetWidgetLocation();   
                    const FQuat DeltaRotation = Rot.Quaternion();   

                    for (int32 i = 0; i < SelectedActors.Num(); ++i)
                    {
                        if (SelectedActors[i].IsValid() && SelectedIndices.IsValidIndex(i))
                        {
                            AActor* Actor = SelectedActors[i].Get();
                            const int32 DataIndex = SelectedIndices[i];

                            if (EditedFormation->AgentDatas.IsValidIndex(DataIndex))
                            {
                                const FQuat OriginalQuat = Actor->GetActorQuat();
                                const FQuat NewQuat = DeltaRotation * OriginalQuat;

                                const FVector OldLocation = Actor->GetActorLocation();
                                const FVector VectorFromPivot = OldLocation - PivotPoint;
                                const FVector RotatedVector = DeltaRotation.RotateVector(VectorFromPivot);
                                const FVector NewLocation = PivotPoint + RotatedVector;

                                Actor->SetActorLocationAndRotation(NewLocation, NewQuat);
                                EditedFormation->AgentDatas[DataIndex].Position = NewLocation;
                                EditedFormation->AgentDatas[DataIndex].Rotation = NewQuat.Rotator();
                            }
                        }
                    }
                }
            }
        }
        else
        {
            const bool bShouldSnap = bEnableSnapping || IsSnappingKeyPressed();

            AccumulatedDrag += Drag;

            FVector SnappedDrag = AccumulatedDrag;

            if (bShouldSnap)
            {
                SnappedDrag.X = FMath::GridSnap(SnappedDrag.X, SnapValue);
                SnappedDrag.Y = FMath::GridSnap(SnappedDrag.Y, SnapValue);
                SnappedDrag.Z = FMath::GridSnap(SnappedDrag.Z, SnapValue);
            }

            FVector AppliedDrag = SnappedDrag;
            AccumulatedDrag -= AppliedDrag;
          
            for (int32 i = 0; i < SelectedActors.Num(); ++i)
            {
                if (SelectedActors[i].IsValid() && SelectedIndices.IsValidIndex(i))
                {
                    const int32 DataIndex = SelectedIndices[i];
                    if (EditedFormation->AgentDatas.IsValidIndex(DataIndex))
                    {
                        const FVector NewLocation = SelectedActors[i]->GetActorLocation() + AppliedDrag;
                        SelectedActors[i]->SetActorLocation(NewLocation);
                        EditedFormation->AgentDatas[DataIndex].Position = NewLocation;
                    }
                }
            }
        }


        if (auto Toolkit = EditorToolkit.Pin())
        {
            Toolkit->RefreshAgentDetailsView();
        }

        return true;
    }

    return FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);
}

bool FFormationEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    if (EventArgs.Key == EKeys::Delete && EventArgs.Event == IE_Pressed)
    {
        if (SelectedIndices.Num() > 0 && EditedFormation)
        {
            const FScopedTransaction Transaction(
                NSLOCTEXT("FormationEditor", "DeleteAgents", "Delete Formation Agents")
            );

            EditedFormation->Modify();

            if (PreviewScene && PreviewScene->GetWorld())
            {
                PreviewScene->GetWorld()->Modify();
            }

            for (const int32 Index : SelectedIndices)
            {
                if (PreviewActors.IsValidIndex(Index) && PreviewActors[Index].IsValid())
                {
                    PreviewActors[Index]->Modify();
                }
            }

            SelectedIndices.Sort([](const int32& A, const int32& B) { return A > B; });
            for (const int32 IndexToRemove : SelectedIndices)
            {
                if (EditedFormation->AgentDatas.IsValidIndex(IndexToRemove))
                {
                    EditedFormation->AgentDatas.RemoveAt(IndexToRemove);
                }
            }

            EditedFormation->OnAgentCountChanged.Broadcast();
            return true;
        }
    }

    const bool bIsCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);

    if (bIsCtrlDown)
    {
        // Ctrl + C
        if (EventArgs.Key == EKeys::C && EventArgs.Event == IE_Pressed)
        {
            if (SelectedIndices.Num() > 0 && EditedFormation)
            {
                CopiedAgentDataClipboard.Empty();
                for (const int32 Index : SelectedIndices)
                {
                    if (EditedFormation->AgentDatas.IsValidIndex(Index))
                    {
                        CopiedAgentDataClipboard.Add(EditedFormation->AgentDatas[Index]);
                    }
                }

                return true;
            }
        }

        // Ctrl + V
        if (EventArgs.Key == EKeys::V && EventArgs.Event == IE_Pressed)
        {
            if (CopiedAgentDataClipboard.Num() > 0 && EditedFormation)
            {
                // record transaction for Undo/Redo
                const FScopedTransaction Transaction(NSLOCTEXT("FormationEditor", "PasteAgents", "Paste Formation Agents"));
                EditedFormation->Modify();

                TArray<int32> NewPastedIndices;

                for (const FAgentData& CopiedData : CopiedAgentDataClipboard)
                {
                    FAgentData NewAgentData = CopiedData;
                    
                    const int32 NewIndex = EditedFormation->AgentDatas.Add(NewAgentData);
                    NewPastedIndices.Add(NewIndex);
                }

                EditedFormation->OnAgentCountChanged.Broadcast();

                ClearSelection();

                for (const int32 Index : NewPastedIndices)
                {
                    if (PreviewActors.IsValidIndex(Index) && PreviewActors[Index].IsValid())
                    {
                        SelectActor(PreviewActors[Index].Get(), true);
                    }
                }

                //EditedFormation->PostEditChange();
                return true;
            }
        }
        
        // Ctrl + D
        if (EventArgs.Key == EKeys::D && EventArgs.Event == IE_Pressed)
        {
            if (SelectedIndices.Num() > 0 && EditedFormation)
            {
                const FScopedTransaction Transaction(NSLOCTEXT("FormationEditor", "DuplicateAgentsWithOffset", "Duplicate Formation Agents with Offset"));
                EditedFormation->Modify();

                TArray<int32> OriginalSelectedIndices = SelectedIndices;
                TArray<int32> NewDuplicatedIndices;

                for (const int32 Index : OriginalSelectedIndices)
                {
                    if (EditedFormation->AgentDatas.IsValidIndex(Index))
                    {
                        FAgentData NewAgentData = EditedFormation->AgentDatas[Index];

                        NewAgentData.Position.X += 10.0f;
                        NewAgentData.Position.Y += 10.0f;
                        
                        const int32 NewIndex = EditedFormation->AgentDatas.Add(NewAgentData);
                        NewDuplicatedIndices.Add(NewIndex);
                    }
                }

                EditedFormation->OnAgentCountChanged.Broadcast();

                for (const int32 NewIndex : NewDuplicatedIndices)
                {
                    if (PreviewActors.IsValidIndex(NewIndex) && PreviewActors[NewIndex].IsValid())
                    {
                        SelectActor(PreviewActors[NewIndex].Get(), true);
                    }
                }

                Invalidate();

                return true; 
            }
        }
    }

    if (EventArgs.Key == EKeys::LeftMouseButton)
    {
        if (!Viewport)
        {
            return FEditorViewportClient::InputKey(EventArgs);
        }

        if (EventArgs.Event == IE_Pressed)
        {
			
			const bool bIsAltDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);

            if (bIsCtrlDown && bIsAltDown)
            {
				HHitProxy* HitProxy = Viewport->GetHitProxy(Viewport->GetMouseX(), Viewport->GetMouseY());
                if (HitProxy && HitProxy->IsA(HWidgetAxis::StaticGetType()))
                {

                }
                else
                {
                    bBoxSelecting = true;
                    BoxSelectStart = FIntPoint(Viewport->GetMouseX(), Viewport->GetMouseY());
                    BoxSelectEnd = BoxSelectStart;
                    return true;
                }
            }
        }
        else if (EventArgs.Event == IE_Released)
        {
            if (bBoxSelecting)
            {
                bBoxSelecting = false;
                ClearSelection(false);

                FBox2D SelectionBox(FVector2D(FMath::Min(BoxSelectStart.X, BoxSelectEnd.X), FMath::Min(BoxSelectStart.Y, BoxSelectEnd.Y)),
                                    FVector2D(FMath::Max(BoxSelectStart.X, BoxSelectEnd.X), FMath::Max(BoxSelectStart.Y, BoxSelectEnd.Y)));

                FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags));
                FSceneView* View = CalcSceneView(&ViewFamily);

                if (!View)
                {
                    Invalidate();
                    return true;
                }

                for (int32 i = 0; i < PreviewActors.Num(); ++i)
                {
                    if (PreviewActors[i].IsValid())
                    {
                        FVector2D ScreenPos;
                        if (View->WorldToPixel(PreviewActors[i]->GetActorLocation(), ScreenPos))
                        {
                            if (SelectionBox.IsInside(ScreenPos))
                            {
                                SelectedActors.Add(PreviewActors[i]);
                                SelectedIndices.Add(i);
                                SetHighlight(PreviewActors[i].Get(), true);
                            }
                        }
                    }
                }

                NotifySelectionChanged();
                
                Invalidate();
                return true;
            }
        }
    }

    return FEditorViewportClient::InputKey(EventArgs);
}

void FFormationEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState,
    bool bIsDraggingWidget, bool bNudge)
{
    AccumulatedDrag = FVector::ZeroVector;
    bIsDuplicating = false;

    if (bIsDraggingWidget && SelectedActors.Num() > 0 && EditedFormation)
    {
        bIsDragging = true;
        
        const bool bIsAltDown = InInputState.IsAltButtonPressed();
        
        if (bIsAltDown)
        {
            bIsDuplicating = true;
            
            GEditor->BeginTransaction(NSLOCTEXT("FormationEditor", "DuplicateAgents", "Duplicate Formation Agents"));
            EditedFormation->Modify();

            TArray<int32> OriginalSelectedIndices = SelectedIndices;
            TArray<int32> NewPastedIndices;
            
            for (const int32 Index : OriginalSelectedIndices)
            {
                if (EditedFormation->AgentDatas.IsValidIndex(Index))
                {
                    FAgentData NewAgentData = EditedFormation->AgentDatas[Index];
                    const int32 NewIndex = EditedFormation->AgentDatas.Add(NewAgentData);
                    NewPastedIndices.Add(NewIndex);
                }
            }

            EditedFormation->OnAgentCountChanged.Broadcast();
            
            for (const int32 NewIndex : NewPastedIndices)
            {
                if (PreviewActors.IsValidIndex(NewIndex) && PreviewActors[NewIndex].IsValid())
                {
                    SelectActor(PreviewActors[NewIndex].Get(), true);
                }
            }
            
        }
        else
        {
            GEditor->BeginTransaction(NSLOCTEXT("FormationEditor", "TransformAgents", "Transform Formation Agents"));
            EditedFormation->Modify();
        }
    }
    
    FEditorViewportClient::TrackingStarted(InInputState, bIsDraggingWidget, bNudge);
}

void FFormationEditorViewportClient::TrackingStopped()
{
    if (bIsDragging)
    {
        //GEditor->EndTransaction();
        //EditedFormation->PostEditChange();

        if (EditedFormation && SelectedIndices.Num() > 0)
        {
			EditedFormation->OnAgentsDataChanged.Broadcast(SelectedIndices);
        }

        GEditor->EndTransaction();
    }
    
    bIsDragging = false;
    bIsDuplicating = false;

    FEditorViewportClient::TrackingStopped();
}

void FFormationEditorViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
    if (HitProxy && HitProxy->IsA(HWidgetAxis::StaticGetType()))
    {
        FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
        return;
    }

    if (Key == EKeys::LeftMouseButton && Event == IE_Released)
    {
        const bool bIsCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);

        if (HitProxy && HitProxy->IsA(HActor::StaticGetType()))
        {
            HActor* ActorProxy = static_cast<HActor*>(HitProxy);
            AActor* ClickedActor = ActorProxy->Actor;

            bool bIsPreviewActor = false;
            for (const auto& PreviewActor : PreviewActors)
            {
                if (PreviewActor.Get() == ClickedActor)
                {
                    bIsPreviewActor = true;
                    break;
                }
            }

            if (bIsPreviewActor)
            {
                SelectActor(ClickedActor, bIsCtrlDown);
                return;
            }
        }
        else
        {
            if (!bIsCtrlDown)
            {
                ClearSelection();
            }
        }
    }

    FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

TArray<int32> FFormationEditorViewportClient::FFormationEditorViewportClient::RefreshPreviewActors()
{
    TArray<int32> OldSelectedIndices = SelectedIndices;

    ClearSelection(false);

    SpawnPreviewActors();

    for (const int32 OldIndex : OldSelectedIndices)
    {
        if (PreviewActors.IsValidIndex(OldIndex) && PreviewActors[OldIndex].IsValid())
        {
            SelectedActors.Add(PreviewActors[OldIndex]);
            SelectedIndices.Add(OldIndex);
            SetHighlight(PreviewActors[OldIndex].Get(), true);
        }
    }


    Invalidate();

    return OldSelectedIndices;
}

void FFormationEditorViewportClient::SelectActor(AActor* NewActor, bool bToggle)
{
    if (!NewActor)
    {
        return;
    }

    const bool bIsAlreadySelected = IsActorSelected(NewActor);

    if (bToggle) // Ctrl +
    {
        if (bIsAlreadySelected)
        {
            DeselectActor(NewActor);
        }
        else
        {
            // Add to SelectedActors
            SelectedActors.Add(NewActor);
            for (int32 i = 0; i < PreviewActors.Num(); ++i)
            {
                if (PreviewActors[i].Get() == NewActor)
                {
                    SelectedIndices.Add(i);
                    break;
                }
            }
            SetHighlight(NewActor, true);
        }
    }
    else 
    {
        if (!bIsAlreadySelected || SelectedActors.Num() > 1)
        {
            ClearSelection();
            SelectedActors.Add(NewActor);
            for (int32 i = 0; i < PreviewActors.Num(); ++i)
            {
                if (PreviewActors[i].Get() == NewActor)
                {
                    SelectedIndices.Add(i);
                    break;
                }
            }
			SetHighlight(NewActor, true);
        } 
    }

    NotifySelectionChanged();
    Invalidate();
}

void FFormationEditorViewportClient::DeselectActor(AActor* ActorToDeselect)
{
    if (!ActorToDeselect)
    {
        return;
    }

    const int32 ActorIndexInSelection = SelectedActors.IndexOfByKey(ActorToDeselect);
    if (ActorIndexInSelection != INDEX_NONE)
    {
        SelectedActors.RemoveAt(ActorIndexInSelection);
        for (int32 i = 0; i < PreviewActors.Num(); ++i)
        {
            if (PreviewActors[i].Get() == ActorToDeselect)
            {
                SelectedIndices.Remove(i);
                break;
            }
        }
		SetHighlight(ActorToDeselect, false);
    }
}

void FFormationEditorViewportClient::ClearSelection(bool bNotify)
{
    for (const auto& ActorPtr : SelectedActors)
    {
        if (ActorPtr.IsValid())
        {
            SetHighlight(ActorPtr.Get(), false);
        }
    }

    SelectedActors.Empty();
    SelectedIndices.Empty();
    if(bNotify)
    {
        NotifySelectionChanged();
    }

    Invalidate();
}

bool FFormationEditorViewportClient::IsActorSelected(AActor* InActor) const
{
    if (!InActor)
    {
        return false;
    }

    return SelectedActors.ContainsByPredicate([InActor](const TWeakObjectPtr<AActor>& Ptr)
    {
            return Ptr.Get() == InActor;
    });
}


void FFormationEditorViewportClient::SelectActorByIndex(int32 Index)
{
    if (PreviewActors.IsValidIndex(Index) && PreviewActors[Index].IsValid())
    {
        ClearSelection();

        SelectActor(PreviewActors[Index].Get(), false);
    }
}
void FFormationEditorViewportClient::SpawnPreviewActors()
{
    if (!EditedFormation || !PreviewScene) return;
    
    UWorld* World = PreviewScene->GetWorld();
    if (!World) return;

    DestroyPreviewActors();

    TSubclassOf<AActor> LeaderSpawnClass = AStaticMeshActor::StaticClass(); 
    AActor* NewLeaderActor = World->SpawnActor<AActor>(
        LeaderSpawnClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator
    );

    if (NewLeaderActor)
    {
        NewLeaderActor->bIsEditorPreviewActor = true;
        NewLeaderActor->SetFlags(RF_Transient);

        UStaticMeshComponent* MeshComponent = NewLeaderActor->GetComponentByClass<UStaticMeshComponent>();
        if (MeshComponent)
        {
            MeshComponent->SetStaticMesh(LeaderMesh);
            MeshComponent->SetMaterial(0, LeaderMaterial);
            MeshComponent->SetRelativeScale3D(FVector(0.5f));
        }

        VirtualLeaderActor = NewLeaderActor;
    }

    // TSubclassOf<AActor> SpawnClass = AStaticMeshActor::StaticClass(); 
    // if (EditedFormation && EditedFormation->UnitActorPreset)
    // {
    //     // SpawnClass = EditedFormation->UnitActorPreset;
    //     SpawnClass = EditedFormation->GetUnitPresetForGroup(UnitData.GroupName);
    // }
    
    for (const FAgentData& UnitData : EditedFormation->AgentDatas)
    {
        TSubclassOf<AActor> SpawnClass = AStaticMeshActor::StaticClass(); 
        if (EditedFormation && !EditedFormation->GroupUnitPresets.IsEmpty())
        {
            SpawnClass = EditedFormation->GetUnitPresetForGroup(UnitData.GroupName);
        }
        FActorSpawnParameters SpawnParams;
        SpawnParams.bNoFail = true;
        SpawnParams.ObjectFlags = RF_Transient | RF_DuplicateTransient;
        SpawnParams.bDeferConstruction = false;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* NewActor = World->SpawnActor<AActor>(
            SpawnClass,
            UnitData.Position,
            UnitData.Rotation,
            SpawnParams
        );

        if (NewActor)
        {
            NewActor->bIsEditorPreviewActor = true;
            PreviewActors.Add(NewActor);
        }
    }
}

void FFormationEditorViewportClient::UpdatePreviewActors()
{
    if (!EditedFormation || !PreviewScene) return;
    
    UWorld* World = PreviewScene->GetWorld();
    if (!World) return;

    for (int32 i = 0; i < FMath::Min(PreviewActors.Num(), EditedFormation->AgentDatas.Num()); ++i)
    {
        if (PreviewActors[i].IsValid() && PreviewActors[i]->IsValidLowLevel())
        {
            PreviewActors[i]->SetActorLocationAndRotation(
                EditedFormation->AgentDatas[i].Position,
                EditedFormation->AgentDatas[i].Rotation
            );
        }
    }
}

void FFormationEditorViewportClient::DestroyPreviewActors()
{
    if (VirtualLeaderActor.IsValid())
    {
        AActor* LeaderActor = VirtualLeaderActor.Get();
        if (LeaderActor && IsValid(LeaderActor))
        {
            LeaderActor->Destroy();
        }
    }

    for (TWeakObjectPtr<AActor> ActorPtr : PreviewActors)
    {
        if (ActorPtr.IsValid())
        {
            AActor* Actor = ActorPtr.Get();
            if (Actor && IsValid(Actor))
            {
                // Force immediate cleanup for editor preview actors
                //Actor->MarkPendingKill();
                Actor->Destroy();
            }
        }
    }

    PreviewActors.Empty(); // Clear the array completely

    // Force garbage collection for editor actors
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(true);
    }
}

void FFormationEditorViewportClient::DrawDebugSpheres(FPrimitiveDrawInterface* PDI) const
{
    if (!EditedFormation || !PDI)
    {
        return;
    }
    
    const FMaterialRenderProxy* MaterialProxy = GEngine->WireframeMaterial->GetRenderProxy();

    for (const FAgentData& AgentData : EditedFormation->AgentDatas)
    {
        DrawWireSphere(
            PDI,
            AgentData.Position,     
            FLinearColor::Green,    
            30.0f,                  
            16,                     
            SDPG_World              
        );
    }
}

void FFormationEditorViewportClient::DrawFormationRadius(FPrimitiveDrawInterface* PDI)
{
    if (!EditedFormation || !PreviewScene) return;
    
    DrawCircle(PDI, FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FColor::Red, EditedFormation->FormationMinRadius, 64, SDPG_Foreground, 4.0f);
}

bool FFormationEditorViewportClient::IsSnappingKeyPressed() const
{
    return Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
}

void FFormationEditorViewportClient::SwitchToOrthographicView(const FVector& NewDirection, ELevelViewportType NewViewportType)
{

    float TargetOrthoZoom = (GetViewportType() == LVT_Perspective) ? GetViewLocation().Size() : GetOrthoZoom();
    if (TargetOrthoZoom < 1.f) 
    {
        TargetOrthoZoom = 100.f; 
    }

    const FVector NewLocation = NewDirection.GetSafeNormal() * TargetOrthoZoom;

    SetViewportType(NewViewportType);
    SetViewLocation(NewLocation);
    SetViewRotation(LookAtOrigin(NewLocation));
    SetOrthoZoom(TargetOrthoZoom);
}

void FFormationEditorViewportClient::SetHighlight(AActor* Actor, bool bEnable)
{
    if (!Actor)
    {
        return;
    }

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
    
    for (UPrimitiveComponent* Component : PrimitiveComponents)
    {
        if (Component)
        {
            Component->SetRenderCustomDepth(bEnable);
            Component->SetCustomDepthStencilValue(bEnable ? OutlineStencilValue : 0);
        }
    }
}

void FFormationEditorViewportClient::NotifySelectionChanged()
{
    if (auto Toolkit = EditorToolkit.Pin())
    {
        TArray<int32> CurrentSelectedIndices = SelectedIndices;
        Toolkit->UpdateAgentDetailsView(CurrentSelectedIndices);

        if (EditedFormation && EditedFormation->bIsUpdatingFromDataChange)
        {
            EditedFormation->bIsUpdatingFromDataChange = false;

            GEditor->GetTimerManager()->SetTimerForNextTick([Toolkit, CurrentSelectedIndices]()
                {
                    if (Toolkit.IsValid())
                    {
                        Toolkit->UpdateAgentDetailsView(CurrentSelectedIndices);
                    }
                });
        }
    }
}

void FFormationEditorViewportClient::HandleAgentCountChanged()
{
    ClearSelection(false);
    SpawnPreviewActors();
    Invalidate();
}

void FFormationEditorViewportClient::HandleAgentsDataChanged(const TArray<int32>& AgentIndices)
{
    bool bNeedsRebuild = false;
    bool bGroupNameChanged = false;

    for (const int32 Index : AgentIndices)
    {
        if (!EditedFormation || !EditedFormation->AgentDatas.IsValidIndex(Index) || !PreviewActors.IsValidIndex(Index) || !PreviewActors[Index].IsValid())
        {
            bNeedsRebuild = true;
            break;
        }

        const FAgentData& NewData = EditedFormation->AgentDatas[Index];
        AActor* CurrentActor = PreviewActors[Index].Get();

        TSubclassOf<APawn> TargetPawnClass = EditedFormation->GetUnitPresetForGroup(NewData.GroupName);
        TSubclassOf<AActor> TargetSpawnClass;
        if (TargetPawnClass)
        {
            TargetSpawnClass = TargetPawnClass;
        }
        else
        {
            TargetSpawnClass = AStaticMeshActor::StaticClass();
        }

        if (CurrentActor->GetClass() != TargetSpawnClass)
        {
            bNeedsRebuild = true;
            bGroupNameChanged = true;
            break;
        }
        else
        {
            CurrentActor->SetActorLocationAndRotation(NewData.Position, NewData.Rotation);
        }
    }

    if (bNeedsRebuild)
    {
        TArray<int32> RestoredIndices = RefreshPreviewActors();

        if (auto Toolkit = EditorToolkit.Pin())
        {
            Toolkit->UpdateAgentDetailsView(RestoredIndices);
        }
    }
    else
    {
        Invalidate();
    }
}

void FFormationEditorViewportClient::HandleGroupPresetsChanged()
{
    RefreshPreviewActors();
}

void FFormationEditorViewportClient::SetTopView()
{
    SwitchToOrthographicView(FVector(0.f, 0.f, 1.f), LVT_OrthoXY);
}

void FFormationEditorViewportClient::SetBottomView()
{
    SwitchToOrthographicView(FVector(0.f, 0.f, -1.f), LVT_OrthoNegativeXY);
}

void FFormationEditorViewportClient::SetLeftView()
{
    SwitchToOrthographicView(FVector(0.f, -1.f, 0.f), LVT_OrthoNegativeXZ);
}

void FFormationEditorViewportClient::SetRightView()
{
    SwitchToOrthographicView(FVector(0.f, 1.f, 0.f), LVT_OrthoXZ);
}

void FFormationEditorViewportClient::SetFrontView()
{
    SwitchToOrthographicView(FVector(1.f, 0.f, 0.f), LVT_OrthoNegativeYZ);
}

void FFormationEditorViewportClient::SetBackView()
{
    SwitchToOrthographicView(FVector(-1.f, 0.f, 0.f), LVT_OrthoYZ);
}

void FFormationEditorViewportClient::SetPerspectiveView()
{
    float Distance = (GetViewportType() == LVT_Perspective) ? GetViewLocation().Size() : GetOrthoZoom();
    FVector NewDirection = FVector(-1.f, -1.f, 1.f);
    NewDirection.Normalize();
    const FVector NewLocation = NewDirection * (Distance * 0.1f);

    SetViewportType(LVT_Perspective);
    SetViewLocation(NewLocation);
    SetViewRotation(LookAtOrigin(NewLocation));
}

void FFormationEditorViewportClient::SetAFSShowFlags(EFormationShowFlags NewMode)
{
    CurrentShowFlags = NewMode;
   
    DrawHelper.bDrawGrid = EnumHasAnyFlags(CurrentShowFlags, EFormationShowFlags::Grid);

    Invalidate();
}

