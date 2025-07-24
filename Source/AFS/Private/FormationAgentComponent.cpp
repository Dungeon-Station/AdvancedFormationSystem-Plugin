/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "FormationAgentComponent.h"
#include "Formation.h"
#include "FormationAsset.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"

DECLARE_CYCLE_STAT(TEXT("Agent - UpdateAgent (Total)"), STAT_UpdateAgent_Total, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Agent - Movement Component Setup"), STAT_UpdateAgent_MovementSetup, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Agent - AIController StopMovement"), STAT_UpdateAgent_AIControllerStop, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Agent - Movement Input & Rotation"), STAT_UpdateAgent_MovementInputRotation, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Agent - Manual Rotation (Shrunk)"), STAT_UpdateAgent_ManualRotation, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Agent - Auto Rotation (Normal)"), STAT_UpdateAgent_AutoRotation, STATGROUP_Formation);

UFormationAgentComponent::UFormationAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
	GroupName = FName("Default");
}

void UFormationAgentComponent::BeginPlay()
{
    Super::BeginPlay();

    if (FormationOwner)
    {
        FormationOwner->RegisterAgent(this);
    }

    // Initialize character movement settings.
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        // Prevent controller rotation from affecting character pitch, yaw, roll.
        Character->bUseControllerRotationPitch = false;
        Character->bUseControllerRotationYaw = false;
        Character->bUseControllerRotationRoll = false;
        Character->GetCharacterMovement()->RotationRate = FRotator(0, 180, 0);
        Character->OnActorHit.AddDynamic(this, &UFormationAgentComponent::OnCharacterHit);
    }
}

void UFormationAgentComponent::LostFormation(float DeltaTime)
{
    if (FormationOwner && FVector::Distance(GetOwner()->GetActorLocation(), TargetLocation) > FormationOwner->GetSphereComponent()->GetScaledSphereRadius()
        && FVector::Distance(GetOwner()->GetActorLocation(), FormationOwner->GetActorLocation()) > FormationOwner->GetSphereComponent()->GetScaledSphereRadius())
    { 
        LostTimer+= DeltaTime;
        if (LostTimer > LostTreshold && !bStray && !GetFormationOwner()->IsBroken())
        {
            bStray = true;
            UCharacterMovementComponent* MovementComponent = Cast<ACharacter>(GetOwner())->GetCharacterMovement();
            MovementComponent->MaxWalkSpeed = GetFormationOwner()->GetStrayAgentSpeed();
            MovementComponent->MaxAcceleration = GetFormationOwner()->GetStrayAgentSpeed() * 2.0f;
        }
    }
    else if (FormationOwner)
    {
        if (bStray)
        {
            bStray =false;
        }
        LostTimer =0;
    }
}

void UFormationAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    LostFormation(DeltaTime);
}

// Collision callback: adjusts agent movement on impact with static objects.
void UFormationAgentComponent::OnCharacterHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
    // Only handle collisions with non-pawn static actors.
    if (OtherActor && OtherActor != GetOwner() && !Cast<APawn>(OtherActor) && FormationOwner)
    {
        if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
        {
            // Compute reflection vector based on hit normal.
            FVector DesiredDirection = Character->GetActorForwardVector().GetSafeNormal();
            FVector HitNormal = Hit.ImpactNormal.GetSafeNormal();
            float Dot = FVector::DotProduct(DesiredDirection, HitNormal);
            DesiredDirection = FMath::GetReflectionVector(DesiredDirection, HitNormal).GetSafeNormal();

            // If collision was nearly head-on, choose side direction.
            if (FMath::Abs(Dot) > 0.95f)
            {
                FVector VectorA = FVector::CrossProduct(Hit.ImpactNormal, FVector::UpVector).GetSafeNormal();
                FVector VectorB = -VectorA;
                float DotA = FVector::DotProduct(DesiredDirection, VectorA);
                float DotB = FVector::DotProduct(DesiredDirection, VectorB);
                DesiredDirection = (DotA >= DotB) ? VectorA : VectorB;
            }

            // Apply movement input to character to slide along the obstacle.
            Character->AddMovementInput(DesiredDirection, 1.0f);
        }
    }
}

// Updates the agent's navigation target based on the desired slot location.
void UFormationAgentComponent::UpdateAgent(const FVector& Location)
{
    SCOPE_CYCLE_COUNTER(STAT_UpdateAgent_Total);

    if (AgentData == nullptr)
    {
        return;  // No data assigned, nothing to update.
    }

    TargetLocation = Location;

    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        {
            SCOPE_CYCLE_COUNTER(STAT_UpdateAgent_MovementSetup);
            if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
            {
                if (FormationOwner)
                {
                    const float DistanceToTarget = FVector::Distance(Character->GetActorLocation(), Location);

                    const float BaseSpeed = FormationOwner->GetFormationSpeed();
                    const float MaxSpeed = FormationOwner->GetAgentSpeed();
                    const float MinDistanceForMaxSpeed = 500.0f; // TODO : Find Standard

                    const float NewSpeed = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, MinDistanceForMaxSpeed), FVector2D(BaseSpeed, MaxSpeed), DistanceToTarget);

                    MovementComponent->MaxWalkSpeed = NewSpeed;
                    MovementComponent->MaxAcceleration = NewSpeed * 2.0f;
                }
            }
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_UpdateAgent_AIControllerStop);
            if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
            {
                AIController->StopMovement();
            }
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_UpdateAgent_MovementInputRotation);
            FVector DesiredDirection = (Location - Character->GetActorLocation()).GetSafeNormal();
            if (!DesiredDirection.IsNearlyZero() && FormationOwner)
            {
                // If formation has shrunk, lock orientation and manually rotate.
                // SCOPE_CYCLE_COUNTER(STAT_UpdateAgent_AutoRotation);
                // Character->GetCharacterMovement()->bOrientRotationToMovement = true;
                // Character->AddMovementInput(DesiredDirection, 1.0f);

                if (FormationOwner->bFixedRotation)
                {
                    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
                    Character->AddMovementInput(DesiredDirection, 1.0f);

                    const FRotator TargetRotation = FormationOwner->GetActorRotation() + AgentData->Rotation;

                    const FRotator CurrentRotation = Character->GetActorRotation();
                    if (!CurrentRotation.Equals(TargetRotation, RotationTolerance))
                    {
                        const float RotationInterpSpeed = 5.0f;

                        const FRotator NewRotation = FMath::RInterpTo(
                            CurrentRotation,
                            TargetRotation,
                            GetWorld()->GetDeltaSeconds(),
                            RotationInterpSpeed
                        );

                        Character->SetActorRotation(NewRotation);
                    }
                }
                else
                {
                    if (FormationOwner->GetFormationAsset()->FormationRadius < FormationOwner->GetRefFormationAsset()->FormationRadius)
                    {
                        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
                        Character->AddMovementInput(DesiredDirection, 1.0f);
                        Character->SetActorRotation(FormationOwner->GetActorRotation());
                    }
                    else
                    {
                        Character->GetCharacterMovement()->bOrientRotationToMovement = true;
                        Character->AddMovementInput(DesiredDirection, 1.0f);
                    }
                }
            }
        }
    }
}

