Formation System
This document describes the core classes and components of the Formation System for Unreal Engine, including their purposes and member variables. Use this as a reference for integration, extension, or understanding the internal structure.

C++ Classes
AFormation
Represents a formation entity that manages characters with UFormationAgentComponent. Responsible for formation creation, disbanding, movement, transformation, and management of its agents.

Components
Formation Center (USceneComponent): Serves as the reference point for the formation. Agents' positions are calculated relative to this center.

Formation Collision (USphereComponent): Defines the size of the formation. Used for path correction and transformation checks. The radius is determined by the FFormationWelzl class, considering the largest agent in the formation.

Extend Formation Collision (USphereComponent): An expanded collision component used to determine if the formation can be expanded after being contracted.

Move Component (UFloatingPawnMovement): Handles the movement of the AFormation actor.

Main Features
Formation Creation: Reads data from UFormationAsset to create the formation. Uses FFormationHungarian for optimal and fast arrangement.

Formation Movement: Generates a path using Unreal Engine's Navigation system, then modifies it (using FFormationPathModifier) to account for the formation's size. Continuously updates the path and commands agents to move accordingly.

Formation Transformation: Adjusts the collision size and agent offsets when shrinking or expanding the formation.

Formation Disband/Reformation: Can temporarily disband and later reform the formation, especially when the formation becomes too small.

Member Variables
Name	Purpose
EFormationPhase	Current state (Idle, Rotating, Moving)
USphereComponent* SphereComponent	Collision defining formation size
USphereComponent* ExtendSphereComponent	Collision for expansion checks
UFloatingPawnMovement* MoveComponent	Controls formation movement
TArray<UFormationAgentComponent*> FormationAgentComponents	Agent components currently in the formation
TArray<UFormationAgentComponent*> PreviousFormationAgentComponents	Agent components from previous tick
UFormationAsset* RefFormationAsset	Reference to the original formation data
UFormationAsset* FormationAsset	Copy of the formation data (mutable during transformation)
EFormationRearrangeMode RearrangeMode	Rearrangement mode for the formation
bool bIsFormationMoveStart	Whether the formation has started moving
FVector TargetLocation	Target location for movement
FRotator TargetRotation	Target rotation for movement
float FormationTurnThreshold	Total rotation amount when turning
float FormationTurnSpeed	Speed of rotation
float FormationSpeed	Formation movement speed
float AgentSpeed	Common speed for agents within the formation
float StrayAgentSpeed	Speed for agents rejoining the formation
float AgentAcceleration	Acceleration for agents
TArray<FVector> PathPoints	Path points for formation movement
int32 CurrentPathIndex	Current index in the path
FPathModifierConfig ModifierConfig	Path modification settings
float ResizeIntensity	Intensity of formation transformation
int32 CorrectPathNum	Number of path points to correct on collision
float CorrectPathIntensity	Intensity of path correction
bool bBroken	Whether the formation is disbanded
bool bDrawDebug	Whether to draw debug elements
UFormationAgentComponent
A component added to actors to make them members of an AFormation. It enables centralized management and movement control as part of the formation.

Main Features
Formation Membership: Associates the actor with a specific AFormation and subjects it to formation control.

Target Location Updates: Continuously receives and updates the world-space target location from its AFormation.

Avoidance Interaction: Implements avoidance logic using Unreal Engine's RVO and custom behaviors via ICrowdAgentInterface.

Member Variables
Name	Purpose
AFormation* FormationOwner	Pointer to the owning formation actor
FAgentData* AgentData	Pointer to agent data structure from the formation asset (relative position, priority, group info, etc.)
FVector TargetLocation	Current world-space target location for the agent
bool bStray	Whether the agent is currently outside the formation
UFormationAsset
A data asset class defining information about a formation, including slot positions, priorities, and group info. Editable via the Formation Editor.

Member Variables
Name	Purpose
FString FormationName	Name for identification in editor/UI
float FormationRadius	Circumscribed circle radius for the formation (auto-calculated)
float FormationMinRadius	Minimum radius required to maintain the formation shape
FVector2D FormationCenter	2D center of the formation (excluding Z)
TArray<FAgentData> AgentDatas	Array of all agent slots in the formation
TArray<FName> GroupNames	List of group names available for slot assignment
FAgentData
Struct used for each agent in the formation asset.

Name	Purpose
FVector Position	Relative position of the agent (slot) in the formation
FRotator Rotation	Rotation of the agent (slot) in the formation
int32 Priority	Slot priority (lower values filled first)
FName GroupName	Group name for the slot (for group-based assignment)
Blueprint Callable Functions
cpp
// Formation.h
UFUNCTION(BlueprintCallable, Category = "Formation")
void FormationMoveTo(const FVector& Location, const FRotator& Rotation);
Moves the formation to a target location and rotation.

cpp
UFUNCTION(BlueprintCallable, Category = "Formation")
void RegisterAgent(UFormationAgentComponent* AgentComponent);
Registers an agent to the formation.

cpp
UFUNCTION(BlueprintCallable, Category = "Formation")
void RearrangeFormation();
Rearranges the formation according to the current formation asset.

cpp
UFUNCTION(BlueprintCallable, Category = "Formation")
void FallOutFormation();
Temporarily disbands the formation.

cpp
UFUNCTION(BlueprintCallable, Category = "Formation")
void FallInFormation();
Reforms the formation after being disbanded.

cpp
UFUNCTION(BlueprintCallable, Category = "Formation")
void ResizeRefFormationAsset();
Resizes the reference formation asset.

UFormationAgentComponent Blueprint/Native Functions
AFormation* GetFormationOwner() const
Returns a pointer to the owning formation actor.

void SetFormationOwner(AFormation* InFormationOwner)
Sets the owning formation actor for this component.

Feel free to use this documentation in your project's README.md to describe the Formation System's architecture and API.