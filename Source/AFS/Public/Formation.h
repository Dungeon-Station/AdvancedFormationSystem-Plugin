// Formation.h
// Responsible for managing a group of agents moving and rotating in formation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PathModifierTypes.h"
#include "Stats/Stats.h"
#include "Formation.generated.h"

DECLARE_STATS_GROUP(TEXT("Formation"), STATGROUP_Formation, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Formation - FormationTick"), STAT_FormationTick, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - RearrangeIfAgentsChanged"), STAT_RearrangeIfAgentsChanged, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - MoveFormationStart"), STAT_MoveFormationStart, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - UpdateAgentsBehaviorByState"), STAT_UpdateAgentsBehaviorByState, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - MoveFormationAlongPath"), STAT_MoveFormationAlongPath, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - AdjustLocationToSolveCrash"), STAT_AdjustLocationToSolveCrash, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - DownsizeFormation"), STAT_DownsizeFormation, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - DrawDebugPath"), STAT_DrawDebugPath, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - UpdateRotation"), STAT_UpdateRotation, STATGROUP_Formation);

DECLARE_CYCLE_STAT(TEXT("Formation - UpdateAgentsLocation_MainLoop"), STAT_UpdateAgentsLocation_MainLoop, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - UpdateAgentsLocation_UpdateNormalAgent"), STAT_UpdateAgentsLocation_UpdateNormalAgent, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - UpdateAgentsLocation_MoveStrayAgent"), STAT_UpdateAgentsLocation_MoveStrayAgent, STATGROUP_Formation);
DECLARE_CYCLE_STAT(TEXT("Formation - UpdateAgentsLocation_IdleLoop"), STAT_UpdateAgentsLocation_IdleLoop, STATGROUP_Formation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFormationMoveCompleted);

class UFloatingPawnMovement;
class USphereComponent;
class UFormationAgentComponent;
class UFormationAsset;
class AAIController;

UENUM()
enum class EFormationRearrangeMode : uint8
{
	OptimalMovement UMETA(DisplayName = "Optimal Movement"), /** Rearrange agents using optimal movement logic */
	MaintainSlot UMETA(DisplayName = "Maintain Slot"), /** Keep agents in their current slots without rearranging */
	ForcedRotation UMETA(DisplayName = "Forced Rotation") /** Force agents to rotate without changing their slots */
};

/**
 * Phases of the formation behavior: Idle, Rotating in place, or Moving along a path.
 */
UENUM()
enum class EFormationPhase : uint8
{
    Idle,       /** No active movement or rotation */
    Rotating,   /** Formation is rotating to face target before moving */
    Moving      /** Formation is moving to its target location */
};

/**
 * AFormation pawn controls a group of agents following formation logic.
 */
UCLASS()
class AFS_API AFormation : public APawn
{
    GENERATED_BODY()

public:
    AFormation();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    /**
     * Callback when another actor begins overlap with the formation's sphere.
     * Used for obstacle detection or agent registration.
     */
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
public:
	/**
	 * Initiates movement of the formation to a target location and rotation.
	 * @param Location Destination point in world space.
	 * @param Rotation Desired facing rotation at destination.
	 */
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void FormationMoveTo(const FVector& Location, const FRotator& Rotation);
	/**
	 * Registers a new agent component to be managed by this formation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void RegisterAgent(UFormationAgentComponent* AgentComponent);
	/**
	 * Recomputes agent slots after formation size or shape changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void RearrangeFormation();

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void FallOutFormation();

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void FallInFormation();

	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "MoveCompleted"))
	FFormationMoveCompleted OnFormationMoveCompleted;
	
private:
	/**
	 * Sets up movement state when starting a new move command.
	 */
	void MoveFormationStart();
	/**
	 * Checks if the formation's units have changed since last update.
	 */
	void RearrangeIfAgentsChanged();
	/**
	* Updates each agent's behavior based on current formation phase.
	*/
	void UpdateAgentsBehaviorByState();
	/**
	 * Rotates agents in place to maintain formation orientation.
	 */
	void UpdateAgentsRotation();

	/**
	 * Moves agents without handling rotation (straight translation).
	 */
	void UpdateAgentsWithoutRotation();

	/**
	 * Adjusts agent locations according to the current formation pose.
	 */
	void UpdateAgentsLocation();

	/**
	 * Moves a specific agent using AI MoveTo for smooth pathfinding.
	 * @param FormationAgentComponent The agent to move.
	 */
	void UpdateAgentsAIMoveTo(UFormationAgentComponent* FormationAgentComponent);

	/**
	 * Temporarily expand formation (e.g., to avoid obstacles) then revert.
	 */
	void ExtendFormation();
    
	/**
	 * Adjust formation spread to avoid collisions dynamically.
	 */
	void DownsizeFormation();

	/**
	 * Modify target positions slightly to resolve intersection with obstacles.
	 */
	void AdjustLocationToSolveCrash();
	
	void ResizeFormationData(float ExpansionIntensity);
	/**
	 * Drives the formation along its computed path each tick.
	 * @param DeltaTime Time since last frame for interpolation.
	 */
	void MoveFormationAlongPath(float DeltaTime);

	bool IsLocationOnNavMesh();

public:
	UFUNCTION(BlueprintCallable, Category = "Formation")
    float GetFormationSpeed() const { return FormationSpeed; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
    float GetAgentSpeed() const { return AgentSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Formation")
	float GetStrayAgentSpeed() const { return StrayAgentSpeed; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
    float GetAgentAcceleration() const { return AgentAcceleration; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
    TArray<UFormationAgentComponent*> GetFormationAgentComponents() const { return FormationAgentComponents; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
    UFormationAsset* GetFormationAsset() const { return FormationAsset; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void SetFormationAsset(UFormationAsset* NewFormationAsset) { FormationAsset = NewFormationAsset; }
	
    UFUNCTION(BlueprintCallable, Category = "Formation")
    UFormationAsset* GetRefFormationAsset() const { return RefFormationAsset; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void SetRefFormationAsset(UFormationAsset* NewRefFormationAsset) { RefFormationAsset = NewRefFormationAsset; }
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
    EFormationPhase GetFormationPhase() const { return Phase; }

	UFUNCTION(BlueprintCallable, Category = "Formation")
	bool IsBroken() { return bBroken; };
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void ResizeRefFormationAsset();
	
	USphereComponent* GetSphereComponent() const { return SphereComponent; }
private:
    /** Current operational phase of the formation. */
    EFormationPhase Phase = EFormationPhase::Idle;

	/** Computed path points for the formation to follow. */
	UPROPERTY()
	TArray<FVector> PathPoints;

    /** Collision sphere used for overlap detection. */
	UPROPERTY(VisibleAnywhere, Category = "Formation", meta = (ToolTip = "Collision sphere component used for overlap detection in the formation."))
	USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "Formation", meta = (ToolTip = "Additional collision sphere component used for predict formation expanding."))
	USphereComponent* ExtendSphereComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Formation", meta = (ToolTip = "Component that controls the movement of the formation."))
	UFloatingPawnMovement* MoveComponent;
    
    /** All agents registered to this formation. */
    UPROPERTY()
    TArray<UFormationAgentComponent*> FormationAgentComponents;

    /** Previous tick's enabled agents */
    UPROPERTY()
	TArray<UFormationAgentComponent*> PreviousFormationAgentComponents;

    /** Base asset defining the formation layout (rows/cols). */
	UPROPERTY(EditAnywhere, Category = "Formation", meta = (ToolTip = "Defines the base layout asset for the formation."))
	UFormationAsset* RefFormationAsset;

    /** Working copy of the formation asset (may be resized). */
    UPROPERTY()
    UFormationAsset* FormationAsset;
    
	/** Rearrange formation's behavior. */
	UPROPERTY(EditAnywhere, Category = "Formation", meta = (ToolTip = "Specifies the rearrangement mode for the formation."))
	EFormationRearrangeMode RearrangeMode = EFormationRearrangeMode::OptimalMovement;

    /** Internal flag indicating move has started. */
    bool bIsFormationMoveStart = false;

    /** Target point for the formation's movement. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Target location for the formation to move to."))
	FVector TargetLocation;

    /** Desired ending rotation for the formation. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Target rotation for the formation upon arrival."))
	FRotator TargetRotation;

    /** Total yaw to cover when rotating. */
    float FormationTurnThreshold;

    /** Yaw change per tick during rotation. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Rotation speed of the formation (degrees per second)."))
	float FormationTurnSpeed;

    /** Speed at which the formation moves. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Movement speed of the entire formation."))
	float FormationSpeed;

    /** Maximum speed of individual agents. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Maximum movement speed for individual agents."))
	float AgentSpeed;

	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Movement speed for agents that have strayed from the formation."))
	float StrayAgentSpeed;
	
    /** Acceleration rate for agents to reach AgentSpeed. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Acceleration rate for agents to reach their maximum speed."))
	float AgentAcceleration;

    /** Current index into PathPoints being followed. */
    UPROPERTY()
    int32 CurrentPathIndex;

    /** Configuration for applying path modifiers (e.g., smoothing). */
    UPROPERTY(EditAnywhere, Category = "Formation")
    FPathModifierConfig ModifierConfig;

    /** Intensity factor when resizing formation. */
	UPROPERTY(EditAnywhere, Category = "FormationMovement", meta = (ToolTip = "Intensity factor applied when resizing the formation (0~1)."))
	float ResizeIntensity = 0.1f;
    
    /** Number of segments to use when correcting the path. */
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ToolTip = "Number of forward segments used for path correction."))
	int32 CorrectPathNum = 5;

    /** Strength of path correction adjustments. */
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ToolTip = "Strength of the path correction adjustment."))
	float CorrectPathIntensity = 20.0f;
	
	UPROPERTY()
	bool bBroken = false;
	
	float PrevCrashAngle = 0.0;
    /** Flag indicating a crash/overlap was detected. */
    bool bCrashed = false;
	bool bNeedDownsize = false;
    /** Direction to apply to resolve a crash. */
    FVector SolveDir = FVector::ZeroVector;

	
	/** Timer handle used for scheduling the revert of expansion. */
	FTimerHandle ExtendTimerHandle;
	
	float TargetFormationScale = 1.0f;
	float CurrentFormationScale = 1.0f;
	float PreviousFormationScale = 1.0f; 

	UPROPERTY(EditAnywhere, Category = "Formation|Sizing", meta = (ClampMin = "0.1", ToolTip = "Interpolation speed for formation scaling. Minimum value is 0.1."))
	float ScaleInterpolationSpeed = 2.0f;

	FVector LastFormationOnNavMeshLocation;

	/* Debug Session */
public:
	/** Enable or disable debug drawing of formation behavior. */
	UPROPERTY(EditAnywhere, Category = "Formation Debug", meta = (ToolTip = "Whether to draw debug information for the formation."))
	bool bDrawDebug = true;

private:
	
	void DrawDebugPath(const TArray<FVector>& Path);
	
	/** @return Flag to enable debug drawing. */
	bool GetDrawDebug() const { return bDrawDebug; }
};