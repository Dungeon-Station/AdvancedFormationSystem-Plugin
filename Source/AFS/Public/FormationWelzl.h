/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Math/Sphere.h"

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
    static FSphere GetMinimumEnclosingSphere(const TArray<FVector>& Points);

private:
    static FCircle WelzlRecursive(TArray<FVector2D> P, TArray<FVector2D> R);
    static FCircle MakeCircleFromBoundary(const TArray<FVector2D>& R);
    static FCircle MakeCircleFromThreePoints(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3);

    // --- 3D Private Helpers ---
    static FSphere WelzlRecursive3D(TArray<FVector> P, TArray<FVector> R);
    static FSphere MakeSphereFromBoundary(const TArray<FVector>& R);
    static FSphere MakeSphereFromTwoPoints(const FVector& P1, const FVector& P2);
    static FSphere MakeSphereFromThreePoints(const FVector& P1, const FVector& P2, const FVector& P3);
    static FSphere MakeSphereFromFourPoints(const FVector& P1, const FVector& P2, const FVector& P3, const FVector& P4);
};

class FFormationWelzl3D
{
public:
    FFormationWelzl3D();
    ~FFormationWelzl3D();

    /**
     * Main entry point for computing the Minimum Enclosing Sphere.
     * @param Points An array of 3D points.
     * @return The minimum sphere that encloses all points.
     */
    static FSphere GetMinimumEnclosingSphere(const TArray<FVector>& Points);

private:
    /** Recursive implementation of Welzl's algorithm for 3D points. */
    static FSphere WelzlRecursive(TArray<FVector> P, TArray<FVector> R);

    /** Helper function to construct a sphere from a set of boundary points. */
    static FSphere MakeSphereFromBoundary(const TArray<FVector>& R);

    /** Helper to compute the minimum sphere from two points. */
    static FSphere MakeSphereFromTwoPoints(const FVector& P1, const FVector& P2);
    
    /** Helper to compute the minimum sphere from three points. */
    static FSphere MakeSphereFromThreePoints(const FVector& P1, const FVector& P2, const FVector& P3);
    
    /** Helper to compute the minimum sphere from four points (circumshpere of a tetrahedron). */
    static FSphere MakeSphereFromFourPoints(const FVector& P1, const FVector& P2, const FVector& P3, const FVector& P4);
};