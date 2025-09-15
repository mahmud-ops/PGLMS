// power_grid.cpp
// Power grid simulation with OpenGL, GLFW, GLAD, ImGui, stb_image.
// Simulates nuclear plant -> transmission tower -> 5 houses.
// Houses have load %, overload at 100%, manual cut, auto-shed, repair.

#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <fstream>
#include <random>

#include <windows.h>
#include <mmsystem.h>

using namespace std::chrono;

std::string load_shader_source(const char *path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        glGetShaderInfoLog(s, 1024, NULL, buf);
        std::cerr << "Shader compile error: " << buf << std::endl;
    }
    return s;
}

GLuint create_program()
{
    std::string vert_src = load_shader_source("shaders/vertex.glsl");
    std::string frag_src = load_shader_source("shaders/fragment.glsl");
    if (vert_src.empty() || frag_src.empty())
        return 0;

    GLuint v = compile_shader(GL_VERTEX_SHADER, vert_src.c_str());
    GLuint f = compile_shader(GL_FRAGMENT_SHADER, frag_src.c_str());
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        glGetProgramInfoLog(p, 1024, NULL, buf);
        std::cerr << "Link error:" << buf << std::endl;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

struct Texture
{
    GLuint id = 0;
    int w = 0, h = 0, c = 0;
};

Texture load_texture(const char *path)
{
    Texture t;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(path, &t.w, &t.h, &t.c, 4);
    if (!data)
    {
        std::cerr << "Failed to load: " << path << std::endl;
        return t;
    }
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return t;
}

// House state
// Normal: powered && load < 100%
// Off: !powered (manual cut or manual/auto-shed)
// Overload: powered && load >= 100%
struct House
{
    std::string name;
    float load = 0.0f; // 0..120
    bool powered = true;
    bool autoShed = false; // requires repair
    bool repairing = false;
    float repairTimer = 0.0f;
    float cutTimer = 0.0f;          // for manual cut return timer
    float manualShedTimer = 0.0f; // for manual shed auto restore
    float x, y;                   // normalized coords -1..1
};

// Globals for demo
int winW = 1280, winH = 720;
GLuint program;
GLuint vao, vbo, lineVao, lineVbo;
Texture plantTex, towerTex, houseNormalTex, houseOffTex, houseOverloadTex;
std::vector<House> houses;
float autoOverloadTimer = 0.0f;
bool showOverloadPopup = false;
int overloadedHouse = -1;

// Day/night cycle variables
float dayTime = 0.0f; // 0..1, 0 = midnight, 0.5 = noon, 1 = midnight
float daySpeed = 1.0f / 60.0f; // speed of day/night cycle

// Helpers
float nowSeconds()
{
    static auto start = high_resolution_clock::now();
    return duration_cast<duration<float>>(high_resolution_clock::now() - start).count();
}

void drawQuad(Texture *t, float cx, float cy, float sx, float sy, float alpha = 1.0f,
              bool useTexture = true, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
              float angle = 0.0f)
{
    glUseProgram(program);
    glBindVertexArray(vao);

    if (useTexture && t && t->id != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glUniform1i(glGetUniformLocation(program, "uTex"), 0);
        glUniform1i(glGetUniformLocation(program, "uUseTexture"), 1);
    }
    else
    {
        glUniform1i(glGetUniformLocation(program, "uUseTexture"), 0);
        glUniform4f(glGetUniformLocation(program, "uColor"), r, g, b, a);
    }

    glUniform2f(glGetUniformLocation(program, "uPos"), cx, cy);
    glUniform2f(glGetUniformLocation(program, "uScale"), sx, sy);
    glUniform1f(glGetUniformLocation(program, "uAlpha"), alpha);
    glUniform1f(glGetUniformLocation(program, "uAngle"), angle); // pass rotation to shader

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void setup_render()
{
    program = create_program();
    // unit quad centered at 0,0 size 1x1 in NDC
    float verts[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 1.0f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main()
{
    // init GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(winW, winH, "Power Grid Demo", NULL, NULL);
    if (!window)
    {
        std::cerr << "Window failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD failed" << std::endl;
        return -1;
    }

    setup_render();

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // load textures
    plantTex = load_texture("images/powerPlant.png");
    towerTex = load_texture("images/tower.png");
    houseNormalTex = load_texture("images/house_normal.png");
    houseOffTex = load_texture("images/house_off.png");
    houseOverloadTex = load_texture("images/house_overload.png");

    // houses
    for (int i = 0; i < 5; i++)
    {
        House h;
        h.name = "House " + std::to_string(i + 1);
        h.load = 20.0f + i * 15.0f; // start values
        if (i == 0)
            h.load = 90.0f; // per user
        h.powered = true;
        h.autoShed = false;
        h.repairing = false;
        // positions along x
        float sx = -0.9f + i * 0.45f; // spread
        h.x = sx;
        h.y = -0.3f;
        houses.push_back(h);
    }

    // plant and tower pos
    float plantX = -0.9f, plantY = 0.6f;
    float towerX = 0.0f, towerY = 0.2f;

    float lastTime = nowSeconds();
    std::srand(std::time(0));
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        float t = nowSeconds();
        float dt = t - lastTime;
        lastTime = t;

        // Update day/night cycle
        dayTime += dt * daySpeed;
        if (dayTime > 1.0f)
        {
            dayTime -= 1.0f;
        }

        // Overload timing tweak: higher frequency at night
        float overloadInterval = 25.0f;
        if (dayTime < 0.25f || dayTime > 0.75f)
        { // night hours
            overloadInterval = 7.0f; // faster overloads at night
        }

        // Automatic overload trigger
        autoOverloadTimer += dt;
        if (autoOverloadTimer >= overloadInterval)
        {
            autoOverloadTimer = 0.0f;
            int idx = std::rand() % 5;
            houses[idx].load = 110.0f; // trigger overload
        }
        
        // simulate loads changing randomly a bit
        for (int i = 0; i < houses.size(); i++)
        {
            auto &h = houses[i];
            if (h.powered && !h.repairing)
            {
                // small fluctuation
                h.load += ((std::rand() % 200 - 100) / 100.0f) * 0.05f;
                if (h.load < 0)
                    h.load = 0;
            }
            // overload detection
            if (h.load >= 100.0f && h.powered && !showOverloadPopup && overloadedHouse == -1)
            {
                showOverloadPopup = true;
                overloadedHouse = i;
                // Set volume to maximum for default wave output device
                HWAVEOUT hWaveOut;
                WAVEFORMATEX wfx = {0};
                wfx.wFormatTag = WAVE_FORMAT_PCM;
                wfx.nChannels = 1;
                wfx.nSamplesPerSec = 44100;
                wfx.wBitsPerSample = 16;
                wfx.nBlockAlign = (WORD)(wfx.nChannels * wfx.wBitsPerSample / 8);
                wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
                wfx.cbSize = 0;
                if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
                    waveOutSetVolume(hWaveOut, 0xFFFFFFFF);
                    waveOutClose(hWaveOut);
                }
                PlaySoundW(L"Sound/overload_alert.mp3", NULL, SND_FILENAME | SND_ASYNC);
            }

            // timers
            if (!h.powered && !h.autoShed && h.manualShedTimer == 0.0f)
            {
                // manual cut -> returns after 20 sec
                h.cutTimer += dt;
                if (h.cutTimer >= 20.0f)
                {
                    h.powered = true;
                    h.cutTimer = 0.0f;
                }
            }
            if (h.manualShedTimer > 0.0f)
            {
                h.manualShedTimer -= dt;
                if (h.manualShedTimer <= 0.0f)
                {
                    h.powered = true;
                    h.manualShedTimer = 0.0f;
                    h.load = 30.0f;
                }
            }
            if (h.repairing)
            {
                h.repairTimer += dt;
                if (h.repairTimer >= 20.0f)
                {
                    h.repairing = false;
                    h.repairTimer = 0.0f;
                    h.autoShed = false;
                    h.powered = true;
                    h.load = 30.0f; // stable
                }
            }
        }

        // Render
        glViewport(0, 0, winW, winH);
        // Adjust background color to transition from morning (#abc0e3) to night (dark blue)
        float factor = (std::cos(dayTime * 2.0f * 3.14159f) + 1.0f) / 2.0f; // 1 at midnight, 0 at noon
        float r_morning = 171.0f / 255.0f, g_morning = 192.0f / 255.0f, b_morning = 227.0f / 255.0f;
        float r_night = 0.0f, g_night = 0.0f, b_night = 0.5f;
        float r = r_night + factor * (r_morning - r_night);
        float g = g_night + factor * (g_morning - g_night);
        float b = b_night + factor * (b_morning - b_night);
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ---- Draw wires (thick black quads underneath) ----
        auto drawWire = [](float x1, float y1, float x2, float y2, float thickness)
        {
            float dx = x2 - x1;
            float dy = y2 - y1;
            float length = std::sqrt(dx * dx + dy * dy);
            float angle = std::atan2(dy, dx);

            float cx = (x1 + x2) / 2.0f;
            float cy = (y1 + y2) / 2.0f;

            // Draw quad with rotation support
            drawQuad(nullptr, cx, cy, length, thickness, 1.0f, false, 0.0f, 0.0f, 0.0f, 1.0f, angle);
        };

        // Plant to tower
        drawWire(plantX, plantY, towerX, towerY, 0.01f); // wire

        // Tower to each house
        for (int i = 0; i < 5; i++)
        {
            float hx = houses[i].x;
            float hy = houses[i].y; // center of house sprite
            drawWire(towerX, towerY, hx, hy, 0.01f);
        }

        // ---- Draw plant and tower on top of wires ----
        drawQuad(&plantTex, plantX, plantY, 0.25f * 2.0f, 0.25f * 2.0f); // 0.5f, 0.5f
        drawQuad(&towerTex, towerX, towerY, 0.2f * 2.0f, 0.25f * 2.0f);  // 0.4f, 0.5f

        // animate electricity flow: small bright circles moving along wire (behind houses)
        for (int i = 0; i < 5; i++)
        {
            if (!houses[i].powered)
                continue; // no flow if cut
            float progress = std::fmod(nowSeconds() * 0.6f + i * 0.15f, 1.0f);
            // from plant to tower to house
            if (progress < 0.5f)
            {
                // plant to tower
                float p = progress * 2.0f;
                float px = plantX + (towerX - plantX) * p;
                float py = plantY + (towerY - plantY) * p;
                drawQuad(nullptr, px, py, 0.015f, 0.015f, 1.0f, false, 1.0f, 1.0f, 0.0f, 1.0f); // bright yellow circle
            }
            else
            {
                // tower to house
                float p = (progress - 0.5f) * 2.0f;
                float hx = houses[i].x;
                float hy = houses[i].y; // center of house
                float px = towerX + (hx - towerX) * p;
                float py = towerY + (hy - towerY) * p;
                drawQuad(nullptr, px, py, 0.015f, 0.015f, 1.0f, false, 1.0f, 1.0f, 0.0f, 1.0f);
            }
        }

        // draw houses
        for (int i = 0; i < 5; i++)
        {
            auto &h = houses[i];
            Texture *tex = &houseNormalTex;
            if (h.autoShed || (h.load >= 100.0f && h.powered))
            {
                tex = &houseOverloadTex;
            }
            else if (!h.powered)
            {
                tex = &houseOffTex;
            }
            drawQuad(tex, h.x, h.y, 0.18f, 0.18f);
        }

        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Overload popup
        if (showOverloadPopup)
        {
            ImGui::OpenPopup("Overload Alert");
        }
        if (ImGui::BeginPopupModal("Overload Alert", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("House %d is overloaded, do you want to cut power?", overloadedHouse + 1);
            if (ImGui::Button("Yes"))
            {
                // Manual shed: off for 10s, then auto restore
                houses[overloadedHouse].powered = false;
                houses[overloadedHouse].manualShedTimer = 10.0f;
                showOverloadPopup = false;
                overloadedHouse = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                // Auto shed: off, requires repair
                houses[overloadedHouse].powered = false;
                houses[overloadedHouse].autoShed = true;
                showOverloadPopup = false;
                overloadedHouse = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Begin("Grid Monitor");
        ImGui::Text("Nuclear Plant -> Transmission -> Houses");
        ImGui::Separator();
        for (int i = 0; i < 5; i++)
        {
            auto &h = houses[i];
            ImGui::PushID(i);
            ImGui::Text("%s", h.name.c_str());
            ImGui::SameLine(200);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Load: %.1f%%", h.load);
            ImGui::Text("Powered: %s", h.powered ? "YES" : "NO");
            // Manual controls
            if (h.powered)
            {
                if (ImGui::Button("Manual Cut"))
                {
                    // manual cut: power cuts and returns after 20s
                    h.powered = false;
                    h.cutTimer = 0.0f;
                    h.autoShed = false;
                }
            }
            else
            {
                // if it was autoShed, show repair button
                if (h.autoShed)
                {
                    if (h.repairing)
                    {
                        ImGui::Text("Repairing House %d: %.0f s remaining", i + 1, 20.0f - h.repairTimer);
                    }
                    else
                    {
                        if (ImGui::Button("Repair"))
                        {
                            h.repairing = true;
                            h.repairTimer = 0.0f;
                        }
                    }
                }
                else if (h.manualShedTimer > 0.0f)
                {
                    ImGui::Text("Manual shed: will auto-restore in %.0f s", h.manualShedTimer);
                }
                else
                {
                    ImGui::Text("Manual cut: will auto-restore in %.0f s", 20.0f - h.cutTimer);
                }
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        // handle window resize events
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        winW = fbW;
        winH = fbH;
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}