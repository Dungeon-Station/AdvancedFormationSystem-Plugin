/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "Formation.h"
#include "FormationAgentComponent.h"
#include "FormationAsset.h"
#include "FormationHungarian.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "FormationPathModifier.h"
#include "Components/SphereComponent.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FormationWelzl.h"
#include "UObject/Package.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"

AFormation::AFormation()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Formation Center");
	MoveComponent = CreateDefaultSubobject<UFloatingPawnMovement>("MoveComponent");
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Formation Collsion");
	ExtendSphereComponent = CreateDefaultSubobject<USphereComponent>("Extend Formation Collision");
	
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AFormation::OnSphereBeginOverlap);

	ExtendSphereComponent->SetupAttachment(SphereComponent);
	ExtendSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ExtendSphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	ExtendSphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	ExtendSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	ExtendSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	
	MoveComponent->UpdatedComponent = RootComponent;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	FormationTurnThreshold = 0.0f;
	FormationTurnSpeed = 1.0f;
	FormationSpeed = 600.0f;
	AgentSpeed = 900.0f;
	AgentAcceleration = AgentSpeed * 2;
	StrayAgentSpeed = AgentSpeed * 2;
}

void AFormation::BeginPlay()
{
	Super::BeginPlay();

	OnFormationMoveCompleted.AddDynamic(this, &AFormation::MoveCompleted);

	if (RefFormationAsset)
	{
		SphereComponent->SetSphereRadius(RefFormationAsset->FormationRadius);
		FormationAsset = DuplicateObject<UFormationAsset>(RefFormationAsset, GetTransientPackage());
		RefFormationAsset->ConvertToFormationAgentDatas();
		FormationAsset->ConvertToFormationAgentDatas();
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("RefFormationAsset is null in %s"), *GetName());
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (APawn* Pawn = AICon->GetPawn())
		{
			if (auto* FloatMove = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent()))
			{
				FloatMove->MaxSpeed = FormationSpeed;
				FloatMove->Acceleration = FormationSpeed * 2.0f;
			}
		}
	}
	AdjustToGround();
}

void AFormation::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_FormationTick);

	Super::Tick(DeltaTime);

	if (RefFormationAsset == nullptr || FormationAsset == nullptr)
	{
		return;
	}
	//AdjustToGround();

	if (FormationAgentComponents.Num() > 0 && !bRegistered)
	{
		//RearrangeFormationNoMove();
		bRegistered = true;
	}

	if (SphereComponent)
	{
		FormationCenter = SphereComponent->GetComponentLocation();
	}

	ExtendSphereComponent->SetSphereRadius(SphereComponent->GetUnscaledSphereRadius() * 1.1f);

	{
		SCOPE_CYCLE_COUNTER(STAT_RearrangeIfAgentsChanged);
		RearrangeIfAgentsChanged();
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_MoveFormationStart);
		MoveFormationStart();
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsBehaviorByState);
		UpdateAgentsBehaviorByState();
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_MoveFormationAlongPath);
		MoveFormationAlongPath(DeltaTime);
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_AdjustLocationToSolveCrash);
		AdjustLocationToSolveCrash();
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_DownsizeFormation);
		DownsizeFormation();
	}
	
	{
		SCOPE_CYCLE_COUNTER(STAT_DrawDebugPath);
		DrawDebugData();
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateRotation);
		if (!PathPoints.IsEmpty() && CurrentPathIndex < PathPoints.Num() - 1)
		{
			FVector temp = PathPoints[CurrentPathIndex + 1] - PathPoints[CurrentPathIndex];
			FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), temp.ToOrientationRotator(), DeltaTime, FormationTurnSpeed);
			SetActorRotation(NewRotation);
		}
	}
}

void AFormation::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp) return;
	
	FVector MyCenter = SphereComponent->GetComponentLocation();
	FVector ClosestPointOnOther = FVector::ZeroVector;
	
	OtherComp->GetClosestPointOnCollision(MyCenter, ClosestPointOnOther);

	float DistanceToClosestPoint = FVector::Dist(MyCenter, ClosestPointOnOther);

	float MyRadius = SphereComponent->GetScaledSphereRadius();
	OverlapDepth = FMath::Max( 0.0f, MyRadius - DistanceToClosestPoint);
	
	FVector Dir = (ClosestPointOnOther - MyCenter).GetSafeNormal();
	FVector ImpactPoint = MyCenter + Dir * MyRadius;

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), ImpactPoint, 20.0f, 12, FColor::Red, false, 2.0f);
	}
	
	FVector CloseDir = (ImpactPoint - MyCenter).GetSafeNormal();
	float CosCurDir = GetActorForwardVector().Dot(CloseDir);
	float CurAngle = acos(CosCurDir);

	if (GetActorForwardVector().Cross(CloseDir).Z < 0.0f)
	{
		CurAngle *= -1.0f;
	}
	
	float CurDegree = FMath::RadiansToDegrees(CurAngle);
	if (CurDegree * PrevCrashAngle < 0.0f)
	{
		bNeedDownsize = true;
		PrevCrashAngle = CurDegree;
		return;
	}

	PrevCrashAngle = CurDegree;
	bCrashed = true;
	SolveDir = CloseDir;
	SolveDir.Z = 0.0f;
	float CorrectIntencity = FMath::Max( OverlapDepth * 1.5, CorrectPathIntensity);
	for (int i = CurrentPathIndex; i< PathPoints.Num() && i< CurrentPathIndex + CorrectPathNum; i++)
	{
		PathPoints[i] -= (SolveDir * CorrectIntencity);
	}
}

void AFormation::FormationMoveTo(const FVector& Location, const FRotator& Rotation)
{
	PrevCrashAngle = 0.0f;
	TargetLocation = Location;
	TargetRotation = Rotation;
	bIsFormationMoveStart = true;

	const FVector CurrentLocation = GetActorLocation();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(), CurrentLocation, TargetLocation);
	if (!NavPath || NavPath->PathPoints.Num() == 0)
	{
		OnFormationMoveCompleted.Broadcast(false);
		Phase = EFormationPhase::Idle;
		return;
	}

	CurrentPathIndex = 0;
	RawPathPoints = NavPath->PathPoints;
	PathPoints = UFormationPathModifier::ApplyPathCorrection(GetWorld(), NavPath->PathPoints, RefFormationAsset, ModifierConfig);
}

void AFormation::FormationMoveAlongSpline(USplineComponent* InSpline, float StepDistance)
{
	// Invalid Spline case
	if (!InSpline|| InSpline->GetNumberOfSplinePoints() < 2)
	{
		OnFormationMoveCompleted.Broadcast(false);
		Phase = EFormationPhase::Idle;
		return;
	}

	PathPoints.Empty();

	// Check if spline is Linear type for optimization
	bool bIsLinearSpline = true;
	const int32 NumSplinePoints = InSpline->GetNumberOfSplinePoints();

	// Check if all spline segments are linear
	for (int32 i = 0; i < NumSplinePoints - 1; i++)
	{
		if (InSpline->GetSplinePointType(i) != ESplinePointType::Linear)
		{
			bIsLinearSpline = false;
			break;
		}
	}

	if (bIsLinearSpline)
	{
		// Linear spline: Just add all spline points without subdivision
		for (int32 i = 0; i < NumSplinePoints; i++)
		{
			const FVector Point = InSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			PathPoints.Add(Point);
		}

		if (InSpline->IsClosedLoop())
		{
			const FVector FirstPoint = InSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			PathPoints.Add(FirstPoint);

			TargetLocation = PathPoints.Last();
			TargetRotation = InSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);
		}
		else 
		{
			TargetLocation = PathPoints.Last();
			TargetRotation = InSpline->GetRotationAtSplinePoint(NumSplinePoints - 1, ESplineCoordinateSpace::World);
		}
	}
	else
	{
		// Curved spline: Use original subdivision logic
		const float SplineLength = InSpline->GetSplineLength();

		for (float Distance = 0.f; Distance < SplineLength; Distance += StepDistance)
		{
			const FVector Point = InSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			PathPoints.Add(Point);
		}

		// Ensure the last point is included
		PathPoints.Add(InSpline->GetLocationAtSplinePoint(NumSplinePoints - 1, ESplineCoordinateSpace::World));

		TargetLocation = PathPoints.Last();
		TargetRotation = InSpline->GetRotationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
	}

	PrevCrashAngle = 0.0f;
	bIsFormationMoveStart = true;
	CurrentPathIndex = 0;
}

void AFormation::RegisterAgent(UFormationAgentComponent* AgentComponent)
{
	FormationAgentComponents.Add(AgentComponent);
}

void AFormation::RearrangeFormation()
{
	int32 UnitNum = FormationAgentComponents.Num();
	
	if (UnitNum == 0) return;

	TArray<FAgentData> AssetAgentData = FormationAsset->FormationAgentDatas;
	TArray<TArray<FAgentData>> AgentDatasByGroupName;

	AgentDatasByGroupName.SetNum(FormationAsset->GroupUnitPresets.Num());

	for(const FAgentData& AgentData : FormationAsset->FormationAgentDatas)
	{
		int32 GroupIndex = FormationAsset->GetGroupNameOptions().IndexOfByKey(AgentData.GroupName);
		if (GroupIndex > -1)
		{
			AgentDatasByGroupName[GroupIndex].Add(AgentData);
		}
		else
		{
			AgentDatasByGroupName[0].Add(AgentData);
		}
	}
	
	for(TArray<FAgentData>& GroupData : AgentDatasByGroupName)
	{
		if (GroupData.Num() == 0)
		{
			continue;
		}

		GroupData.Sort([](const FAgentData& A, const FAgentData& B) {
			return A.Priority < B.Priority;
		});

		TArray<UFormationAgentComponent*> FormationAgentComponentsInGroup;
		for(UFormationAgentComponent* FormationAgentComponent : FormationAgentComponents)
		{
			if (FormationAgentComponent && FormationAgentComponent->GetGroupName() == GroupData[0].GroupName)
			{
				FormationAgentComponentsInGroup.Add(FormationAgentComponent);
			}
		}

		if(FormationAgentComponentsInGroup.Num() == 0)
		{
			continue; // No agents in this group
		}

		if(FormationAgentComponentsInGroup.Num() > GroupData.Num())
		{
			int32 Difference = FormationAgentComponentsInGroup.Num() - GroupData.Num();
			// Remove excess agents from the group
			for(int32 i = 0; i < Difference; ++i)
			{
				FormationAgentComponentsInGroup.Pop();
			}
		}

		// Create a cost matrix for the Hungarian algorithm
		TArray<TArray<float>> CostMatrix;
		TArray<int32> UnitToSlot;
		TArray<int32> SlotToUnit;
	
		CostMatrix.SetNum(FormationAgentComponentsInGroup.Num());
		for (int32 i = 0; i < FormationAgentComponentsInGroup.Num(); ++i)
		{
			for (int32 j = 0; j < FormationAgentComponentsInGroup.Num(); ++j)
			{
				if(GroupData.Num() <= j)
				{
					break; // No more agents in this group
				}

				ACharacter* Unit = Cast<ACharacter>(FormationAgentComponentsInGroup[i]->GetOwner());
				if (!Unit)
				{
					continue;
				}

				const FVector UnitLocation = Unit->GetActorLocation();
				const FVector SlotLocation = GetActorRotation().RotateVector(GroupData[j].Position) + FormationCenter;

				float Cost = FVector::Dist(UnitLocation, SlotLocation);
				CostMatrix[i].Add(Cost);
			}
		}

		UnitToSlot.Init(-1, FormationAgentComponentsInGroup.Num());
		SlotToUnit.Init(-1, FormationAgentComponentsInGroup.Num());

		// Solve the assignment problem using the Hungarian algorithm
		FFormationHungarian::Solve(CostMatrix, UnitToSlot, SlotToUnit);

		// Move units to their new positions based on the Hungarian algorithm results
		for (int32 i = 0; i < FormationAgentComponentsInGroup.Num(); ++i)
		{
			ACharacter* Unit = Cast<ACharacter>(FormationAgentComponentsInGroup[i]->GetOwner());
			ensure(Unit);

			FAgentData* AgentData = new FAgentData();
			AgentData->Position = GroupData[UnitToSlot[i]].Position;
			AgentData->Rotation = GroupData[UnitToSlot[i]].Rotation;
			AgentData->Priority = GroupData[UnitToSlot[i]].Priority;

			FormationAgentComponentsInGroup[i]->SetAgentData(AgentData);
			FormationAgentComponentsInGroup[i]->SetRefAgentData(AgentData);
			FVector Destination = GetActorRotation().RotateVector(GroupData[UnitToSlot[i]].Position) + FormationCenter;

			AAIController* UnitAIController = Cast<AAIController>(Unit->GetController());
			if (UnitAIController)
			{
				UnitAIController->MoveToLocation(Destination, 10.f);
			}

			// Draw Debug Circle at the destination
			if (bDrawDebug)
			{
				FVector Center = Destination + FVector(0.f, 0.f, 50.f);
				FString Label = FString::Printf(TEXT("%d"), GroupData[UnitToSlot[i]].Priority);

				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (PC && PC->PlayerCameraManager)
				{
					FVector CamRight = PC->PlayerCameraManager->GetActorRightVector();
					FVector CamUp = PC->PlayerCameraManager->GetActorUpVector();

					DrawDebugCircle(GetWorld(), Center, 100.f, 64, FColor::Blue, false, 5.f, 0, 5.f, CamRight, CamUp, false);
				}
				DrawDebugString(GetWorld(), Center, Label, nullptr, FColor::White, 5.f, false, 1.5f);
			}
		}
	}

}

void AFormation::RearrangeFormationNoMove()
{
	int32 UnitNum = FormationAgentComponents.Num();

	if (UnitNum == 0) return;

	TArray<FAgentData> AssetAgentData = FormationAsset->FormationAgentDatas;
	TArray<TArray<FAgentData>> AgentDatasByGroupName;

	AgentDatasByGroupName.SetNum(FormationAsset->GroupUnitPresets.Num());

	for (const FAgentData& AgentData : FormationAsset->FormationAgentDatas)
	{
		int32 GroupIndex = FormationAsset->GetGroupNameOptions().IndexOfByKey(AgentData.GroupName);
		if (GroupIndex > -1)
		{
			AgentDatasByGroupName[GroupIndex].Add(AgentData);
		}
		else
		{
			AgentDatasByGroupName[0].Add(AgentData);
		}
	}

	for (TArray<FAgentData>& GroupData : AgentDatasByGroupName)
	{
		if (GroupData.Num() == 0)
		{
			continue;
		}

		GroupData.Sort([](const FAgentData& A, const FAgentData& B) {
			return A.Priority < B.Priority;
			});

		TArray<UFormationAgentComponent*> FormationAgentComponentsInGroup;
		for (UFormationAgentComponent* FormationAgentComponent : FormationAgentComponents)
		{
			if (FormationAgentComponent && FormationAgentComponent->GetGroupName() == GroupData[0].GroupName)
			{
				FormationAgentComponentsInGroup.Add(FormationAgentComponent);
			}
		}

		if (FormationAgentComponentsInGroup.Num() == 0)
		{
			continue; // No agents in this group
		}

		if (FormationAgentComponentsInGroup.Num() > GroupData.Num())
		{
			int32 Difference = FormationAgentComponentsInGroup.Num() - GroupData.Num();
			// Remove excess agents from the group
			for (int32 i = 0; i < Difference; ++i)
			{
				FormationAgentComponentsInGroup.Pop();
			}
		}

		// Create a cost matrix for the Hungarian algorithm
		TArray<TArray<float>> CostMatrix;
		TArray<int32> UnitToSlot;
		TArray<int32> SlotToUnit;

		CostMatrix.SetNum(FormationAgentComponentsInGroup.Num());
		for (int32 i = 0; i < FormationAgentComponentsInGroup.Num(); ++i)
		{
			for (int32 j = 0; j < FormationAgentComponentsInGroup.Num(); ++j)
			{
				if (GroupData.Num() <= j)
				{
					break; // No more agents in this group
				}

				ACharacter* Unit = Cast<ACharacter>(FormationAgentComponentsInGroup[i]->GetOwner());
				if (!Unit)
				{
					continue;
				}

				const FVector UnitLocation = Unit->GetActorLocation();
				const FVector SlotLocation = GetActorRotation().RotateVector(GroupData[j].Position) + FormationCenter;

				float Cost = FVector::Dist(UnitLocation, SlotLocation);
				CostMatrix[i].Add(Cost);
			}
		}

		UnitToSlot.Init(-1, FormationAgentComponentsInGroup.Num());
		SlotToUnit.Init(-1, FormationAgentComponentsInGroup.Num());

		// Solve the assignment problem using the Hungarian algorithm
		FFormationHungarian::Solve(CostMatrix, UnitToSlot, SlotToUnit);

		// Move units to their new positions based on the Hungarian algorithm results
		for (int32 i = 0; i < GroupData.Num(); ++i)
		{
			if (FormationAgentComponentsInGroup.Num() <= i)
			{
				continue; // No agent assigned to this slot
			}

			ACharacter* Unit = Cast<ACharacter>(FormationAgentComponentsInGroup[i]->GetOwner());
			ensure(Unit);

			FAgentData* AgentData = new FAgentData();
			AgentData->Position = GroupData[UnitToSlot[i]].Position;
			AgentData->Rotation = GroupData[UnitToSlot[i]].Rotation;
			AgentData->Priority = GroupData[UnitToSlot[i]].Priority;

			FormationAgentComponentsInGroup[i]->SetAgentData(AgentData);
			FormationAgentComponentsInGroup[i]->SetRefAgentData(AgentData);
			FVector Destination = GetActorRotation().RotateVector(GroupData[UnitToSlot[i]].Position) + FormationCenter;

			Unit->SetActorLocation(Destination);
			Unit->SetActorRotation(GroupData[UnitToSlot[i]].Rotation + GetActorRotation());

			// Draw Debug Circle at the destination
			if (bDrawDebug)
			{
				FVector Center = Destination + FVector(0.f, 0.f, 50.f);
				FString Label = FString::Printf(TEXT("%d"), GroupData[UnitToSlot[i]].Priority);

				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (PC && PC->PlayerCameraManager)
				{
					FVector CamRight = PC->PlayerCameraManager->GetActorRightVector();
					FVector CamUp = PC->PlayerCameraManager->GetActorUpVector();

					DrawDebugCircle(GetWorld(), Center, 100.f, 64, FColor::Blue, false, 5.f, 0, 5.f, CamRight, CamUp, false);
				}
				DrawDebugString(GetWorld(), Center, Label, nullptr, FColor::White, 5.f, false, 1.5f);
			}
		}
	}
}

void AFormation::StopFormationMove()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) return;

	AIController->StopMovement();
	OnFormationMoveCompleted.Broadcast(false);
	Phase = EFormationPhase::Idle;
}

void AFormation::FallOutFormation()
{
	if (!bBroken)
	{
		bBroken = true;
	}
}

void AFormation::FallInFormation()
{
	if (bBroken)
	{
		bBroken = false;
	}
}

void AFormation::SetRearrangementMode(EFormationRearrangeMode NewMode)
{
	if (RearrangeMode != NewMode)
	{
		RearrangeMode = NewMode;

		if (RearrangeMode == EFormationRearrangeMode::OptimalMovement)
		{
			RearrangeFormation();
		}
	}
}

void AFormation::SetFixedRotationMode(bool bInFixedRotation)
{
	bFixedRotation = bInFixedRotation;
}

void AFormation::ChangeFormationAsset(UFormationAsset* NewFormation)
{
	if (NewFormation)
	{
		for (auto Agent : FormationAgentComponents)
		{
			if (Agent)
			{
				Agent->SetRefAgentData(nullptr);
				Agent->SetAgentData(nullptr);
			}
		}
		SetRefFormationAsset(NewFormation);
		SetFormationAsset( DuplicateObject<UFormationAsset>(NewFormation, GetTransientPackage()));
		RefFormationAsset->ConvertToFormationAgentDatas();
		FormationAsset->ConvertToFormationAgentDatas();
		RearrangeFormation();
		ResizeRefFormationAsset();
	}
}

void AFormation::MoveFormationStart()
{
	const FRotator CurrentRotation = GetActorRotation();

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) return;

	if (bIsFormationMoveStart)
	{
		FormationTurnThreshold = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw);
		Phase = EFormationPhase::Rotating;
		bIsFormationMoveStart = false;
		AIController->StopMovement();

		switch (RearrangeMode)
		{
		case EFormationRearrangeMode::OptimalMovement:
			{
				SetActorRotation(TargetRotation);
				RearrangeFormation();
				break;
			}
		case EFormationRearrangeMode::MaintainSlot:
			{
				SetActorRotation(TargetRotation);
				break;
			}
		case EFormationRearrangeMode::ForcedRotation:
			{
				break;
			}
		default:
			break;
		}
	}
}

void AFormation::RearrangeIfAgentsChanged()
{
	for (int32 i = FormationAgentComponents.Num() - 1; i >= 0; i--)
	{
		if (!FormationAgentComponents[i] ||
			!IsValid(FormationAgentComponents[i]->GetOwner()) ||
			FormationAgentComponents[i]->GetOwner()->IsActorBeingDestroyed())
		{
			FormationAgentComponents.RemoveAt(i);
		}
	}

	if (PreviousFormationAgentComponents != FormationAgentComponents)
	{
		RearrangeFormation();
		ResizeRefFormationAsset();
		PreviousFormationAgentComponents = FormationAgentComponents;
	}
}

void AFormation::ResizeRefFormationAsset()
{
	// Formation Collision Sphere Update By Agents Position
	TArray<FVector> AgentPositions;
	float MaxRadius = 0.0f;

	for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
	{
		if (AgentComponent && AgentComponent->GetRefAgentData())
		{
			FVector Position = AgentComponent->GetComponentLocation();
			AgentPositions.Add(Position);
			if (ACharacter* Character = Cast<ACharacter>(AgentComponent->GetOwner()))
			{
				FBoxSphereBounds CharacterSphere;
				CharacterSphere = Character->GetMesh()->GetLocalBounds();
				MaxRadius = FMath::Max(MaxRadius, CharacterSphere.SphereRadius);
			}
		}
	}

	FSphere Sphere = FFormationWelzl::GetMinimumEnclosingSphere(AgentPositions);

	//RefFormationAsset->FormationRadius = Sphere.W + MaxRadius;
	//RefFormationAsset->FormationCenter = FVector2D(Sphere.Center.X, Sphere.Center.Y);
	//FormationAsset->FormationRadius = RefFormationAsset->FormationRadius;
	//FormationAsset->FormationCenter = FVector2D(Sphere.Center.X, Sphere.Center.Y);
	SphereComponent->SetSphereRadius(FormationAsset->FormationRadius);
	SphereComponent->SetWorldLocation(Sphere.Center);
}

void AFormation::UpdateAgentsBehaviorByState()
{
	switch (Phase)
	{
	case EFormationPhase::Rotating:
		RearrangeMode == EFormationRearrangeMode::ForcedRotation 
			? UpdateAgentsRotation() : UpdateAgentsWithoutRotation();
		break;

	case EFormationPhase::Moving:
		UpdateAgentsLocation();
		break;

	case EFormationPhase::Idle:

	default:
		break;
	}
}

void AFormation::UpdateAgentsRotation()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) return;

	const FRotator CurrentRotation = GetActorRotation();

	// If the rotate direction is based on shortest way to the target rotation
	FRotator NextTickRotation = CurrentRotation + FRotator(0.0f, FMath::Sign(FormationTurnThreshold) * FormationTurnSpeed, 0.0f);
	SetActorRotation(NextTickRotation);

	for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
	{
		if (AgentComponent && AgentComponent->GetAgentData())
		{
			// Calculate the new location for the agent based on the next tick rotation
			// Agent's linear speed is in proportion to the distance from the center of the formation
			FVector Location = NextTickRotation.RotateVector(AgentComponent->GetAgentData()->Position) + FormationCenter;
			AgentComponent->UpdateAgent(Location);
		}
	}

	// Check if the rotation is close enough to the target rotation
	float AbsDeltaYaw = FMath::Abs(FormationTurnThreshold);
	if(AbsDeltaYaw - FormationTurnSpeed < 0.0f)
	{
		Phase = EFormationPhase::Moving;
	}

	// Update the remaining delta yaw
	FormationTurnThreshold -= FMath::Sign(FormationTurnThreshold) * FormationTurnSpeed;
}

void AFormation::UpdateAgentsWithoutRotation()
{
	Phase = EFormationPhase::Moving;
}

void AFormation::UpdateAgentsLocation()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	
	if (!AIController) return;

	const FRotator CurrentFormationRotation = GetActorRotation();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	// formation's agents update their position based on the current rotation and target location
	if (PathPoints.IsValidIndex(CurrentPathIndex))
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_MainLoop);
		for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
		{
			if (!AgentComponent || AgentComponent->GetFormationOwner() != this || !AgentComponent->GetAgentData())
			{
				continue;
			}

			ACharacter* Unit = Cast<ACharacter>(AgentComponent->GetOwner());
			UCharacterMovementComponent* MovementComp = Unit ? Unit->GetCharacterMovement() : nullptr;
			if (!Unit || !MovementComp)
			{
				continue;
			}
			const FVector Destination = CurrentFormationRotation.RotateVector(AgentComponent->GetAgentData()->Position) + FormationCenter;

			if (MovementComp->IsFlying())
			{
				
				const float DistanceToTarget = FVector::Dist(Unit->GetActorLocation(), Destination);
				
				const float DampingDistance = 300.0f; 
			

				const float InterpSpeed = (DistanceToTarget > 1.0f) 
					? FMath::Min(AgentSpeed, AgentSpeed * (DistanceToTarget / DampingDistance)) 
					: 0.0f;

				FVector NewLocation = FMath::VInterpTo(Unit->GetActorLocation(), Destination, DeltaTime, 1.0);
				Unit->SetActorLocation(NewLocation);
			
				// Make the unit face its target location if it's moving
				if (!Unit->GetActorLocation().Equals(Destination, 1.0f))
				{
					FRotator LocalTargetRotation = (Destination - Unit->GetActorLocation()).Rotation();
					// Unit->SetActorRotation(FMath::RInterpTo(Unit->GetActorRotation(), LocalTargetRotation, DeltaTime, 5.0f));
					Unit->SetActorRotation(LocalTargetRotation);

				}
			}
			else if (!AgentComponent->IsStray() && !IsBroken())
			{
				SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_UpdateNormalAgent);
				FVector Location = CurrentFormationRotation.RotateVector(AgentComponent->GetAgentData()->Position) + FormationCenter;
				AgentComponent->UpdateAgent(Location);
			}
			else 
			{
				SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_MoveStrayAgent);
				Unit->GetCharacterMovement()->bOrientRotationToMovement = true;

				AAIController* UnitAIController = Cast<AAIController>(Unit->GetController());
				if (UnitAIController)
				{
					UnitAIController->MoveToLocation(FormationCenter, 10.f);
				}
			}
		}
	}
	else
	{
		OnFormationMoveCompleted.Broadcast(true);
		Phase = EFormationPhase::Idle;
		SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_IdleLoop);
		for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
		{
			if (AgentComponent && AgentComponent->GetFormationOwner() == this)
			{
				UpdateAgentsAIMoveTo(AgentComponent);
			}
		}
	}
}

void AFormation::UpdateAgentsAIMoveTo(UFormationAgentComponent* FormationAgentComponent)
{
	ACharacter* Unit = Cast<ACharacter>(FormationAgentComponent->GetOwner());
	ensure(Unit);

	if(FormationAgentComponent->GetAgentData() == nullptr)
	{
		return;
	}

	FVector Destination = GetActorRotation().RotateVector(FormationAgentComponent->GetAgentData()->Position) + FormationCenter;
	AAIController* UnitAIController = Cast<AAIController>(Unit->GetController());
	if (UnitAIController)
	{
		UnitAIController->MoveToLocation(Destination, 10.f);
	}
}

void AFormation::ExtendFormation()
{
	TArray<AActor*> OverlappingActors;
	ExtendSphereComponent->GetOverlappingActors(OverlappingActors);
	
	if (OverlappingActors.Num() > 0)
	{
		return;
	}

	float ExpansionIntensity = 1.0f + ResizeIntensity;
	const float MaxRadius = RefFormationAsset->FormationRadius;

	if (bBroken &&  FormationAsset->FormationRadius  > FormationAsset->FormationMinRadius)
	{
		FallInFormation();
	}
	// Make orginal formtaion 
	if (FormationAsset->FormationRadius * ExpansionIntensity > MaxRadius)
	{
		FormationAsset->FormationRadius = MaxRadius;
		// Resize the formation asset's same as the reference asset
		for (int32 i = 0; i < FormationAsset->AgentDatas.Num(); i++)
		{
			FormationAsset->AgentDatas[i].Position = RefFormationAsset->AgentDatas[i].Position;
		}
		RearrangeFormation();
		SphereComponent->SetSphereRadius(FormationAsset->FormationRadius);

		
		// No more check expansion chance
		GetWorld()->GetTimerManager().ClearTimer(ExtendTimerHandle);

		return;
	}

	SphereComponent->SetSphereRadius(FormationAsset->FormationRadius * ExpansionIntensity);
	// Resize the formation radius and update the sphere component
	ResizeFormationData(ExpansionIntensity);
}

void AFormation::DownsizeFormation()
{ 
	TArray<AActor*> OverlappingActors;
	SphereComponent->GetOverlappingActors(OverlappingActors);

	if (Phase != EFormationPhase::Idle)
	{
		if (bNeedDownsize || (OverlappingActors.Num() > 0 &&  !bCrashed))
		{
			check(FormationAsset);
			// Ensure the ResizeFactor is within a reasonable range to prevent extreme scaling
			const float MinRadius = FormationAsset->FormationMinRadius;
			float DownsizeIntensity = 1.0f - ResizeIntensity;
			if (FormationAsset->FormationRadius * DownsizeIntensity <= MinRadius)
			{
				FallOutFormation();
				return;
			}
	
			SphereComponent->SetSphereRadius(FormationAsset->FormationRadius * DownsizeIntensity);
			ResizeFormationData(DownsizeIntensity);
	
			// On Timer for Retransform Original Formation 
			if (!GetWorldTimerManager().IsTimerActive(ExtendTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(ExtendTimerHandle,this, &AFormation::ExtendFormation,0.1f,true, 0.0f);
			}
			bNeedDownsize = false;
		}
	}

	bCrashed = false;
}

void AFormation::AdjustLocationToSolveCrash()
{
	if (bCrashed)
	{
		float CorrectIntencity = FMath::Max( OverlapDepth * 1.5, CorrectPathIntensity);
		SetActorLocation(GetActorLocation() - SolveDir * CorrectIntencity);
	}
}

void AFormation::ResizeFormationData(float ResizeFactor)
{
	FormationAsset->FormationRadius *= ResizeFactor;
	// Resize the formation asset's agent data positions
	for (FAgentData& AgentData : FormationAsset->AgentDatas)
	{
		AgentData.Position.X *= ResizeFactor;
		AgentData.Position.Y *= ResizeFactor;
	}
	// Update the formation's Components AgentData positions
	for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
	{
		if (AgentComponent)
		{
			FAgentData* AgentData = AgentComponent->GetAgentData();
			if (AgentData)
			{
				AgentData->Position.X *= ResizeFactor;
				AgentData->Position.Y *= ResizeFactor;
			}
		}
		ACharacter* Unit = Cast<ACharacter>(AgentComponent->GetOwner());
		
		ensure(Unit);

		if (!AgentComponent->GetAgentData())
			continue;
		FVector Destination = GetActorRotation().RotateVector(AgentComponent->GetAgentData()->Position) + FormationCenter;

		AAIController* UnitAIController = Cast<AAIController>(Unit->GetController());
		
		if (UnitAIController)
		{
			UnitAIController->MoveToLocation(Destination, 10.0f);
		}
	}
}

void AFormation::MoveFormationAlongPath(float DeltaTime)
{
	if (PathPoints.IsValidIndex(CurrentPathIndex) && Phase == EFormationPhase::Moving)
	{
		FVector NextPoint = PathPoints[CurrentPathIndex];
		float Distance = FVector::Distance(GetActorLocation(), NextPoint);
		
		if (Distance < 1.0f)
		{
			CurrentPathIndex++;
		}
		else
		{
			FVector MoveDir = (NextPoint - GetActorLocation()).GetSafeNormal();
			FRotator TargetRot = MoveDir.ToOrientationRotator();
			FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime,0.5f);
			SetActorRotation(NewRot);
			SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(),NextPoint, DeltaTime, FormationSpeed));
		}
	}
}

bool AFormation::IsLocationOnNavMesh()
{
	if (auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation OutLocation;
		
		auto temp = GetActorLocation();
		bool bProjected = NavSys->ProjectPointToNavigation(
			GetActorLocation(),
			OutLocation,
			FVector(50.0f, 50.0f, 200.0f),
			nullptr
		);

		if (bProjected)
		{
			LastFormationOnNavMeshLocation = OutLocation.Location;
			return true;
		}
		else
		{
			SetActorLocation(LastFormationOnNavMeshLocation);
			GetController()->StopMovement();
			return false;
		}
	}

	return false;
}

void AFormation::AdjustToGround()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector CurrentLocation = GetActorLocation();
	const FVector Start = CurrentLocation + FVector(0, 0, 100.0f);
	const FVector End = CurrentLocation - FVector(0, 0, 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	const bool bDidHit = World->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_WorldStatic,
		CollisionParams
	);

	if (bDidHit)
	{
		FVector NewLocation = CurrentLocation;
		NewLocation.Z = HitResult.ImpactPoint.Z + 50.0f;
		SetActorLocation(NewLocation);
	}
}

void AFormation::DrawDebugData()
{
	if (bDrawDebug)
	{
		DrawDebugPath(PathPoints);

		// Raw path data
		if (!RawPathPoints.IsEmpty() && bDrawDebug)
		{
			for (int32 i = 0; i < RawPathPoints.Num() - 1; i++)
			{
				DrawDebugLine(GetWorld(), RawPathPoints[i], RawPathPoints[i + 1], FColor::Blue, false, -1.0f, 0, 5.0f);
			}
		}
	}
}

void AFormation::DrawDebugPath(const TArray<FVector>& Path)
{
	if (!Path.IsEmpty() && bDrawDebug)
	{
		for (int32 i = 0; i < Path.Num() - 1; i++)
		{
			DrawDebugLine(GetWorld(), Path[i], Path[i + 1], FColor::Green, false, -1.0f, 0, 5.0f);
			DrawDebugSphere(GetWorld(), GetActorLocation(), 30.0f, 12, FColor::Red, false);
		}
	}
}
