#include "FormationPathModifier.h"
#include "FormationAsset.h"
#include "NavigationSystem.h"
#include "Components/SplineComponent.h"

TArray<FVector> UFormationPathModifier::ApplyPathCorrection(
    UWorld* WorldContext,
    const TArray<FVector>& RawPath,
    UFormationAsset* FormationAsset,
    FPathModifierConfig ModifierConfig
)
{
    if (RawPath.Num() < 2) return RawPath;

    TArray<FVector> CorrectedPath = RawPath;

    if (ModifierConfig.ModifierFlags.bApplyOffset)
    {
        CorrectedPath = StraightenPath(WorldContext, RawPath);
        CorrectedPath = ApplyOffset(
            WorldContext,
            CorrectedPath,
            FormationAsset->FormationRadius,
            FormationAsset->FormationRadius * 2,
            ModifierConfig
        );
    }

    if (ModifierConfig.ModifierFlags.bApplySmoothing)
    {
        CorrectedPath = ApplySmoothing(CorrectedPath, ModifierConfig);
    }

    return CorrectedPath;
}

TArray<FVector> UFormationPathModifier::ApplyOffset(
    UWorld* WorldContext,
    const TArray<FVector>& Path,
    float OffsetDistance,
    float TraceRadius,
    FPathModifierConfig ModifierConfig
)
{
    TArray<FVector> NewPath = Path;
    if (Path.Num() < 3) return NewPath;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(WorldContext);
    if (!NavSys) return Path;

    for (int32 i = 1; i < Path.Num() - 1; i++)
    {
        const FVector& PrevPoint = Path[i - 1];
        const FVector& CurrPoint = Path[i];
        const FVector& NextPoint = Path[i + 1];

        FVector V1 = (CurrPoint - PrevPoint).GetSafeNormal();
        FVector V2 = (CurrPoint - NextPoint).GetSafeNormal();
        FVector Tangent = (V1 + V2).GetSafeNormal();

        FVector LeftDir = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
        FVector RightDir = -LeftDir;
        FVector MidLeftDir = (Tangent + LeftDir).GetSafeNormal();
        FVector MidRightDir = (Tangent + RightDir).GetSafeNormal();

        FVector BestOffsetPoint = CurrPoint;
        float MaxFreeDistance = TraceRadius;

        TArray<FVector> CheckDirs = { Tangent, MidLeftDir, LeftDir, MidRightDir, RightDir };

        bool bHit = false;
        for (const FVector& Dir : CheckDirs)
        {
            float AngleRad = FMath::Acos(FVector::DotProduct(Tangent, Dir));
            float AngleRatio = AngleRad / (PI / 2);
            float DynamicTraceRadius = TraceRadius * (1.0f + AngleRatio * 0.5f);

            FVector AdjustedStart = CurrPoint + Dir * 10.0f;
            FVector TraceEnd = AdjustedStart + Dir * (TraceRadius);
            FVector HitLocation;

            bHit |= NavSys->NavigationRaycast(
                WorldContext,
                AdjustedStart,
                TraceEnd,
                HitLocation
            );

            float FreeDistance = OffsetDistance;
            if (bHit)
            {
                float HitDist = FVector::Dist(AdjustedStart, HitLocation);
                FreeDistance = HitDist;
            }
            if (FreeDistance < MaxFreeDistance)
            {
                MaxFreeDistance = FreeDistance;
                BestOffsetPoint = CurrPoint + Tangent * FreeDistance * 0.5f;
            }

#if ENABLE_DRAW_DEBUG
            if (WorldContext && ModifierConfig.ModifierFlags.bDrawDebug)
            {
                FColor RayColor = FColor::Blue;

                DrawDebugLine(WorldContext, AdjustedStart, TraceEnd, RayColor, false, 5.0f, 0, 2.0f);
                if (bHit) DrawDebugSphere(WorldContext, HitLocation, 15.0f, 8, FColor::Red, false, 5.0f);
            }
#endif
        }

        if (!bHit)
        {
            NewPath[i] = CurrPoint + Tangent * OffsetDistance;
        }
        else
        {
            NewPath[i] = BestOffsetPoint;
        }

#if ENABLE_DRAW_DEBUG
        if (WorldContext && ModifierConfig.ModifierFlags.bDrawDebug)
        {
            DrawDebugSphere(WorldContext, BestOffsetPoint, 25.0f, 12, FColor::Cyan, false, 5.0f);
            DrawDebugLine(WorldContext, CurrPoint, BestOffsetPoint, FColor::Yellow, false, 5.0f, 0, 3.0f);
        }
#endif
    }
    return NewPath;
}

TArray<FVector> UFormationPathModifier::ApplySmoothing(
    const TArray<FVector>& Path,
    FPathModifierConfig ModifierConfig
)
{
    if (Path.Num() < 2) return Path;

    const int32 Subdivisions = ModifierConfig.PathSplineConfig.Subdivisions;
    const float MergeThreshold = ModifierConfig.PathSplineConfig.MergeThreshold;
    const float Curvature = ModifierConfig.PathSplineConfig.Curvature;

    TArray<FVector> MergedPath;
    MergedPath.Add(Path[0]);

    for (int32 i = 1; i < Path.Num(); i++)
    {
        if (FVector::Dist(Path[i], MergedPath.Last()) > MergeThreshold)
        {
            MergedPath.Add(Path[i]);
        }
    }
    if (MergedPath.Num() < 2) return Path;

    USplineComponent* TempSplineComponent = NewObject<USplineComponent>();

    TempSplineComponent->ClearSplinePoints();

    for (int32 i = 0; i < MergedPath.Num(); i++)
    {
        FSplinePoint SplinePoint(i, MergedPath[i], ESplinePointType::CurveCustomTangent);

        if (i == 0 && MergedPath.Num() > 1)
        {
            FVector Dir = (MergedPath[i + 1] - MergedPath[i]).GetSafeNormal();
            float Dist = FVector::Dist(MergedPath[i], MergedPath[i + 1]);
            float TangentLength = FMath::Lerp(0.0f, Dist * 0.5f, Curvature);
            SplinePoint.LeaveTangent = Dir * TangentLength;
        }
        else if (i == MergedPath.Num() - 1 && MergedPath.Num() > 1)
        {
            FVector Dir = (MergedPath[i] - MergedPath[i - 1]).GetSafeNormal();
            float Dist = FVector::Dist(MergedPath[i], MergedPath[i - 1]);
            float TangentLength = FMath::Lerp(0.0f, Dist * 0.5f, Curvature);
            SplinePoint.ArriveTangent = Dir * TangentLength;
        }
        else if (MergedPath.Num() > 2)
        {
            FVector LinearDir = (MergedPath[i + 1] - MergedPath[i]).GetSafeNormal();
            FVector CurveDir = (MergedPath[i + 1] - MergedPath[i - 1]).GetSafeNormal();
            FVector FinalDir = FMath::Lerp(LinearDir, CurveDir, Curvature);

            float MaxTangentLength = FVector::Dist(MergedPath[i - 1], MergedPath[i + 1]) * 0.25f;
            float TangentLength = FMath::Lerp(0.0f, MaxTangentLength, Curvature);

            SplinePoint.ArriveTangent = FinalDir * TangentLength;
            SplinePoint.LeaveTangent = FinalDir * TangentLength;
        }

        TempSplineComponent->AddPoint(SplinePoint);
    }

    TempSplineComponent->UpdateSpline();
    const float SplineLength = TempSplineComponent->GetSplineLength();

    TArray<FVector> SmoothedPath;
    const int32 TotalSamples = FMath::Max(MergedPath.Num() * ModifierConfig.PathSplineConfig.Subdivisions, 10);
    SmoothedPath.Reserve(TotalSamples);

    for (int32 i = 0; i < TotalSamples; i++)
    {
        const float Distance = (SplineLength * i) / (TotalSamples - 1);
        SmoothedPath.Add(TempSplineComponent->GetLocationAtDistanceAlongSpline(
            Distance, ESplineCoordinateSpace::World));
    }

    if (TempSplineComponent)
    {
        TempSplineComponent->DestroyComponent();
    }

    return SmoothedPath;
}

TArray<FVector> UFormationPathModifier::StraightenPath(
    UWorld* WorldContextObject,
    const TArray<FVector>& RawPath
)
{
    if (RawPath.Num() < 2)
    {
        return RawPath;
    }

    TArray<FVector> StraightenedPath;
    StraightenedPath.Add(RawPath[0]);

    int32 BasePointIndex = 0;
    for (int32 i = BasePointIndex + 2; i < RawPath.Num(); ++i)
    {
        FVector HitLocation;
        bool bObstructed = UNavigationSystemV1::NavigationRaycast(
            WorldContextObject,
            RawPath[BasePointIndex],
            RawPath[i],
            HitLocation
        );

        if (bObstructed)
        {
            StraightenedPath.Add(RawPath[i - 1]);
            BasePointIndex = i - 1;
        }
    }

    StraightenedPath.Add(RawPath.Last());
    return StraightenedPath;
}

