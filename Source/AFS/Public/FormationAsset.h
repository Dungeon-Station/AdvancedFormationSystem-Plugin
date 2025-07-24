/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Delegates/Delegate.h"
#include "FormationAsset.generated.h"
//DECLARE_MULTICAST_DELEGATE(OnAgentDatasChanged);

DECLARE_MULTICAST_DELEGATE(FOnAgentCountChanged);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAgentsDataChanged, const TArray<int32>& /*AgentIndex*/);

DECLARE_MULTICAST_DELEGATE(FOnGroupPresetsChanged);

USTRUCT(BlueprintType)
struct FAgentData
{
	GENERATED_BODY()

	/** Relative position offset from the formation's center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data", meta = (ToolTip = "Relative position offset from the formation's center."))
	FVector Position;

	/** Relative rotation from the formation's forward direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data", meta = (ToolTip = "Relative rotation from the formation's forward direction."))
	FRotator Rotation;
	
	/** Priority for slot assignment (lower value is higher priority). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data", meta = (ToolTip = "Priority for slot assignment (lower value is higher priority)."))
	int32 Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data", meta = (GetOptions = "GetGroupNameOptions", ToolTip = "The name of the agent slot."))
	FName GroupName = FName("Default");

	bool operator==(const FAgentData& Other) const
	{
		return (Position == Other.Position) && (Rotation == Other.Rotation) && (Priority == Other.Priority) && (GroupName == Other.GroupName);
	}
};

USTRUCT(BlueprintType)
struct FGroupUnitPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data")
	FName GroupName = FName("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Data")
	TSubclassOf<APawn> UnitPreset;
};
/**
 * A Data Asset that defines the shape and properties of a formation.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Formation Asset", meta = (AssetColor = "1.0,1.0,0.0"))
class AFS_API UFormationAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UFormationAsset();

	void ConvertToFormationAgentDatas();
    
	/** The display name for identifying this formation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(DisplayName="Formation Name", ToolTip="The display name for identifying this formation."))
	FString FormationName = "Default Formation";

	/** The overall radius of the formation, used for collision checks. */
	UPROPERTY(BlueprintReadWrite, Category = "Formation", meta=(ToolTip="The overall radius of the formation, used for collision checks."))
	float FormationRadius = 1000.0f;

	/** The minimum radius required to maintain the formation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", DisplayName = "Formation Minimum Radius", meta = (ToolTip = "The minimum radius required to maintain the formation."))
	float FormationMinRadius = 300.0f;

	/** The 2D offset of the formation's geometric center. */
	FVector2D FormationCenter;

    /** The list of all agent slots that make up this formation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Agents", meta=(DisplayName="Agent Data", ToolTip="The list of all agent slots that make up this formation."))
    TArray<FAgentData> AgentDatas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agents", meta = (DisplayName = "Formation Agent Data", ToolTip = "The list of all agent slots that make up this formation."))
	TArray<FAgentData> FormationAgentDatas;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(DisplayName="Group Unit Presets"))
	TArray<FGroupUnitPreset> GroupUnitPresets;
	
	UFUNCTION(BlueprintCallable, Category = "Formation")
	TSubclassOf<APawn> GetUnitPresetForGroup(FName GroupName) const;
	
	UFUNCTION()
	TArray<FString> GetGroupNameOptions() const;

public:
	FOnAgentCountChanged OnAgentCountChanged;
	FOnAgentsDataChanged OnAgentsDataChanged;
	FOnGroupPresetsChanged OnGroupPresetsChanged;
	
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& ChainEvent) override;
#endif
	
	bool bIsUpdatingFromDataChange = false;
private:
#if WITH_EDITORONLY_DATA
	TArray<FGroupUnitPreset> CachedGroupUnitPresets;

	TArray<FAgentData> CachedAgentDatas;
#endif

	TSet<int32> PendingChangedAgentIndices;
};

