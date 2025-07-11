#include "FormationAsset.h"

UFormationAsset::UFormationAsset()
{
    UE_LOG(LogTemp, Warning, TEXT("FormationAsset Created!"));
	GroupNames.Add(FName("Default"));
}

#if WITH_EDITOR
TArray<FString> UFormationAsset::GetGroupNameOptions() const
{
    TArray<FString> Options;

    for (const FName& GroupName : GroupNames)
    {
        if(!GroupName.IsNone())
        {
            Options.Add(GroupName.ToString());
		}
    }
    return Options;
}
void UFormationAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UObject::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.GetPropertyName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, AgentDatas) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, UnitActorPreset))
        {
            OnAgentPositionsChanged.Broadcast();
        }
        
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UFormationAsset, AgentDatas))
        {
            if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd ||
                PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
            {
                const int32 NewIndex = PropertyChangedEvent.GetArrayIndex(PropertyName.ToString());
                
                if (AgentDatas.IsValidIndex(NewIndex))
                {
                    AgentDatas[NewIndex].Priority = AgentDatas.Num() - 1;
                }
            }
        }
    }
}
#endif