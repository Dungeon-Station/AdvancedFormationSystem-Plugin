// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class AFS_API FFormationHungarian
{
public:
	FFormationHungarian();
	~FFormationHungarian();

	UFUNCTION()
	static void Solve(const TArray<TArray<float>>& CostMatrix, TArray<int32>& OutU2V, TArray<int32>& OutV2U);
};
