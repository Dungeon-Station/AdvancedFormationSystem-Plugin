// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FormationAsset.h"
#include "AgentDataWrapper.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAgentDataWrapperChanged, int32 /* Agent Index */, EPropertyChangeType::Type /* ChangeType */);

UCLASS()
class AFS_API UAgentDataWrapper : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Agent Data")
	FAgentData AgentData;

	UPROPERTY()
	UFormationAsset* OwnerAsset;

	UPROPERTY()
	int32 AgentIndex;

	bool bIsMultiSelected = false;

	FOnAgentDataWrapperChanged OnChanged;

	UFUNCTION()
	TArray<FString> GetGroupNameOptions() const;

	void SetAgentData(UFormationAsset* InOwnerAsset, int32 InAgentIndex);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual bool CanEditChange(const FProperty* InProperty) const override;
};
