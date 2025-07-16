/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "PathModifierTypes.generated.h"

USTRUCT(BlueprintType)
struct FPathModifierFlags
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Modifier Flags", meta = (ToolTip = "Whether to apply a positional offset that considers the formation radius to the path points."))
    bool bApplyOffset = true;

    UPROPERTY(EditAnywhere, Category = "Modifier Flags", meta = (ToolTip = "Whether to apply smoothing algorithms to the path."))
    bool bApplySmoothing = true;

    UPROPERTY(EditAnywhere, Category = "Modifier Flags", meta = (ToolTip = "Whether to draw debug visuals for the path in the editor."))
    bool bDrawDebug = true;
};

USTRUCT(BlueprintType)
struct FPathSplineConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Path Smoothing Settings", meta = (ToolTip = "Controls the curvature intensity for path smoothing. Higher values result in smoother, more curved paths."))
    float Curvature = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Path Smoothing Settings", meta = (ToolTip = "Distance threshold for merging nearby path points."))
    float MergeThreshold = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Path Smoothing Settings", meta = (ToolTip = "Number of subdivisions used when generating the smoothed spline path."))
    int32 Subdivisions = 10;
};

USTRUCT(BlueprintType)
struct FPathModifierConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Formation", meta = (ToolTip = "Flags to control which path modifications are applied (offset, smoothing, debug drawing)."))
    FPathModifierFlags ModifierFlags;

    UPROPERTY(EditAnywhere, Category = "Formation", meta = (ToolTip = "Configuration parameters for spline-based path smoothing."))
    FPathSplineConfig PathSplineConfig;
};
