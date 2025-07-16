/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Navigation/CrowdManager.h"
#include "NavigationSystem.h"
#include "Navigation/CrowdAgentInterface.h"
#include "Stats/Stats.h"
#include "FormationAgentComponent.generated.h"

class AFormation;
struct FAgentData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AFS_API UFormationAgentComponent : public UActorComponent, public ICrowdAgentInterface
{
    GENERATED_BODY()

public:
    UFormationAgentComponent();
    
protected:
    virtual void BeginPlay() override;


public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    UFUNCTION()
    void OnCharacterHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

    /**
     * Update the agent's target position in the world.
     * @param Location Desired world-space location for the agent.
     */
    UFUNCTION()
    void UpdateAgent(const FVector& Location);

    /*
     When agent lose agent's way, Change his state
     If you wanna change the lost timing, customize this function 
     */
    UFUNCTION(Blueprintable, Category = "Formation")
    void LostFormation(float DeltaTime);

    /**
     * Accessor for the underlying original agent data (position, slot index, etc.).
     */
    FAgentData* GetRefAgentData() const { return RefAgentData; }
    
    /**
     * Accessor for the underlying agent data (position, slot index, etc.).
     */
    FAgentData* GetAgentData() const { return AgentData; }
    /**
     * Assigns the agent's data struct (filled by formation asset).
     */
    void SetRefAgentData(FAgentData* InAgentData) { RefAgentData = InAgentData; }
    /**
     * Assigns the agent's data struct (filled by formation asset).
     */
    void SetAgentData(FAgentData* InAgentData) { AgentData = InAgentData; }

    /**
     * Retrieves the formation owner that manages this agent.
     */
    UFUNCTION(BlueprintCallable, Category = "Formation")
    AFormation* GetFormationOwner() const { return FormationOwner; }

    UFUNCTION(BlueprintCallable, Category = "Formation")
    void SetFormationOwner(AFormation* InFormationOwner)
    {
        FormationOwner = InFormationOwner;
	}

    bool IsStray() const { return bStray; }
    void SetStray(bool InStray) { bStray = InStray; }

    
	FName GetGroupName() const { return GroupName; }
	void SetGroupName(FName InGroupName) { GroupName = InGroupName; }
private:
    /**
     * Pointer to the agent's original position and slot metadata.
     */
    FAgentData* RefAgentData;
    /**
     * Pointer to the agent's position and slot metadata.
     */
    FAgentData* AgentData;
    /**
     * The formation that owns and commands this agent.
     */
    UPROPERTY(EditInstanceOnly, Category = "Formation")
    AFormation* FormationOwner;

	UPROPERTY(EditAnywhere, Category = "Formation")
    FName GroupName;
    
    FVector TargetLocation;
    float LostTimer = 0;
    float LostTreshold = 2.0f;

    bool bStray = false;
};
