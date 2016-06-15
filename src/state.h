enum reg_type
{
    REG_JK, REG_D, REG_T, REG_SR
};

struct input
{
    char Name[32];

    int OneCount;
    int *One;

    int DontCareCount;
    int *DontCare;
};

struct reg_info
{
    reg_type RegType;

    int InputCount;
    input Input[2];
};

struct state_table_info
{
    int InCount;
    int OutCount;
    int RegCount;

    int VariableCount;

    int RowMax;
    int ColMax;

    int **StateTable;
    reg_info *RegInfo;

    input *Output;
};
