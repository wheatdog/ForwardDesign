#include <stdio.h>
#include <math.h>
// #define WD_DEBUG
#include "wd_common.h"
#include <assert.h>
#include "state.h"

void FillOutputInfo(state_table_info *StateTableInfo, int OutputIndex)
{
    int Col = OutputIndex + StateTableInfo->InCount + 2*StateTableInfo->RegCount;

    input *ThisInput = StateTableInfo->Output + OutputIndex;
    snprintf(ThisInput->Name, sizeof(ThisInput->Name), "Z[%d]", OutputIndex);

    ThisInput->OneCount = 0;
    int *TempOne = (int *)malloc(StateTableInfo->RowMax*sizeof(int));

    ThisInput->DontCareCount = 0;
    int *TempDontCare = (int *)malloc(StateTableInfo->RowMax*sizeof(int));

    for (int Row = 0; Row < StateTableInfo->RowMax; ++Row)
    {
        int Check = StateTableInfo->StateTable[Row][Col];
        if (Check == 1)
        {
            TempOne[ThisInput->OneCount++] = Row;
        }
        else if (Check == -1)
        {
            TempDontCare[ThisInput->DontCareCount++] = Row;
        }
    }

    ThisInput->One = (int *)malloc(ThisInput->OneCount*sizeof(int));
    wdDebugPrint("1(%d):\n", ThisInput->OneCount);
    for (int Index = 0; Index < ThisInput->OneCount; ++Index)
    {
        ThisInput->One[Index] = TempOne[Index];
        wdDebugPrint("%d\n", ThisInput->One[Index]);
    }

    ThisInput->DontCare = (int *)malloc(ThisInput->DontCareCount*sizeof(int));
    wdDebugPrint("D(%d):\n", ThisInput->DontCareCount);
    for (int Index = 0; Index < ThisInput->DontCareCount; ++Index)
    {
        ThisInput->DontCare[Index] = TempDontCare[Index];
        wdDebugPrint("%d\n", ThisInput->DontCare[Index]);
    }

    free(TempOne);
    free(TempDontCare);
}

void FillRegInputInfo(state_table_info *StateTableInfo, int LookUpTable[2][2],
                      int RegIndex, int InputIndex)
{
    int Start = RegIndex + StateTableInfo->InCount + StateTableInfo->RegCount;

    input *ThisInput = StateTableInfo->RegInfo[RegIndex].Input + InputIndex;

    ThisInput->OneCount = 0;
    int *TempOne = (int *)malloc(StateTableInfo->RowMax*sizeof(int));

    ThisInput->DontCareCount = 0;
    int *TempDontCare = (int *)malloc(StateTableInfo->RowMax*sizeof(int));

    for (int Row = 0; Row < StateTableInfo->RowMax; ++Row)
    {
        int Check = LookUpTable[StateTableInfo->StateTable[Row][RegIndex]][StateTableInfo->StateTable[Row][Start]];
        if (Check == 1)
        {
            TempOne[ThisInput->OneCount++] = Row;
        }
        else if (Check == -1)
        {
            TempDontCare[ThisInput->DontCareCount++] = Row;
        }

        wdDebugPrint("%d -> %d: %d\n",
                     StateTableInfo->StateTable[Row][RegIndex],
                     StateTableInfo->StateTable[Row][Start],
                     LookUpTable[StateTableInfo->StateTable[Row][RegIndex]][StateTableInfo->StateTable[Row][Start]]);
    }

    ThisInput->One = (int *)malloc(ThisInput->OneCount*sizeof(int));
    wdDebugPrint("1(%d):\n", ThisInput->OneCount);
    for (int Index = 0; Index < ThisInput->OneCount; ++Index)
    {
        ThisInput->One[Index] = TempOne[Index];
        wdDebugPrint("%d\n", ThisInput->One[Index]);
    }

    ThisInput->DontCare = (int *)malloc(ThisInput->DontCareCount*sizeof(int));
    wdDebugPrint("D(%d):\n", ThisInput->DontCareCount);
    for (int Index = 0; Index < ThisInput->DontCareCount; ++Index)
    {
        ThisInput->DontCare[Index] = TempDontCare[Index];
        wdDebugPrint("%d\n", ThisInput->DontCare[Index]);
    }

    free(TempOne);
    free(TempDontCare);
}

// TODO(wheatdog): This function now only print sum of product of one.
// If we have time, we will use Quine-McCluskey algorithm to simplify terms.
void PrintTerm(state_table_info *StateTableInfo, input *RegInput)
{
    printf("%s: ", RegInput->Name);
    for (int OneIndex = 0; OneIndex < RegInput->OneCount; ++OneIndex)
    {
        int ToCheck = RegInput->One[OneIndex];

        for (int BitIndex = StateTableInfo->RegCount + StateTableInfo->InCount;
             BitIndex > StateTableInfo->InCount;
             --BitIndex)
        {
            printf("%s", (ToCheck & (1 << (BitIndex - 1)))? "" : "~" );
            printf("Q[%d]", StateTableInfo->RegCount + StateTableInfo->InCount - BitIndex);
        }

        for (int BitIndex = StateTableInfo->InCount; BitIndex > 0; --BitIndex)
        {
            printf("%s", (ToCheck & (1 << (BitIndex - 1)))? "" : "~" );
            printf("X[%d]", StateTableInfo->InCount - BitIndex);
        }

        if ((OneIndex + 1) < RegInput->OneCount)
        {
            printf(" + ");
        }
    }
    printf("\n");
}

int SolveTable(state_table_info *StateTableInfo)
{
    // -1 means don't care
    int JK_TABLE[2][2][2] =
        {
            // J
            {
                {0, 1},
                {-1, -1},
            },

            // K
            {
                {-1, -1},
                {1, 0},
            },
        };

    int D_TABLE[2][2] =
        {
            {0, 1},
            {0, 1},
        };

    int T_TABLE[2][2] =
        {
            {0, 1},
            {1, 0},
        };

    int SR_TABLE[2][2][2] =
        {
            // S
            {
                {0, 1},
                {0, -1},
            },

            // R
            {
                {-1, 0},
                {1, 0},
            },
        };

    for (int Col = 0; Col < StateTableInfo->RegCount; ++Col)
    {
        switch(StateTableInfo->RegInfo[Col].RegType)
        {
        case REG_SR:
        {
            wdDebugPrint("Q_%d+ is SR\n", Col);

            input *ThisInput = StateTableInfo->RegInfo[Col].Input;
            wdDebugPrint("S:\n");
            snprintf(ThisInput[0].Name, sizeof(ThisInput->Name), "Q[%d]_S", Col);
            FillRegInputInfo(StateTableInfo, (SR_TABLE[0]), Col, 0);
            wdDebugPrint("R:\n");
            snprintf(ThisInput[1].Name, sizeof(ThisInput->Name), "Q[%d]_R", Col);
            FillRegInputInfo(StateTableInfo, (SR_TABLE[1]), Col, 1);
        } break;
        case REG_JK:
        {
            wdDebugPrint("Q_%d+ is  J  K\n", Col);

            input *ThisInput = StateTableInfo->RegInfo[Col].Input;
            wdDebugPrint("J:\n");
            snprintf(ThisInput[0].Name, sizeof(ThisInput->Name), "Q[%d]_J", Col);
            FillRegInputInfo(StateTableInfo, (JK_TABLE[0]), Col, 0);
            wdDebugPrint("K:\n");
            snprintf(ThisInput[1].Name, sizeof(ThisInput->Name), "Q[%d]_K", Col);
            FillRegInputInfo(StateTableInfo, (JK_TABLE[1]), Col, 1);
        } break;
        case REG_T:
        {
            wdDebugPrint("Q_%d+ is T\n", Col);
            input *ThisInput = StateTableInfo->RegInfo[Col].Input;
            snprintf(ThisInput[0].Name, sizeof(ThisInput->Name), "Q[%d]_T", Col);
            FillRegInputInfo(StateTableInfo, (T_TABLE), Col, 0);
        } break;
            break;
        case REG_D:
        {
            wdDebugPrint("Q_%d+ is D\n", Col);
            input *ThisInput = StateTableInfo->RegInfo[Col].Input;
            snprintf(ThisInput[0].Name, sizeof(ThisInput->Name), "Q[%d]_D", Col);
            FillRegInputInfo(StateTableInfo, (D_TABLE), Col, 0);
        } break;
            break;
        default:
            assert(!"Stange things happened!");
        }
    }

    for (int RegIndex = 0; RegIndex < StateTableInfo->RegCount; ++RegIndex)
    {
        reg_info *ThisReg = StateTableInfo->RegInfo + RegIndex;
        for (int RegInputIndex = 0; RegInputIndex < ThisReg->InputCount; ++RegInputIndex)
        {
            PrintTerm(StateTableInfo, ThisReg->Input + RegInputIndex);
        }
    }

    for (int OutputIndex = 0; OutputIndex < StateTableInfo->OutCount; ++OutputIndex)
    {
        FillOutputInfo(StateTableInfo, OutputIndex);
        PrintTerm(StateTableInfo, StateTableInfo->Output + OutputIndex);
    }

    return 0;
}
