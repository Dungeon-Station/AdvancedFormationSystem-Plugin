/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "FormationWelzl.h"

FFormationWelzl::FFormationWelzl()
{
}

FFormationWelzl::~FFormationWelzl()
{
}

// Main entry point for computing the Minimum Enclosing Circle
FCircle FFormationWelzl::GetMinimumEnclosingCircle(const TArray<FVector2D>& Points)
{
    // If there are no points, return a default circle (center at (0,0), radius 0)
    if (Points.Num() == 0)
    {
        return FCircle();
    }

    // For expected linear time complexity, shuffle the points randomly (Fisher-Yates shuffle)
    TArray<FVector2D> ShuffledPoints = Points;
    const int32 NumPoints = ShuffledPoints.Num();
    for (int32 i = NumPoints - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        ShuffledPoints.Swap(i, j);
    }

    // Start the recursive Welzl algorithm (boundary set R starts empty)
    return WelzlRecursive(ShuffledPoints, {});
}

// Recursive implementation of Welzl's algorithm
FCircle FFormationWelzl::WelzlRecursive(TArray<FVector2D> P, TArray<FVector2D> R)
{
    // Base case: no more points to process or boundary set has 3 points
    if (P.Num() == 0 || R.Num() == 3)
    {
        return MakeCircleFromBoundary(R);
    }

    // Remove one point p from P
    const FVector2D p = P.Pop();

    // Compute the MEC for the remaining points
    FCircle mec = WelzlRecursive(P, R);

    // If p is inside the current MEC, return it directly
    // Use DistSquared to avoid unnecessary square root operations
    if (FVector2D::DistSquared(mec.Center, p) < FMath::Square(mec.Radius) + KINDA_SMALL_NUMBER)
    {
        return mec;
    }

    // Otherwise, p must be on the boundary of the new MEC
    R.Add(p);
    return WelzlRecursive(P, R);
}

// Helper: construct the circle defined by boundary points R
FCircle FFormationWelzl::MakeCircleFromBoundary(const TArray<FVector2D>& R)
{
    const int32 NumPoints = R.Num();
    // No boundary points: default circle
    if (NumPoints == 0)
    {
        return FCircle();
    }
    // Single boundary point: circle of radius zero at that point
    if (NumPoints == 1)
    {
        return FCircle(R[0], 0.f);
    }
    // Two boundary points: circle with these points as diameter ends
    if (NumPoints == 2)
    {
        FVector2D Center = (R[0] + R[1]) / 2.0f;
        float Radius = FVector2D::Distance(R[0], R[1]) / 2.0f;
        return FCircle(Center, Radius);
    }
    // Three boundary points: compute the circumcircle
    if (NumPoints == 3)
    {
        return MakeCircleFromThreePoints(R[0], R[1], R[2]);
    }
    // In this algorithm, boundary never exceeds 3 points
    return FCircle();
}

// Helper: compute the circumcircle of three points
FCircle FFormationWelzl::MakeCircleFromThreePoints(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3)
{
    // Compute the determinant D to check for colinearity
    const float D = 2.0f * (P1.X * (P2.Y - P3.Y) + P2.X * (P3.Y - P1.Y) + P3.X * (P1.Y - P2.Y));

    // If D is nearly zero, points are considered colinear
    if (FMath::Abs(D) < KINDA_SMALL_NUMBER)
    {
        // For colinear points, the MEC is defined by the two farthest points
        const float d12 = FVector2D::DistSquared(P1, P2);
        const float d13 = FVector2D::DistSquared(P1, P3);
        const float d23 = FVector2D::DistSquared(P2, P3);

        if (d12 >= d13 && d12 >= d23)
            return FCircle((P1 + P2) / 2.0f, FVector2D::Distance(P1, P2) / 2.0f);
        if (d13 >= d12 && d13 >= d23)
            return FCircle((P1 + P3) / 2.0f, FVector2D::Distance(P1, P3) / 2.0f);

        return FCircle((P2 + P3) / 2.0f, FVector2D::Distance(P2, P3) / 2.0f);
    }

    // Otherwise, compute the circumcenter coordinates
    const float P1_sq = P1.SizeSquared();
    const float P2_sq = P2.SizeSquared();
    const float P3_sq = P3.SizeSquared();

    const float CenterX = (P1_sq * (P2.Y - P3.Y) + P2_sq * (P3.Y - P1.Y) + P3_sq * (P1.Y - P2.Y)) / D;
    const float CenterY = (P1_sq * (P3.X - P2.X) + P2_sq * (P1.X - P3.X) + P3_sq * (P2.X - P1.X)) / D;

    FVector2D Center(CenterX, CenterY);
    float Radius = FVector2D::Distance(Center, P1);

    return FCircle(Center, Radius);
}

FSphere FFormationWelzl::GetMinimumEnclosingSphere(const TArray<FVector>& Points)
{
    if (Points.Num() == 0)
    {
        return FSphere();
    }

    TArray<FVector> ShuffledPoints = Points;
    const int32 NumPoints = ShuffledPoints.Num();
    for (int32 i = NumPoints - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        ShuffledPoints.Swap(i, j);
    }

    return WelzlRecursive3D(ShuffledPoints, {});
}

FSphere FFormationWelzl::WelzlRecursive3D(TArray<FVector> P, TArray<FVector> R)
{
    if (P.Num() == 0 || R.Num() == 4)
    {
        return MakeSphereFromBoundary(R);
    }

    const FVector p = P.Pop();
    FSphere mes = WelzlRecursive3D(P, R);

    if (FVector::DistSquared(mes.Center, p) < FMath::Square(mes.W) + KINDA_SMALL_NUMBER)
    {
        return mes;
    }

    R.Add(p);
    return WelzlRecursive3D(P, R);
}

FSphere FFormationWelzl::MakeSphereFromBoundary(const TArray<FVector>& R)
{
    switch (R.Num())
    {
        case 0: return FSphere();
        case 1: return FSphere(R[0], 0.f);
        case 2: return MakeSphereFromTwoPoints(R[0], R[1]);
        case 3: return MakeSphereFromThreePoints(R[0], R[1], R[2]);
        case 4: return MakeSphereFromFourPoints(R[0], R[1], R[2], R[3]);
    }
    return FSphere();
}

FSphere FFormationWelzl::MakeSphereFromTwoPoints(const FVector& P1, const FVector& P2)
{
    FVector Center = (P1 + P2) / 2.0f;
    float Radius = FVector::Distance(P1, P2) / 2.0f;
    return FSphere(Center, Radius);
}

FSphere FFormationWelzl::MakeSphereFromThreePoints(const FVector& P1, const FVector& P2, const FVector& P3)
{
    // Check for obtuse/right triangles. If found, the sphere is defined by the longest edge.
    if (FVector::DotProduct(P2 - P1, P3 - P1) <= 0) return MakeSphereFromTwoPoints(P2, P3);
    if (FVector::DotProduct(P1 - P2, P3 - P2) <= 0) return MakeSphereFromTwoPoints(P1, P3);
    if (FVector::DotProduct(P1 - P3, P2 - P3) <= 0) return MakeSphereFromTwoPoints(P1, P2);

    const FVector a = P2 - P1;
    const FVector b = P3 - P1;
    const FVector aCrossB = FVector::CrossProduct(a, b);

    // ★★★ FIX APPLIED HERE ★★★
    // If points are collinear, find the sphere from the two most distant points.
    if (aCrossB.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        const float d12 = FVector::DistSquared(P1, P2);
        const float d13 = FVector::DistSquared(P1, P3);
        const float d23 = FVector::DistSquared(P2, P3);

        if (d12 >= d13 && d12 >= d23)
            return MakeSphereFromTwoPoints(P1, P2);
        if (d13 >= d12 && d13 >= d23)
            return MakeSphereFromTwoPoints(P1, P3);
        
        return MakeSphereFromTwoPoints(P2, P3);
    }
    
    // For an acute triangle, calculate the circumcenter in 3D space.
    const FVector Top = (FVector::CrossProduct(aCrossB, a) * b.SizeSquared() + FVector::CrossProduct(b, aCrossB) * a.SizeSquared());
    const FVector Center = P1 + Top / (2.f * aCrossB.SizeSquared());
    const float Radius = FVector::Distance(Center, P1);
    return FSphere(Center, Radius);
}

FSphere FFormationWelzl::MakeSphereFromFourPoints(const FVector& P1, const FVector& P2, const FVector& P3, const FVector& P4)
{
    const FVector a = P2 - P1;
    const FVector b = P3 - P1;
    const FVector c = P4 - P1;
    const float Denom = 2.0f * FVector::DotProduct(a, FVector::CrossProduct(b, c));

    if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER)
    {
        FSphere s1 = MakeSphereFromThreePoints(P1, P2, P3);
        FSphere s2 = MakeSphereFromThreePoints(P1, P2, P4);
        FSphere s3 = MakeSphereFromThreePoints(P1, P3, P4);
        FSphere s4 = MakeSphereFromThreePoints(P2, P3, P4);
        if (s1.W < s2.W && s1.W < s3.W && s1.W < s4.W) return s1;
        if (s2.W < s3.W && s2.W < s4.W) return s2;
        if (s3.W < s4.W) return s3;
        return s4;
    }

    const FVector o = (FVector::CrossProduct(b, c) * a.SizeSquared() +
                 FVector::CrossProduct(c, a) * b.SizeSquared() +
                 FVector::CrossProduct(a, b) * c.SizeSquared()) / Denom;
    const FVector Center = P1 + o;
    const float Radius = o.Size();
    return FSphere(Center, Radius);
}