#include <imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <GL/gl3w.h>
#include <SDL.h>
// #define WD_DEBUG
#include "wd_common.h"
#include "state.cpp"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))
/*
  TODO(wheatdog):
  - only support 1 input, 1 output now, need to find a way to handle multiple input and ou
 */

#define PI 3.14159265f
#define STATE_MAX 32
#define REG_MAX 5

const char* Global_RegTypes[] = { "SR", "JK", "T", "D" };
float Global_StateRadius = 100.0f;

struct vec2
{
    float X, Y;
};

vec2 V2(float X, float Y)
{
    vec2 Result;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

enum line_status
{
    LineStatus_None,
    LineStatus_Setting,
    LineStatus_Finish,
};

struct state;
struct connection
{
    line_status LineStatus;
    vec2 End;
    i32 Target;
    int Output;
};

struct state
{
    vec2 Pos;
    ImVec4 Color;
    bool Show;

    i32 In;
    // TODO: Right now only support 1 input, need to figure a way to handle
    connection Connection[2];
};

struct draw_vert
{
    vec2 Pos;
    vec2 UV;
    u32 Color;
};

u32 ColorToU32(ImVec4 Color)
{
    u32 R = (u32)(Color.x * 255.0f);
    u32 G = (u32)(Color.y * 255.0f);
    u32 B = (u32)(Color.z * 255.0f);
    u32 A = (u32)(Color.w * 255.0f);

    u32 Result = (A << 24) | (B << 16) | (G << 8) | R;

    return Result;
}

void CheckProgramCompile(GLuint Handle)
{
    GLint Result = GL_FALSE;
    int InfoLogLength = 0;
    glGetProgramiv(Handle, GL_LINK_STATUS, &Result);
    glGetProgramiv(Handle, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        int OutputLength;
        char ErrorMessage[InfoLogLength+1];
        glGetProgramInfoLog(Handle, InfoLogLength, &OutputLength, ErrorMessage);
        ErrorMessage[InfoLogLength] = '\0';
        wdDebugPrint("%s\n", ErrorMessage);
    }
}

void CheckShaderCompile(GLuint Handle)
{
    GLint Result = GL_FALSE;
    int InfoLogLength = 0;
    glGetShaderiv(Handle, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(Handle, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        int OutputLength;
        char ErrorMessage[InfoLogLength+1];
        glGetShaderInfoLog(Handle, InfoLogLength, &OutputLength, ErrorMessage);
        ErrorMessage[InfoLogLength] = '\0';
        wdDebugPrint("%s\n", ErrorMessage);
    }
}

void AddNewState(state *States, int *StateCount, int *StateIndex, int X, int Y)
{
    state *ThisState = States + (*StateIndex)%STATE_MAX;
    (*StateIndex) = (*StateIndex + 1) % STATE_MAX;

    if (*StateCount < STATE_MAX) (*StateCount)++;

    ThisState->Pos.X = X;
    ThisState->Pos.Y = Y;
    ThisState->Color = ImColor(rand() % 128, rand() % 128, rand() % 128);

    return;
}

state *InStateSquare(state *States, int StatesCount, int X, int Y)
{
    for (int Index = 0; Index < StatesCount; ++Index)
    {
        state *ThisState = States + Index;
        if ((X < (ThisState->Pos.X + 0.5*Global_StateRadius)) &&
            (X >= (ThisState->Pos.X - 0.5*Global_StateRadius)) &&
            (Y < (ThisState->Pos.Y + 0.5*Global_StateRadius)) &&
            (Y >= (ThisState->Pos.Y - 0.5*Global_StateRadius)))
        {
            return ThisState;
        }
    }
    return 0;
}

void Generate(state *States, i32 StateCount, i32 RegCount, int *RegType)
{
    state_table_info StateTableInfo = {};
    StateTableInfo.InCount = 1;
    StateTableInfo.OutCount = 1;
    StateTableInfo.RegCount = RegCount;

    StateTableInfo.VariableCount = StateTableInfo.InCount + StateTableInfo.RegCount;
    StateTableInfo.RowMax = (int)pow(2.0f, (float)StateTableInfo.VariableCount);
    StateTableInfo.ColMax = StateTableInfo.InCount + 2*StateTableInfo.RegCount + StateTableInfo.OutCount;
    StateTableInfo.StateTable = (int **)malloc(StateTableInfo.RowMax*sizeof(int *));
    for (int Row = 0; Row < StateTableInfo.RowMax; ++Row)
    {
        StateTableInfo.StateTable[Row] = (int *)malloc(StateTableInfo.ColMax*sizeof(int));
    }

    for (i32 Row = 0; Row < StateTableInfo.RowMax; ++Row)
    {
        int ThisStateNumber = (Row >> StateTableInfo.InCount);
        int ThisInput = Row ^ (ThisStateNumber << StateTableInfo.InCount);
        int NextStateNumber = States[ThisStateNumber].Connection[ThisInput].Target;
        int Output = States[ThisStateNumber].Connection[ThisInput].Target;

        int Combine = ((ThisStateNumber << (StateTableInfo.OutCount + StateTableInfo.RegCount + StateTableInfo.InCount)) |
                       (ThisInput << (StateTableInfo.OutCount + StateTableInfo.RegCount)) |
                       (NextStateNumber << StateTableInfo.OutCount) |
                       (Output));
        for (i32 Col = 0; Col < StateTableInfo.ColMax; ++Col)
        {
            int Result = (Combine >> (StateTableInfo.ColMax - Col - 1)) & 1;
            StateTableInfo.StateTable[Row][Col] = Result;
        }
    }

    StateTableInfo.RegInfo = (reg_info *)malloc(StateTableInfo.RegCount*sizeof(reg_info));
    for (int RegIndex = 0; RegIndex < RegCount; ++RegIndex)
    {
        switch(Global_RegTypes[RegType[RegIndex]][0])
        {
        case 'S':
            StateTableInfo.RegInfo[RegIndex].RegType = REG_SR;
            StateTableInfo.RegInfo[RegIndex].InputCount = 2;
            break;
        case 'J':
            StateTableInfo.RegInfo[RegIndex].RegType = REG_JK;
            StateTableInfo.RegInfo[RegIndex].InputCount = 2;
            break;
        case 'T':
            StateTableInfo.RegInfo[RegIndex].RegType = REG_T;
            StateTableInfo.RegInfo[RegIndex].InputCount = 1;
            break;
        case 'D':
            StateTableInfo.RegInfo[RegIndex].RegType = REG_D;
            StateTableInfo.RegInfo[RegIndex].InputCount = 1;
            break;
        default:
            assert(!"Stange things happened, or you enter now allowed input!");
        }
    }

    StateTableInfo.Output = (input *)malloc(StateTableInfo.OutCount*sizeof(input));

    SolveTable(&StateTableInfo);

}

int main(int, char**)
{
    srand (time(NULL));

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    int WindowWidth = 1280;
    int WindowHeight = 720;

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window *window = SDL_CreateWindow("ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, SDL_WINDOW_OPENGL);//|SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color;\n"
        //"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    GLuint ShaderHandle = glCreateProgram();
    GLuint VertHandle = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(VertHandle, 1, &vertex_shader, 0);
    glShaderSource(FragHandle, 1, &fragment_shader, 0);
    glCompileShader(VertHandle); CheckShaderCompile(VertHandle);
    glCompileShader(FragHandle); CheckShaderCompile(FragHandle);
    glAttachShader(ShaderHandle, VertHandle);
    glAttachShader(ShaderHandle, FragHandle);
    glLinkProgram(ShaderHandle); CheckProgramCompile(ShaderHandle);

    GLuint AttribLocationTex = glGetUniformLocation(ShaderHandle, "Texture");
    GLuint AttribLocationProjMtx = glGetUniformLocation(ShaderHandle, "ProjMtx");
    GLuint AttribLocationPosition = glGetAttribLocation(ShaderHandle, "Position");
    GLuint AttribLocationUV = glGetAttribLocation(ShaderHandle, "UV");
    GLuint AttribLocationColor = glGetAttribLocation(ShaderHandle, "Color");

    GLuint VBOHandle, ElementsHandle;
    glGenBuffers(1, &VBOHandle);
    glGenBuffers(1, &ElementsHandle);

    GLuint VAOHandle;
    glGenVertexArrays(1, &VAOHandle);
    glBindVertexArray(VAOHandle);
    glBindBuffer(GL_ARRAY_BUFFER, VBOHandle);
    glEnableVertexAttribArray(AttribLocationPosition);
    glEnableVertexAttribArray(AttribLocationUV);
    glEnableVertexAttribArray(AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(draw_vert), (GLvoid*)OFFSETOF(draw_vert, Pos));
    glVertexAttribPointer(AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(draw_vert), (GLvoid*)OFFSETOF(draw_vert, UV));
    glVertexAttribPointer(AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(draw_vert), (GLvoid*)OFFSETOF(draw_vert, Color));
#undef OFFSETOF

    i32 StatesCount = 0;
    i32 StateIndex = 0;
    i32 RegCount = 0;
    i32 RegType[REG_MAX];
    state States[STATE_MAX] = {};

    bool show_test_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    vec2 Origin = V2(WindowWidth / 2.0f, WindowHeight / 2.0f);
    float Radius = 100.0f;
    state *SettingStates = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
            {
                done = true;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_a)
            {
#if 0
                int MouseX, MouseY;
                SDL_GetMouseState(&MouseX, &MouseY);

                AddNewState(States, &StatesCount, &StateIndex, MouseX, MouseY);
#else
                AddNewState(States, &StatesCount, &StateIndex, 0, 0);

                for (int BitIndex = 0; BitIndex < 31; ++BitIndex)
                {
                    if (StatesCount <= (int)(pow(2.0f, BitIndex+1)))
                    {
                        RegCount = BitIndex + 1;
                        break;
                    }
                }
#endif
            }

            if (event.button.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    state* Inside = InStateSquare(States, StatesCount, event.button.x, event.button.y);
                    if (Inside)
                    {
                        Inside->Show = true;
                    }
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    state* Inside = InStateSquare(States, StatesCount, event.button.x, event.button.y);
                    if (event.button.clicks == 1 && !SettingStates)
                    {
                        int MouseX, MouseY;
                        SDL_GetMouseState(&MouseX, &MouseY);
                        if (Inside)
                        {
                            Inside->Connection[Inside->In].LineStatus = LineStatus_Setting;
                            SettingStates = Inside;
                        }
                    }
                    else if (event.button.clicks == 1 && SettingStates)
                    {
                        if (Inside)
                        {
                            SettingStates->Connection[SettingStates->In].LineStatus = LineStatus_Finish;
                            SettingStates->Connection[SettingStates->In].End = Inside->Pos;
                            SettingStates->Connection[SettingStates->In].Target = (Inside - States);
                        }
                        else
                        {
                            SettingStates->Connection[SettingStates->In].LineStatus = LineStatus_None;
                        }

                        SettingStates = 0;
                    }
                }
            }
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            ImGui::Begin("Main Window");
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            ImGui::DragFloat("radius", &Radius, 10.0f, 100.0f, 2000.0f, "%.1f");
            ImGui::DragFloat("state box radius", &Global_StateRadius, 10.0f, 30.0f, 100.0f, "%.1f");
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            if (ImGui::Button("Generate")) Generate(States, StatesCount, RegCount, RegType);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Number of states: %d\n", StatesCount);
            ImGui::Text("%p\n", SettingStates);
            ImGui::Text("Number of registers: %d\n", RegCount);
            for (int RegIndex = 0; RegIndex < RegCount; ++RegIndex)
            {
                char RegNameBuf[32];
                sprintf(RegNameBuf, "Q[%d]", RegIndex);
                ImGui::PushID(RegIndex);
                ImGui::Combo(RegNameBuf, RegType + RegIndex, Global_RegTypes, IM_ARRAYSIZE(Global_RegTypes));
                ImGui::PopID();
            }
            ImGui::End();
        }

        double Delta = 2.0f*PI / (double)StatesCount;
        for (int StateNum = 0; StateNum < StatesCount; ++StateNum)
        {
            States[StateNum].Pos = V2(Origin.X + Radius*cos(StateNum*Delta), Origin.Y + Radius*sin(StateNum*Delta));

            for (int InIndex = 0; InIndex < 2; ++InIndex)
            {
                connection *Con = &States[StateNum].Connection[InIndex];
                if (States[StateNum].Connection[InIndex].LineStatus == LineStatus_Setting)
                {
                    Con->End = V2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
                }
                else if (States[StateNum].Connection[InIndex].LineStatus == LineStatus_Finish)
                {
                    Con->End = States[Con->Target].Pos;
                }
            }

            char buf[32];
            sprintf(buf, "state %d", StateNum);
            if (States[StateNum].Show)
            {
                ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
                ImGui::Begin(buf, &(States[StateNum].Show));
                ImGui::ColorEdit3("color", (float*)&(States[StateNum].Color));
                ImGui::RadioButton("0", &(States[StateNum].In), 0); ImGui::SameLine();
                ImGui::RadioButton("1", &(States[StateNum].In), 1);
                for (int InIndex = 0; InIndex < 2; ++InIndex)
                {
                    if (States[StateNum].In != InIndex) continue;
                    connection *Con = &States[StateNum].Connection[InIndex];
                    ImGui::PushID(InIndex);
                    ImGui::Text("%d\n", InIndex);
                    ImGui::BulletText("Next States: %d", Con->Target);
                    ImGui::BulletText("Output: %d", Con->Output);
                    ImGui::RadioButton("0", &(Con->Output), 0); ImGui::SameLine();
                    ImGui::RadioButton("1", &(Con->Output), 1);
                    ImGui::PopID();
                    break;
                }
                ImGui::End();
            }
        }
        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        const float ortho_projection[4][4] =
            {
                { 2.0f/ImGui::GetIO().DisplaySize.x, 0.0f,                   0.0f, 0.0f },
                { 0.0f,                  2.0f/-ImGui::GetIO().DisplaySize.y, 0.0f, 0.0f },
                { 0.0f,                  0.0f,                              -1.0f, 0.0f },
                {-1.0f,                  1.0f,                               0.0f, 1.0f },
            };
        glUseProgram(ShaderHandle);
        glUniform1i(AttribLocationTex, 0);
        glUniformMatrix4fv(AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
        glBindVertexArray(VAOHandle);

        for (int StateNum = 0; StateNum < StatesCount; ++StateNum)
        {
            state *ThisState = States + StateNum;
            draw_vert DrawData[4] = {};
            DrawData[0].Pos = V2(ThisState->Pos.X - 0.5f*Global_StateRadius, ThisState->Pos.Y - 0.5f*Global_StateRadius);
            DrawData[1].Pos = V2(ThisState->Pos.X + 0.5f*Global_StateRadius, ThisState->Pos.Y - 0.5f*Global_StateRadius);
            DrawData[2].Pos = V2(ThisState->Pos.X - 0.5f*Global_StateRadius, ThisState->Pos.Y + 0.5f*Global_StateRadius);
            DrawData[3].Pos = V2(ThisState->Pos.X + 0.5f*Global_StateRadius, ThisState->Pos.Y + 0.5f*Global_StateRadius);

            for (int VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
            {
                DrawData[VertexIndex].Color = ColorToU32(ThisState->Color);
            }

            glBindBuffer(GL_ARRAY_BUFFER, VBOHandle);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)wdArrayCount(DrawData) * sizeof(draw_vert),
                         (GLvoid *)DrawData, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, wdArrayCount(DrawData));

            for (int InIndex = 0; InIndex < 2; ++InIndex)
            {
                if (ThisState->Connection[InIndex].LineStatus)
                {
                    draw_vert DrawLine[2] = {};
                    DrawLine[0].Pos = V2(ThisState->Pos.X, ThisState->Pos.Y);
                    DrawLine[1].Pos = V2(ThisState->Connection[InIndex].End.X, ThisState->Connection[InIndex].End.Y);

                    for (int VertexIndex = 0; VertexIndex < 2; ++VertexIndex)
                    {
                        DrawLine[VertexIndex].Color = ColorToU32(ThisState->Color);
                    }

                    glBindBuffer(GL_ARRAY_BUFFER, VBOHandle);
                    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)wdArrayCount(DrawLine) * sizeof(draw_vert),
                                 (GLvoid *)DrawLine, GL_STREAM_DRAW);
                    glDrawArrays(GL_LINES, 0, wdArrayCount(DrawLine));
                }
            }
        }

        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
