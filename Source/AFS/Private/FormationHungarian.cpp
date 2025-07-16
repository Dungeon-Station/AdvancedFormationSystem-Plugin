/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

#include "FormationHungarian.h"

FFormationHungarian::FFormationHungarian()
{
}

FFormationHungarian::~FFormationHungarian()
{
}

void FFormationHungarian::Solve(const TArray<TArray<float>>& CostMatrix,
    TArray<int32>& OutRowToCol,
    TArray<int32>& OutColToRow)
{
    const int32 Num = CostMatrix.Num();
    OutRowToCol.Init(-1, Num);
    OutColToRow.Init(-1, Num);

    TArray<float> RowLabel;    RowLabel.Init(0.0f, Num + 1);
    TArray<float> ColLabel;    ColLabel.Init(0.0f, Num + 1);
    TArray<int32> ColMatch;    ColMatch.Init(0, Num + 1);
    TArray<int32> PrevColumn;  PrevColumn.Init(0, Num + 1);

    for (int32 Row = 1; Row <= Num; ++Row)
    {
        ColMatch[0] = Row;
        int32 CurCol = 0;

        TArray<float> Slack;      Slack.Init(FLT_MAX, Num + 1);
        TArray<bool>  Visited;    Visited.Init(false, Num + 1);

        do
        {
            Visited[CurCol] = true;
            int32 CurRow = ColMatch[CurCol];
            float BestDelta = FLT_MAX;
            int32 NextCol = 0;

            for (int32 Col = 1; Col <= Num; ++Col)
            {
                if (Visited[Col]) continue;
                float CostDiff = CostMatrix[CurRow - 1][Col - 1]
                    - RowLabel[CurRow]
                    - ColLabel[Col];
                if (CostDiff < Slack[Col])
                {
                    Slack[Col] = CostDiff;
                    PrevColumn[Col] = CurCol;
                }
                if (Slack[Col] < BestDelta)
                {
                    BestDelta = Slack[Col];
                    NextCol = Col;
                }
            }

            for (int32 Col = 0; Col <= Num; ++Col)
            {
                if (Visited[Col])
                {
                    RowLabel[ColMatch[Col]] += BestDelta;
                    ColLabel[Col] -= BestDelta;
                }
                else
                {
                    Slack[Col] -= BestDelta;
                }
            }

            CurCol = NextCol;

        } while (ColMatch[CurCol] != 0);

        do
        {
            int32 PrevCol = PrevColumn[CurCol];
            ColMatch[CurCol] = ColMatch[PrevCol];
            CurCol = PrevCol;
        } while (CurCol != 0);
    }

    for (int32 Col = 1; Col <= Num; ++Col)
    {
        int32 Row = ColMatch[Col];
        if (Row > 0)
        {
            OutRowToCol[Row - 1] = Col - 1;
            OutColToRow[Col - 1] = Row - 1;
        }
    }
}