// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FCircle
{
    FVector2D Center;
    float Radius;

    FCircle() : Center(FVector2D::ZeroVector), Radius(0.f) {}
    FCircle(const FVector2D& InCenter, float InRadius) : Center(InCenter), Radius(InRadius) {}
};

class AFS_API FFormationWelzl
{
public:
    FFormationWelzl();
    ~FFormationWelzl();

public:
    static FCircle GetMinimumEnclosingCircle(const TArray<FVector2D>& Points);

private:
    static FCircle WelzlRecursive(TArray<FVector2D> P, TArray<FVector2D> R);
    static FCircle MakeCircleFromBoundary(const TArray<FVector2D>& R);
    static FCircle MakeCircleFromThreePoints(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3);
};