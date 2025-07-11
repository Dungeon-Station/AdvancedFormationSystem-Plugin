#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PathModifierTypes.h"
#include "FormationPathModifier.generated.h"

class UWorld;
class UFormationAsset;

UCLASS()
class AFS_API UFormationPathModifier : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Formation|PathModifier")
    static TArray<FVector> ApplyPathCorrection(
        UWorld* WorldContextObject,
        const TArray<FVector>& RawPath,
        UFormationAsset* FormationAsset,
        FPathModifierConfig ModifierConfig
    );

    UFUNCTION(BlueprintCallable, Category="Formation|PathModifier")
    static TArray<FVector> ApplyOffset(
        UWorld* WorldContextObject,
        const TArray<FVector>& Path,
        float OffsetDistance,
        float TraceRadius,
        FPathModifierConfig ModifierConfig
    );

    UFUNCTION(BlueprintCallable, Category="Formation|PathModifier")
    static TArray<FVector> ApplySmoothing(
        const TArray<FVector>& Path,
        FPathModifierConfig ModifierConfig
    );

    UFUNCTION(BlueprintCallable, Category="Formation|PathModifier")
    static TArray<FVector> StraightenPath(
        UWorld* WorldContextObject,
        const TArray<FVector>& RawPath
    );
};
