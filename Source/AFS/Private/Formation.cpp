//================================================================
// AFormation.cpp
//================================================================

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


AFormation::AFormation()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Formation Center");
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Formation Collsion");
	SphereComponent->SetupAttachment(RootComponent);

	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	SphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	
	SphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AFormation::OnSphereBeginOverlap);

	ExtendSphereComponent = CreateDefaultSubobject<USphereComponent>("Extend Sphere");
	ExtendSphereComponent->SetupAttachment(SphereComponent);

	ExtendSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	ExtendSphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	
	ExtendSphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	
	ExtendSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	ExtendSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	
	MoveComponent = CreateDefaultSubobject<UFloatingPawnMovement>("MoveComponent");
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

	// Initialize the formation asset if it is not set
	if (RefFormationAsset)
	{
		SphereComponent->SetSphereRadius(RefFormationAsset->FormationRadius);
		FormationAsset = DuplicateObject<UFormationAsset>(RefFormationAsset, GetTransientPackage());
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

	for (int32 i = 0; i < FormationAsset->AgentDatas.Num(); i++)
	{
		if (FormationAgentComponents.Num() <= i)
		{
			break;
		}

		if(FormationAgentComponents[i] == nullptr || FormationAgentComponents[i]->GetOwner() == nullptr)
		{
			continue;
		}

		FormationAgentComponents[i]->GetOwner()->SetActorLocation(FormationAsset->AgentDatas[i].Position + GetActorLocation());
		FormationAgentComponents[i]->GetOwner()->SetActorRotation(FormationAsset->AgentDatas[i].Rotation + GetActorRotation());
	}
}

void AFormation::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_FormationTick);

	Super::Tick(DeltaTime);

	// if (!FMath::IsNearlyEqual(CurrentFormationScale, TargetFormationScale))
	// {
	// 	CurrentFormationScale = FMath::FInterpTo(CurrentFormationScale, TargetFormationScale, DeltaTime, ScaleInterpolationSpeed);
	// 	
	// 	// 2. ��이 �제�변경되�을 �만 ResizeFormationData른출�여 �동 명령
	// 	if (!FMath::IsNearlyEqual(CurrentFormationScale, PreviousFormationScale))
	// 	{
	// 		if (PreviousFormationScale > 1e-6f) // 0�로 �누�방�
	// 		{
	// 			const float ResizeFactor = CurrentFormationScale / PreviousFormationScale;
	// 			ResizeFormationData(ResizeFactor);
	// 		}
	// 		PreviousFormationScale = CurrentFormationScale;
	// 	}
	// }
	
	// SphereComponent�기�본 �셋기�로 �� �데�트
	// if (RefFormationAsset)
	// {
	// 	SphereComponent->SetSphereRadius(RefFormationAsset->FormationRadius * CurrentFormationScale);
	// }

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
		DrawDebugPath(PathPoints);
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
	
	float MyRadius = SphereComponent->GetScaledSphereRadius();
	FVector Dir = (ClosestPointOnOther - MyCenter).GetSafeNormal();
	FVector ImpactPoint = MyCenter + Dir * MyRadius;
	
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), ImpactPoint, 20.0f, 12, FColor::Red, false, 2.0f);
	}
	
	FVector CloseDir = (ImpactPoint - MyCenter).GetSafeNormal();
	float CosCurDir = GetActorForwardVector().Dot(CloseDir);
	float CurAngle = acos(CosCurDir);
	if ( GetActorForwardVector().Cross(CloseDir).Z < 0.0f )
		CurAngle *= -1.0f;
	float CurDegree = FMath::RadiansToDegrees(CurAngle);
	UE_LOG(LogTemp, Warning, TEXT("BeginOverlap [Cur Degree] %f [Prev Degree] %f"), CurDegree, PrevCrashAngle);
	if (CurDegree * PrevCrashAngle < 1.0f)
	{
		bNeedDownsize = true;
		PrevCrashAngle = CurDegree;
		return;
	}
	PrevCrashAngle = CurDegree;
	bCrashed = true;
	SolveDir = CloseDir;
	for (int i = CurrentPathIndex; i< PathPoints.Num() && i< CurrentPathIndex + CorrectPathNum; i++)
	{
		PathPoints[i] -= (CloseDir  * CorrectPathIntensity);
	}
}

void AFormation::FormationMoveTo(const FVector& Location, const FRotator& Rotation)
{
	PrevCrashDir = FVector::ZeroVector;
	PrevCrashAngle = 0.0f;
	TargetLocation = Location;
	TargetRotation = Rotation;
	bIsFormationMoveStart = true;
}

void AFormation::RegisterAgent(UFormationAgentComponent* AgentComponent)
{
	FormationAgentComponents.Add(AgentComponent);
}

void AFormation::RearrangeFormation()
{
	int32 UnitNum = FormationAgentComponents.Num();
	// If there are no enabled agents, do nothing
	if (UnitNum == 0) return;

	TArray<FAgentData> AssetAgentData = FormationAsset->AgentDatas;
	TArray<TArray<FAgentData>> AgentDatasByGroupName;
	AgentDatasByGroupName.SetNum(FormationAsset->GroupNames.Num());

	for(const FAgentData& AgentData : FormationAsset->AgentDatas)
	{
		int32 GroupIndex = FormationAsset->GroupNames.IndexOfByKey(AgentData.GroupName);
		AgentDatasByGroupName[GroupIndex].Add(AgentData);
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
			if (FormationAgentComponent->GetGroupName() == GroupData[0].GroupName)
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
				const FVector SlotLocation = GetActorRotation().RotateVector(GroupData[j].Position) + GetActorLocation();

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
			if(FormationAgentComponentsInGroup.Num() <= i)
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
			FVector Destination = GetActorRotation().RotateVector(GroupData[UnitToSlot[i]].Position) + GetActorLocation();

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

void AFormation::FallOutFormation()
{
	if (!bBroken)
	{
		UE_LOG(LogTemp, Display, TEXT("FallOutFormation"));
		bBroken = true;
	}
}

void AFormation::FallInFormation()
{
	if (bBroken)
	{
		UE_LOG(LogTemp, Display, TEXT("FallInFormation"));
		bBroken = false;
	}
}

void AFormation::ResizeRefFormationAsset()
{
	// Formation Collision Sphere Update By Agents Position
	TArray<FVector2D> AgentPositions2D;
	float MaxRadius = 0.0f;
	for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
	{
		if (AgentComponent && AgentComponent->GetAgentData())
		{
			FVector Position = AgentComponent->GetAgentData()->Position;
			AgentPositions2D.Add(FVector2D(Position.X, Position.Y));
			if (ACharacter* Character = Cast<ACharacter>(AgentComponent->GetOwner()))
			{
				FVector Origin, BoxExtent;
				bool bOnlyCollidingComponents = false;
				Character->GetActorBounds(bOnlyCollidingComponents, Origin, BoxExtent);
				float MaxXY = FMath::Max(BoxExtent.X, BoxExtent.Y);
				MaxRadius = FMath::Max(MaxRadius, MaxXY);
			}
		}
	}

	FCircle Circle = FFormationWelzl::GetMinimumEnclosingCircle(AgentPositions2D);
	RefFormationAsset->FormationRadius = Circle.Radius + MaxRadius;
	RefFormationAsset->FormationCenter = Circle.Center;
	FormationAsset->FormationRadius = Circle.Radius + MaxRadius;
	FormationAsset->FormationCenter = Circle.Center;
	SphereComponent->SetSphereRadius(FormationAsset->FormationRadius);
	SphereComponent->SetRelativeLocation(FVector(Circle.Center.X, Circle.Center.Y, 0.f));
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
	if (PreviousFormationAgentComponents != FormationAgentComponents)
	{
		RearrangeFormation();
		ResizeRefFormationAsset();
		PreviousFormationAgentComponents = FormationAgentComponents;
	}
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

	const FVector CurrentLocation = GetActorLocation();
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
			FVector Location = NextTickRotation.RotateVector(AgentComponent->GetAgentData()->Position) + CurrentLocation;
			AgentComponent->UpdateAgent(Location);
		}
	}

	// Check if the rotation is close enough to the target rotation
	float AbsDeltaYaw = FMath::Abs(FormationTurnThreshold);
	if(AbsDeltaYaw - FormationTurnSpeed < 0.0f)
	{
		if (AIController)
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (!NavSys) return;

			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(),CurrentLocation,TargetLocation);
			if (!NavPath || NavPath->PathPoints.Num() == 0)
			{
				Phase = EFormationPhase::Idle;
				return; 
			}
			CurrentPathIndex = 0;

			PathPoints = UFormationPathModifier::ApplyPathCorrection(GetWorld(), NavPath->PathPoints, RefFormationAsset, ModifierConfig);
		}

		Phase = EFormationPhase::Moving;
	}

	// Update the remaining delta yaw
	FormationTurnThreshold -= FMath::Sign(FormationTurnThreshold) * FormationTurnSpeed;
}

void AFormation::UpdateAgentsWithoutRotation()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) return;

	const FVector CurrentLocation = GetActorLocation();
	const FRotator CurrentRotation = GetActorRotation();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(),CurrentLocation,TargetLocation);
	if (!NavPath || NavPath->PathPoints.Num() == 0)
	{
		Phase = EFormationPhase::Idle;
		return; 
	}
	CurrentPathIndex = 0;

	PathPoints = UFormationPathModifier::ApplyPathCorrection(GetWorld(), NavPath->PathPoints, RefFormationAsset, ModifierConfig);
	
	Phase = EFormationPhase::Moving;
}

void AFormation::UpdateAgentsLocation()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) return;

	const FVector CurrentLocation = GetActorLocation();
	const FRotator CurrentRotation = GetActorRotation();

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

			if (!AgentComponent->IsStray() && !IsBroken())
			{
				SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_UpdateNormalAgent);
				FVector Location = CurrentRotation.RotateVector(AgentComponent->GetAgentData()->Position) + CurrentLocation;
				AgentComponent->UpdateAgent(Location);
			}
			else 
			{
				SCOPE_CYCLE_COUNTER(STAT_UpdateAgentsLocation_MoveStrayAgent);
				ACharacter* Unit = Cast<ACharacter>(AgentComponent->GetOwner());
				Unit->GetCharacterMovement()->bOrientRotationToMovement = true;

				FVector Destination = GetActorRotation().RotateVector(AgentComponent->GetAgentData()->Position) + GetActorLocation();
				AAIController* UnitAIController = Cast<AAIController>(Unit->GetController());
				if (UnitAIController)
				{
					UnitAIController->MoveToLocation(GetActorLocation(), 10.f);
				}
			}
		}
	}
	else
	{
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

	FVector Destination = GetActorRotation().RotateVector(FormationAgentComponent->GetAgentData()->Position) + GetActorLocation();
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
		if (bNeedDownsize || (OverlappingActors.Num() > 1 &&  !bCrashed))
		{
			check(FormationAsset);
			
			UE_LOG(LogTemp, Error, TEXT("Overlap Num : %d"), OverlappingActors.Num());
	
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
		SetActorLocation(GetActorLocation() - SolveDir * CorrectPathIntensity);
		// bCrashed = false;
	}
}

void AFormation::ResizeFormationData(float ResizeFactor)
{
	FormationAsset->FormationRadius *= ResizeFactor;
	// Resize the formation asset's agent data positions
	for (FAgentData& AgentData : FormationAsset->AgentDatas)
	{
		AgentData.Position *= ResizeFactor;
	}
	// Update the formation's Components AgentData positions
	for (UFormationAgentComponent* AgentComponent : FormationAgentComponents)
	{
		if (AgentComponent)
		{
			FAgentData* AgentData = AgentComponent->GetAgentData();
			if (AgentData)
			{
				AgentData->Position *= ResizeFactor;
			}
		}
		ACharacter* Unit = Cast<ACharacter>(AgentComponent->GetOwner());
		ensure(Unit);
		FVector Destination = GetActorRotation().RotateVector(AgentComponent->GetAgentData()->Position) + GetActorLocation();

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

void AFormation::DrawDebugPath(const TArray<FVector>& Path)
{
	if (!PathPoints.IsEmpty() && bDrawDebug)
	{
		for (int32 i = 0; i < Path.Num() - 1; i++)
		{
			DrawDebugLine(GetWorld(), Path[i], Path[i + 1], FColor::Green, false, 5.0f, 0, 2.0f);
			DrawDebugSphere(GetWorld(), GetActorLocation(), 30.0f, 12, FColor::Red, false);
		}
	}
}
