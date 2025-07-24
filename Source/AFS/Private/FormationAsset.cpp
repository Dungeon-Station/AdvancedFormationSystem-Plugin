/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "FormationAsset.h"
#include "FormationWelzl.h"


UFormationAsset::UFormationAsset()
{
    GroupUnitPresets.Add({FName("Default"), nullptr}); 
}

void UFormationAsset::ConvertToFormationAgentDatas()
{
	FormationAgentDatas.Empty();

    TArray<FVector2D> AgentPositions2D;
    TArray<FVector> AgentPositions;
    float MaxRadius = 0.0f;

    for (FAgentData AgentData : AgentDatas)
    {
        FVector Position = AgentData.Position;
        AgentPositions2D.Add(FVector2D(Position.X, Position.Y));
        AgentPositions.Add(Position);
    }

    FCircle Circle = FFormationWelzl::GetMinimumEnclosingCircle(AgentPositions2D);
    FSphere Sphere = FFormationWelzl::GetMinimumEnclosingSphere(AgentPositions);

    for (FAgentData Agents : AgentDatas)
    {
        FAgentData NewAgentData;
		NewAgentData.GroupName = Agents.GroupName;
		NewAgentData.Priority = Agents.Priority;
		NewAgentData.Position = Agents.Position - Sphere.Center;
		NewAgentData.Rotation = Agents.Rotation;
		FormationAgentDatas.Add(NewAgentData);
    }
}

TSubclassOf<APawn> UFormationAsset::GetUnitPresetForGroup(FName GroupName) const
{
    for (const FGroupUnitPreset& Preset : GroupUnitPresets)
    {
        if (Preset.GroupName == GroupName)
        {
            return Preset.UnitPreset;
        }
    }

    if (GroupUnitPresets.Num() > 0)
    {
        return GroupUnitPresets[0].UnitPreset;
    }
    return nullptr;
}

TArray<FString> UFormationAsset::GetGroupNameOptions() const
{
    TArray<FString> Options;

    for (auto Group : GroupUnitPresets)
    {
        Options.Add(Group.GroupName.ToString());
    }
    return Options;
}


#if WITH_EDITOR
void UFormationAsset::PreEditChange(FProperty* PropertyAboutToChange)
{
    Super::PreEditChange(PropertyAboutToChange);

    if (PropertyAboutToChange)
    {
        const FName PropertyName = PropertyAboutToChange->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, GroupUnitPresets))
        {
            CachedGroupUnitPresets = GroupUnitPresets;
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, AgentDatas) ||
            PropertyAboutToChange->GetOwnerStruct() == FAgentData::StaticStruct())
        {
            CachedAgentDatas = AgentDatas;
        }
    }
}

void UFormationAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    int32 DefaultIndex = GroupUnitPresets.IndexOfByPredicate([](const FGroupUnitPreset& Preset){ return Preset.GroupName == FName("Default"); });
    if (DefaultIndex == INDEX_NONE)
    {
        GroupUnitPresets.Insert({FName("Default"), nullptr}, 0);
    }
    else if (DefaultIndex > 0)
    {
        FGroupUnitPreset DefaultData = GroupUnitPresets[DefaultIndex];
        GroupUnitPresets.RemoveAt(DefaultIndex);
        GroupUnitPresets.Insert(DefaultData, 0);
    }

    if (!PropertyChangedEvent.MemberProperty)
    {
        return;
    }

    const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();

    if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, GroupUnitPresets))
    {
        if (CachedGroupUnitPresets.IsEmpty()) return;

        TSet<FName> OldGroupNames;
        for (const FGroupUnitPreset& Preset : CachedGroupUnitPresets) { OldGroupNames.Add(Preset.GroupName); }
        
        TSet<FName> NewGroupNames;
        for (const FGroupUnitPreset& Preset : GroupUnitPresets) { NewGroupNames.Add(Preset.GroupName); }

        TSet<FName> RemovedNames = OldGroupNames.Difference(NewGroupNames);
        TSet<FName> AddedNames = NewGroupNames.Difference(OldGroupNames);

        if (CachedGroupUnitPresets.Num() == GroupUnitPresets.Num() && RemovedNames.Num() == 1 && AddedNames.Num() == 1)
        {
            const FName OldName = *RemovedNames.begin();
            const FName NewName = *AddedNames.begin();
            for (FAgentData& AgentData : AgentDatas)
            {
                if (AgentData.GroupName == OldName)
                {
                    AgentData.GroupName = NewName;
                }
            }
        }
        else
        {
            for (FAgentData& AgentData : AgentDatas)
            {
                if (!NewGroupNames.Contains(AgentData.GroupName))
                {
                    AgentData.GroupName = FName("Default");
                }
            }
        }

        OnGroupPresetsChanged.Broadcast();
        CachedGroupUnitPresets.Empty();
    }
    
    if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, AgentDatas))
    {
        if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd ||
            PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
        {
            const int32 NewIndex = PropertyChangedEvent.GetArrayIndex(MemberPropertyName.ToString());
            if (AgentDatas.IsValidIndex(NewIndex))
            {
                AgentDatas[NewIndex].Priority = AgentDatas.Num() - 1;
            }

            OnAgentCountChanged.Broadcast();
        }
        else if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
        {
            if (CachedAgentDatas.Num() == AgentDatas.Num())
            {
                TArray<int32> ChangedIndices;
                for (int32 i = 0; i < AgentDatas.Num(); ++i)
                {
                    if (!(AgentDatas[i] == CachedAgentDatas[i]))
                    {
                        ChangedIndices.Add(i);
                    }
                }

                if (ChangedIndices.Num() > 0)
                {
                    OnAgentsDataChanged.Broadcast(ChangedIndices);
                }
            }
            else 
            {
                OnAgentCountChanged.Broadcast();
            }
        }

        CachedAgentDatas.Empty();
    }
}
void UFormationAsset::PostEditChangeChainProperty(FPropertyChangedChainEvent& ChainEvent)
{
    Super::PostEditChangeChainProperty(ChainEvent);
    TArray<int32> AllIndices;
    for (int32 i = 0; i < AgentDatas.Num(); ++i)
    {
        AllIndices.Add(i);
    }
    OnAgentsDataChanged.Broadcast(AllIndices);
}
#endif