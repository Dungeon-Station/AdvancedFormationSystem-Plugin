/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

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

	static void Solve(const TArray<TArray<float>>& CostMatrix, TArray<int32>& OutU2V, TArray<int32>& OutV2U);
};
