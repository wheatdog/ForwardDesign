// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>
#include <SDL.h>
#define WD_DEBUG
#include <wd_common.h>

#define StateMax 32

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

#define StateRadius 100
struct state
{
    vec2 Pos;
    ImVec4 Color;
    u32 State[2];
    u32 Output[2];
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
    state *ThisState = States + (*StateIndex)%StateMax;
    (*StateIndex) = (*StateIndex + 1) % StateMax;

    if (*StateCount < StateMax) (*StateCount)++;

    ThisState->Pos.X = X;
    ThisState->Pos.Y = Y;

    return;
}

state *InStateSquare(state *States, int StatesCount, int X, int Y)
{
    for (int Index = 0; Index < StatesCount; ++Index)
    {
        state *ThisState = States + Index;
        if ((X < (ThisState->Pos.X + 0.5*StateRadius)) &&
            (X >= (ThisState->Pos.X - 0.5*StateRadius)) &&
            (Y < (ThisState->Pos.Y + 0.5*StateRadius)) &&
            (Y >= (ThisState->Pos.Y - 0.5*StateRadius)))
        {
            return ThisState;
        }
    }
    return 0;
}

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

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
    SDL_Window *window = SDL_CreateWindow("ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);//|SDL_WINDOW_RESIZABLE);
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

    // GLuint VertexArrayID;
    // glGenVertexArrays(1, &VertexArrayID);
    // glBindVertexArray(VertexArrayID);

    i32 StatesCount = 0;
    i32 StateIndex = 0;
    state States[StateMax] = {};

    // static const GLfloat g_vertex_buffer_data[] = {
    //     -1.0f, -1.0f, 0.0f,
    //     1.0f, -1.0f, 0.0f,
    //     0.0f,  1.0f, 0.0f,
    // };
    // static const GLfloat g_color_buffer_data[] = {
    //     0.583f,  0.771f,  0.014f,
    //     0.609f,  0.115f,  0.436f,
    //     0.327f,  0.483f,  0.844f,
    // };

    // GLuint vertexbuffer;
    // glGenBuffers(1, &vertexbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // GLuint colorbuffer;
    // glGenBuffers(1, &colorbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    bool show_test_window = false;
    bool show_another_window = false;
    bool show_state[StateMax] = {};
    ImVec4 clear_color = ImColor(114, 144, 154);

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
                int MouseX, MouseY;
                SDL_GetMouseState(&MouseX, &MouseY);

                AddNewState(States, &StatesCount, &StateIndex, MouseX, MouseY);
            }

            if (event.button.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                state* Inside = InStateSquare(States, StatesCount, event.button.x, event.button.y);
                if (Inside)
                {
                    Inside->Color = ImColor(255, 255, 255);
                }
            }
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            ImGui::Begin("Main Window");
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("%d, %d\n", StatesCount, StateIndex);

            if (ImGui::CollapsingHeader("States"))
            {
                for (int StateNum = 0; StateNum < StatesCount; ++StateNum)
                {
                    char buf[32];
                    sprintf(buf, "state %d", StateNum);
                    if (ImGui::CollapsingHeader(buf))
                    {
                        ImGui::ColorEdit3(buf, (float*)&(States[StateNum].Color));
                    }
                }
            }
            ImGui::End();
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
            DrawData[0].Pos = V2(ThisState->Pos.X - 0.5f*StateRadius, ThisState->Pos.Y - 0.5f*StateRadius);
            DrawData[1].Pos = V2(ThisState->Pos.X + 0.5f*StateRadius, ThisState->Pos.Y - 0.5f*StateRadius);
            DrawData[2].Pos = V2(ThisState->Pos.X - 0.5f*StateRadius, ThisState->Pos.Y + 0.5f*StateRadius);
            DrawData[3].Pos = V2(ThisState->Pos.X + 0.5f*StateRadius, ThisState->Pos.Y + 0.5f*StateRadius);

            for (int VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
            {
                DrawData[VertexIndex].Color = ColorToU32(ThisState->Color);
            }

            glBindBuffer(GL_ARRAY_BUFFER, VBOHandle);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)wdArrayCount(DrawData) * sizeof(draw_vert),
                         (GLvoid *)DrawData, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, wdArrayCount(DrawData));
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
