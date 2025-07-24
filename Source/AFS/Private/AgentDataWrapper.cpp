// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentDataWrapper.h"

TArray<FString> UAgentDataWrapper::GetGroupNameOptions() const
{
	if (OwnerAsset)
	{
		return OwnerAsset->GetGroupNameOptions();
	}
	return TArray<FString>();
}

void UAgentDataWrapper::SetAgentData(UFormationAsset* InOwnerAsset, int32 InAgentIndex)
{
	OwnerAsset = InOwnerAsset;
	AgentIndex = InAgentIndex;

	if (OwnerAsset && OwnerAsset->AgentDatas.IsValidIndex(AgentIndex))
	{
		AgentData = OwnerAsset->AgentDatas[AgentIndex];
	}
}

void UAgentDataWrapper::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (OwnerAsset && OwnerAsset->AgentDatas.IsValidIndex(AgentIndex))
	{
		OwnerAsset->Modify();
		OwnerAsset->AgentDatas[AgentIndex] = AgentData;
		OnChanged.Broadcast(AgentIndex, PropertyChangedEvent.ChangeType);
	}
}

bool UAgentDataWrapper::CanEditChange(const FProperty* InProperty) const
{
	const bool bParentCanEdit = Super::CanEditChange(InProperty);

	if (!bParentCanEdit)
	{
		return false;
	}

	if (!bIsMultiSelected)
	{
		return true;
	}

	if (InProperty)
	{

		if (InProperty->GetName() == TEXT("AgentData"))
		{
			return true;
		}
		const FName PropertyName = InProperty->GetFName();

		if (InProperty->GetOwnerStruct() == FAgentData::StaticStruct())
		{
			if (PropertyName == GET_MEMBER_NAME_CHECKED(FAgentData, Priority) ||
				PropertyName == GET_MEMBER_NAME_CHECKED(FAgentData, GroupName))
			{
				return true;
			}
		}
	}
	return false;
}
