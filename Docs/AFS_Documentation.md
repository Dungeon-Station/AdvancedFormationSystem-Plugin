# **Advanced Formation System Documentation**

## **Overview**

This document describes the core classes and components of the Advanced Formation System for Unreal Engine, including their purposes and member variables. Use this as a reference for integration, extension, or understanding the internal structure.

## Index
- [Quick Start Guide](#quick-start-guide)
- [Formation Editor Guide](#formation-editor-guide)
- [Key Features](#key-features)
- [Installation and Setup](#installation-and-setup)
- [Technical Reference](#technical-reference)
  - [Actors and Components](#actors-and-components)
    - [AFormation](#aformation)
      - [Member Variables](#member-variables)
        - [Rearrange Mode](#rearrange-mode)
    - [UFormationAgentComponent](#uformationagentcomponent)
    - [UFormationAsset](#uformationasset)
    - [FAgentData](#fagentdata)
    - [FPathModifierConfig](#fpathmodifierconfig)
    - [FPathModifierFlags](#fpathmodifierflags)
    - [FPathSplineConfig](#fpathsplineconfig)
- [Blueprint API](#blueprint-api)
  - [AFormation Blueprint/Native Functions](#aformation-blueprintnative-functions)
  - [UFormationAgentComponent Blueprint/Native Functions](#uformationagentcomponent-blueprintnative-functions)


## **Quick Start Guide**

<a href="https://docs.google.com/document/d/1POnYppEOzGx3zGe-9xcjTadZXVfd7frvdmD3qeWJs-k/edit?tab=t.0" target="_blank">Click here to see Quick Start Guide</a>

## **Formation Editor Guide**

<a href="https://docs.google.com/document/d/1hzuTeGKZg9SqCWtk9LeSFj6omMIiIJ5438lF2eN6flg/edit?tab=t.0" target="_blank">Click here to see Formation Editor Guide</a>

## **Key Features**

### **Intuitive Custom Formation Editor**
    
Our powerful, user-friendly editor gives you complete freedom to design, save, and modify any formation pattern you can imagine. Create unique arrangements perfectly tailored to your game's factions.
    
### **Formation Movement & Obstacle Avoidance**
    
Achieve seamless group movement. Units will maintain their position within the formation while navigating around obstacles and complex terrain. The system ensures that your groups move as a disciplined unit, not as a chaotic cluster.

### **Dynamic Rearrangement and Transformation**
    
Formations can dynamically adjust to changing conditions, such as losing units or merging with other groups. You can also seamlessly transform one formation into another formation.

### **Navmesh-Based Dynamic Path Generation**
    
The system dynamically generates an optimal travel path for the entire formation using your existing Navmesh. This ensures smooth, intelligent navigation across any environment without requiring complex manual setup.


## **Installation and Setup**

Blank


## **Technical Reference**

### **Actors and Components**

#### **AFormation**

Represents a formation entity that manages characters with UFormationAgentComponent. Responsible for formation creation, disbanding, movement, transformation, and management of its agents.

- **Components of AFormation**
    - **Formation Center (USceneComponent)**: Serves as the reference point for the formation. Agents' positions are calculated relative to this center.

    - **Formation Collision (USphereComponent)**: Defines the size of the formation. Used for path correction and transformation checks. The radius is determined by the FFormationWelzl class, considering the largest agent in the formation.

    - **Extend Formation Collision (USphereComponent)**: An expanded collision component used to determine if the formation can be expanded after being contracted.

    - **Move Component (UFloatingPawnMovement)**: Handles the movement of the AFormation actor.

- **Main Features**
    - **Formation Creation**: Reads data from UFormationAsset to create the formation. Uses FFormationHungarian for optimal arrangement.

    - **Formation Movement**: Generates a path using Unreal Engine's Navigation system, then modifies it (using FFormationPathModifier) to account for the formation's size. Continuously updates the path and commands agents to move accordingly.

    - **Formation Transformation**: Adjusts the collision size and agent offsets when shrinking or expanding the formation.

    - **Formation Disband/Reformation**: Can temporarily disband and later reform the formation, especially when the formation becomes too small.

- **Member Variables**

| Name                                   | Purpose                                             |
|:-----------------------------------------|:-----------------------------------------------------|
| **`EFormationPhase`**                        | Current state (Idle, Rotating, Moving)              |
| **`USphereComponent* SphereComponent`**      | Collision defining formation size                   |
| **`USphereComponent* ExtendSphereComponent`** | Collision for expansion checks                      |
| **`UFloatingPawnMovement* MoveComponent`**   | Controls formation movement                         |
| **`TArray<UFormationAgentComponent*> FormationAgentComponents`** | Agent components currently in the formation         |
| **`TArray<UFormationAgentComponent*> PreviousFormationAgentComponents`** | Agent components from previous tick          |
| **`UFormationAsset* RefFormationAsset`**     | Reference to the original formation data            |
| **`UFormationAsset* FormationAsset`**        | Copy of the formation data (mutable during transformation) |
| **`EFormationRearrangeMode RearrangeMode`**  | Rearrangement mode for the formation                |
| **`bool bIsFormationMoveStart`**             | Whether the formation has started moving            |
| **`FVector TargetLocation`**                 | Target location for movement                        |
| **`FRotator TargetRotation`**                | Target rotation for movement                        |
| **`float FormationTurnThreshold`**           | Total rotation amount when turning                  |
| **`float FormationTurnSpeed`**               | Speed of rotation                                   |
| **`float FormationSpeed`**                   | Formation movement speed                            |
| **`float AgentSpeed`**                       | Common speed for agents within the formation        |
| **`float StrayAgentSpeed`**                  | Speed for agents rejoining the formation            |
| **`float AgentAcceleration`**                | Acceleration for agents                             |
| **`TArray<FVector> PathPoints`**             | Path points for formation movement                  |
| **`int32 CurrentPathIndex`**                 | Current index in the path                           |
| **`FPathModifierConfig ModifierConfig`**     | Path modification settings                          |
| **`float ResizeIntensity`**                  | Intensity of formation transformation               |
| **`int32 CorrectPathNum`**                   | Number of path points to correct on collision       |
| **`float CorrectPathIntensity`**             | Intensity of path correction                        |
| **`bool bBroken`**                           | Whether the formation is disbanded                  |
| **`bool bDrawDebug`**                        | Whether to draw debug elements                      |

##### **Rearrange Mode**

OptimalMovement: Selects the best possible movement for each agent to achieve the new formation.

MaintainSlot: Agents remain in their original slots.

ForcedRotation: Agents rotate to face a new direction first, then moves to target location.


#### **UFormationAgentComponent**

A component added to actors to make them members of an AFormation. It enables centralized management and movement control as part of the formation.

- **Main Features**
    - **Formation Subscribe**: Associates the actor with a specific AFormation and subjects it to formation control.

    - **Target Location Updates**: Continuously receives and updates the world-space target location from its AFormation.

    - **Avoidance Interaction**: Implements avoidance logic using Unreal Engine's RVO and custom behaviors via ICrowdAgentInterface.

- **Member Variables**

| Name                           | Purpose                                                                                   |
|:-------------------------------|:------------------------------------------------------------------------------------------|
| **`AFormation* FormationOwner`**   | Pointer to the owning formation actor                                                     |
| **`FAgentData* AgentData`**        | Pointer to agent data structure from the formation asset (relative position, priority, group info, etc.) |
| **`FVector TargetLocation`**       | Current world-space target location for the agent                                         |
| **`bool bStray`**                  | Whether the agent is currently outside the formation                                      |


#### **UFormationAsset**
A data asset class defining information about a formation, including slot positions, priorities, and group info. Editable via the Formation Editor.

- **Member Variables**

| Name                              | Purpose                                                                                 |
|:-----------------------------------|:----------------------------------------------------------------------------------------|
| **`FString FormationName`**        | Name for identification in editor/UI                                                    |
| **`float FormationRadius`**        | Circumscribed circle radius for the formation (auto-calculated)                         |
| **`float FormationMinRadius`**     | Minimum radius required to maintain the formation shape                                 |
| **`FVector2D FormationCenter`**    | 2D center of the formation (excluding Z)                                                |
| **`TArray<FAgentData> AgentDatas`**| Array of all agent slots in the formation                                               |
| **`TArray<FName> GroupNames`**     | List of group names available for slot assignment                                       |


#### **FAgentData**
Struct used for each agent in the formation asset.

| Name                       | Purpose                                                        |
|:---------------------------|:---------------------------------------------------------------|
| **`FVector Position`**      | Relative position of the agent (slot) in the formation         |
| **`FRotator Rotation`**     | Rotation of the agent (slot) in the formation                  |
| **`int32 Priority`**        | Slot priority (lower values filled first)                      |
| **`FName GroupName`**       | Group name for the slot (for group-based assignment)           |


#### **FPathModifierConfig**

Struct aggregating path modification flags and spline configuration.

| Name                | Type                | Purpose                                                                                          |
|:--------------------|:--------------------|:-------------------------------------------------------------------------------------------------|
| `ModifierFlags`     | FPathModifierFlags  | Flags to control which path modifications are applied (offset, smoothing).         |
| `PathSplineConfig`  | FPathSplineConfig   | Configuration parameters for spline-based path smoothing.                                         |


#### **FPathModifierFlags**

Struct controlling which path modification features are enabled.

| Name               | Type   | Purpose                                                                                       |
|:-------------------|:-------|:---------------------------------------------------------------------------------------------|
| **`bApplyOffset`**     | bool   | Whether to apply a positional offset that considers the formation radius to the path points.  |
| **`bApplySmoothing`**  | bool   | Whether to apply smoothing algorithms to the path.                                            |
| **`bDrawDebug`**       | bool   | Whether to draw debug visuals for the path in the editor.                                     |


#### **FPathSplineConfig**

Struct containing configuration parameters for spline-based path smoothing.

| Name            | Type    | Purpose                                                                                       |
|:----------------|:--------|:---------------------------------------------------------------------------------------------|
| **`Curvature`**     | float   | Controls the curvature intensity for path smoothing. Higher values result in smoother, more curved paths. |
| **`MergeThreshold`**| float   | Distance threshold for merging nearby path points.                                            |
| **`Subdivisions`**  | int32   | Number of subdivisions used when generating the smoothed spline path.                         |

- If the Curvature value is set too high, the generated path may extend outside the Navmesh, especially in complex terrain.
- The MergeThreshold parameter can help reduce tangled paths in environments with many sharp turns. However, if this value is set too large, segments that should remain separate may be merged, resulting in unintended path shapes.


### **Blueprint API**

#### **AFormation Blueprint/Native Functions**

| Return Type | Function            | Parameters                                             | Description                                             |
|:-----------|:--------------------|:------------------------------------------------------|:--------------------------------------------------------|
| `void`     | **`FormationMoveTo`**   | `const FVector& Location, const FRotator& Rotation`   | Moves the formation to a target location and rotation.  |
| `void`     | **`RegisterAgent`**     | `UFormationAgentComponent* AgentComponent`            | Registers an agent to the formation.                    |
| `void`     | **`RearrangeFormation`**|                                                      | Rearranges the formation according to the current formation asset. |
| `void`     | **`FallOutFormation`**  |                                                      | Temporarily disbands the formation.                     |
| `void`     | **`FallInFormation`**   |                                                      | Reforms the formation after being disbanded.            |
| `void`     | **`ResizeRefFormationAsset`** |                                                | Resizes the reference formation asset.                  |


#### **UFormationAgentComponent Blueprint/Native Functions**

| Return Type   | Function              | Parameters                        | Description                                         |
|:-------------|:----------------------|:-----------------------------------|:----------------------------------------------------|
| `AFormation*` | **`GetFormationOwner`**   |                        | Returns a pointer to the owning formation actor.     |
| `void`        | **`SetFormationOwner`**   | `AFormation* InFormationOwner`     | Sets the owning formation actor for this component.  |
