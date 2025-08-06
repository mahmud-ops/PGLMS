#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/glm.hpp>
#include <string>
#include <sstream> // For stringstream to format log messages
#include <iomanip> // For std::fixed and std::setprecision
#include <algorithm> // Required for std::remove_if
#include <cstdlib>   // For rand() and srand()
#include <ctime>     // For time()

// Include Dear ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "shader.h"
#include "shape.h"

#define M_PI 3.14159265358979323846

// --- Global Variables and Structs for Simulation State ---

enum HouseState {
    NORMAL,
    WARNING,
    OVERLOADED,
    POWER_CUT,
    COOLDOWN
};

struct HouseZone {
    Shape shape;
    glm::vec3 basePosition; // Store original position for reference
    float currentLoad;
    float maxLoad;
    float warningThreshold;
    float overloadThreshold;
    HouseState state;
    double stateChangeTime; // Time when the state last changed (for timers)
    bool showPowerCutPrompt; // Flag to show modal for this house
    bool isManualCut; // Was the power cut manual or automatic?
    std::string name; // Name for GUI display
};

struct AnimatedCircle {
    Shape shape;
    glm::vec3 startPos;
    glm::vec3 endPos;
    float pathDuration;
    float delayOffset;
    int targetHouseIndex; // To know which house this circle is going to
    bool isActive; // To control visibility/animation
};

// Global variables for ImGui and simulation
std::vector<std::string> logMessages;
std::vector<HouseZone> houseZones; // Vector to hold all house zones
std::vector<AnimatedCircle> animatedCircles; // Main flow circles
std::vector<AnimatedCircle> overloadCircles; // Circles spawned due to overload

// Global flags for modal
int houseIndexToCutPower = -1; // -1 if no house needs power cut prompt
double overloadPromptTime = 0.0; // Time when the overload prompt was triggered

// --- New Global Variables for Overload Delay ---
double lastOverloadEventTime = -15.0; // Initialize to allow immediate first overload
const double OVERLOAD_INTERVAL = 15.0; // 30 seconds between overload events

// --- Helper Functions ---

void AddLog(const std::string& message) {
    logMessages.push_back(message);
    if (logMessages.size() > 20) { // Keep only last 20 messages
        logMessages.erase(logMessages.begin());
    }
}

// Function to spawn additional circles for an overloaded house
void SpawnOverloadCircles(int houseIdx, const std::vector<float>& circleVertices, const std::vector<GLuint>& circleIndices, float circleSize, glm::vec3 circleColor) {
    glm::vec3 txPos;
    // Determine the correct transmitter position based on house index
    if (houseIdx == 0 || houseIdx == 1) { // House 1 and 2 are connected to Transmitter 1
        // Transmitter 1 is at (-0.4f, 0.0f, 0.0f) with a height scale of 0.2f and top point at 1.5f * 0.2f
        txPos = glm::vec3(-0.4f, 0.0f, 0.0f) + glm::vec3(0.0f, 1.5f * 0.2f, 0.0f); // Top of Tx1
    } else { // House 3 and 4 are connected to Transmitter 2
        // Transmitter 2 is at (0.4f, 0.3f, 0.0f) with a height scale of 0.2f and top point at 1.5f * 0.2f
        txPos = glm::vec3(0.4f, 0.3f, 0.0f) + glm::vec3(0.0f, 1.5f * 0.2f, 0.0f); // Top of Tx2
    }

    // Spawn 2 circles
    for (int i = 0; i < 2; ++i) {
        overloadCircles.push_back({
            Shape(circleVertices, circleIndices, txPos, circleSize, circleColor),
            txPos,
            houseZones[houseIdx].basePosition,
            1.0f, // Faster duration for overload circles
            (float)i * 0.5f, // Stagger them
            houseIdx,
            true
        });
    }
    AddLog("Spawned 2 overload circles for " + houseZones[houseIdx].name);
}

// Function to remove circles going to a specific house
void ClearOverloadCirclesForHouse(int houseIdx) {
    overloadCircles.erase(
        std::remove_if(overloadCircles.begin(), overloadCircles.end(),
                       [houseIdx](const AnimatedCircle& circle) {
                           return circle.targetHouseIndex == houseIdx;
                       }),
        overloadCircles.end());
    AddLog("Cleared overload circles for " + houseZones[houseIdx].name);
}


void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // Initialize random seed for house selection
    srand(time(NULL));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(960, 540, "Mahmud's OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // --- End ImGui Initialization ---

    Shader shader("Shaders/default.vs", "Shaders/default.fs");

    // --- Define vertices and indices for various static shapes ---
    std::vector<float> sourceVertices = {
        -0.5f, 0.0f, 0.0f,
        0.5f, 0.0f, 0.0f,
        0.3f, 0.3f, 0.0f,
        0.2f, 0.6f, 0.0f,
        0.2f, 1.0f, 0.0f,
        -0.2f, 1.0f, 0.0f,
        -0.2f, 0.6f, 0.0f,
        -0.3f, 0.3f, 0.0f,
    };
    std::vector<GLuint> sourceIndices = 
      { 0, 1, 2, 
        2, 7, 0, 
        7, 2, 3, 
        3, 6, 7, 
        6, 3, 4, 
        4, 5, 6 };

    std::vector<float> transmissionVertices = {
        0.0f, 0.0f, 0.0f, 
        0.25f, 0.0f, 0.0f, 
        0.125f, 0.75f, 0.0f, 
        0.0f, 1.5f, 0.0f,
        -0.125f, 0.75f, 0.0f, 
        -0.25f, 0.0f, 0.0f, 
        0.0f, 0.2f, 0.0f
    };
    std::vector<GLuint> transmissionIndices = 
      { 5, 4, 3, 
        5, 3, 2, 
        5, 2, 6, 
        6, 2, 1 };

    // --- MODIFIED: House shape as rectangles ---
    std::vector<float> houseVertices = {
        -0.5f, 0.0f, 0.0f, // 0: Bottom-left
         0.5f, 0.0f, 0.0f, // 1: Bottom-right
         0.5f, 0.5f, 0.0f, // 2: Top-right
        -0.5f, 0.5f, 0.0f  // 3: Top-left
    };
    std::vector<GLuint> houseIndices = { 
        0, 1, 2, // First triangle
        0, 2, 3  // Second triangle
    };

    std::vector<float> wireVertices = {
        -0.8f, 0.3f, 0.0f, -0.4f, 0.3f, 0.0f, // Generator to Tx1
        -0.4f, 0.3f, 0.0f, -0.6f, -0.4f, 0.0f, // Tx1 to House 1
        -0.4f, 0.3f, 0.0f, -0.2f, -0.4f, 0.0f, // Tx1 to House 2
        0.4f, 0.6f, 0.0f, 0.2f, -0.4f, 0.0f, // Tx2 to House 3
        0.4f, 0.6f, 0.0f, 0.6f, -0.4f, 0.0f, // Tx2 to House 4
        -0.8f, 0.3f, 0.0f, 0.4f, 0.6f, 0.0f // Generator to Tx2
    };

    std::vector<GLuint> wireIndices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    Shape wires(wireVertices, wireIndices, glm::vec3(0.0f), 1.0f, glm::vec3(0, 0, 0), GL_LINES);

    // --- Circle definition (base for all animated circles) ---
    std::vector<float> circleVertices;
    std::vector<GLuint> circleIndices;
    const int segments = 50;
    const float radius = 0.5f;
    circleVertices.push_back(0.0f); circleVertices.push_back(0.0f); circleVertices.push_back(0.0f); // Center
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        circleVertices.push_back(radius * cos(angle));
        circleVertices.push_back(radius * sin(angle));
        circleVertices.push_back(0.0f);
    }
    for (int i = 1; i <= segments; i++) {
        circleIndices.push_back(0); // Center
        circleIndices.push_back(i);
        circleIndices.push_back(i + 1 > segments ? 1 : i + 1); // Connect to next segment, or back to 1 for last segment
    }

    // --- Create static Shape objects for all scene elements ---
    Shape generatorShape(sourceVertices, sourceIndices, glm::vec3(-0.8f, 0.3f, 0.0f), 0.3f, glm::vec3(0.5f, 0.5f, 0.5f));
    Shape transmitter1Shape(transmissionVertices, transmissionIndices, glm::vec3(-0.4f, 0.0f, 0.0f), 0.2f, glm::vec3(0.36f, 0.25f, 0.20f));
    Shape transmitter2Shape(transmissionVertices, transmissionIndices, glm::vec3(0.4f, 0.3f, 0.0f), 0.2f, glm::vec3(0.36f, 0.25f, 0.20f));

    // --- Initialize House Zones ---
    // Note: The scale for houses is 0.2f, so the actual size will be 0.2 * 1.0 (width) by 0.2 * 0.5 (height)
    houseZones.push_back({Shape(houseVertices, houseIndices, glm::vec3(-0.6f, -0.4f, 0.0f), 0.2f, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(-0.6f, -0.4f, 0.0f), 0.5f, 1.0f, 0.6f, 0.9f, NORMAL, 0.0, false, false, "House 1"});
    houseZones.push_back({Shape(houseVertices, houseIndices, glm::vec3(-0.2f, -0.4f, 0.0f), 0.2f, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(-0.2f, -0.4f, 0.0f), 0.5f, 1.0f, 0.6f, 0.9f, NORMAL, 0.0, false, false, "House 2"});
    houseZones.push_back({Shape(houseVertices, houseIndices, glm::vec3(0.2f, -0.4f, 0.0f), 0.2f, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.2f, -0.4f, 0.0f), 0.5f, 1.0f, 0.6f, 0.9f, NORMAL, 0.0, false, false, "House 3"});
    houseZones.push_back({Shape(houseVertices, houseIndices, glm::vec3(0.6f, -0.4f, 0.0f), 0.2f, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.6f, -0.4f, 0.0f), 0.5f, 1.0f, 0.6f, 0.9f, NORMAL, 0.0, false, false, "House 4"});

    // --- Animation setup: Define positions for paths ---
    glm::vec3 generatorPos = glm::vec3(-0.8f, 0.3f, 0.0f);
    glm::vec3 transmitter1BasePos = glm::vec3(-0.4f, 0.0f, 0.0f);
    glm::vec3 transmitter2BasePos = glm::vec3(0.4f, 0.3f, 0.0f);

    // Calculate the top positions of transmitters accurately
    // Transmitter shape has height 1.5 units relative to its local origin, scaled by 0.2f
    glm::vec3 transmitter1TopPos = transmitter1BasePos + glm::vec3(0.0f, 1.5f * 0.2f, 0.0f); // Top of Tx1
    glm::vec3 transmitter2TopPos = transmitter2BasePos + glm::vec3(0.0f, 1.5f * 0.2f, 0.0f); // Top of Tx2

    float circleSize = 0.05f;
    glm::vec3 circleColor = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow

    // Phase 1: Generator to Transmitters (8 circles total, 4 to each)
    float genToTxDuration = 2.0f;
    float genToTxStagger = genToTxDuration / 4.0f;

    for (int i = 0; i < 4; ++i) {
        animatedCircles.push_back({Shape(circleVertices, circleIndices, generatorPos, circleSize, circleColor), generatorPos, transmitter1TopPos, genToTxDuration, (float)i * genToTxStagger, -1, true});
        animatedCircles.push_back({Shape(circleVertices, circleIndices, generatorPos, circleSize, circleColor), generatorPos, transmitter2TopPos, genToTxDuration, (float)i * genToTxStagger, -1, true});
    }

    // Phase 2: Transmitters to Houses (8 circles total, 2 to each house)
    float txToHouseDuration = 1.5f;
    float txToHouseStagger = txToHouseDuration / 2.0f;

    for (int i = 0; i < 2; ++i) {
        animatedCircles.push_back({Shape(circleVertices, circleIndices, transmitter1TopPos, circleSize, circleColor), transmitter1TopPos, houseZones[0].basePosition, txToHouseDuration, (float)i * txToHouseStagger + genToTxDuration, 0, true});
        animatedCircles.push_back({Shape(circleVertices, circleIndices, transmitter1TopPos, circleSize, circleColor), transmitter1TopPos, houseZones[1].basePosition, txToHouseDuration, (float)i * txToHouseStagger + genToTxDuration, 1, true});
        animatedCircles.push_back({Shape(circleVertices, circleIndices, transmitter2TopPos, circleSize, circleColor), transmitter2TopPos, houseZones[2].basePosition, txToHouseDuration, (float)i * txToHouseStagger + genToTxDuration, 2, true});
        animatedCircles.push_back({Shape(circleVertices, circleIndices, transmitter2TopPos, circleSize, circleColor), transmitter2TopPos, houseZones[3].basePosition, txToHouseDuration, (float)i * txToHouseStagger + genToTxDuration, 3, true});
    }

    glClearColor(0.1f, 0.3f, 0.15f, 1.0f);

    // Initial log message
    AddLog("Simulation started.");

    // --- Main rendering loop ---
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double currentTime = glfwGetTime(); // Use double for time for consistency

        // --- Overload Management Logic (New) ---
        // This ensures only one house goes into OVERLOADED state every OVERLOAD_INTERVAL seconds
        if (currentTime - lastOverloadEventTime >= OVERLOAD_INTERVAL) {
            lastOverloadEventTime = currentTime; // Reset timer for the next overload event

            // Reset all houses to NORMAL if they are not in POWER_CUT/COOLDOWN
            // and clear any pending prompts or overload circles from previous cycles
            for (int i = 0; i < houseZones.size(); ++i) {
                HouseZone& house = houseZones[i];
                if (house.state != POWER_CUT && house.state != COOLDOWN) {
                    if (house.state != NORMAL) { // Only log if actually changing state
                        AddLog(house.name + " reset to NORMAL for new cycle.");
                    }
                    house.state = NORMAL;
                    house.showPowerCutPrompt = false;
                    ClearOverloadCirclesForHouse(i); // Clear any lingering overload circles
                }
            }
            houseIndexToCutPower = -1; // Ensure no modal is active from previous cycle

            // Select a random house that is currently in NORMAL or WARNING state to overload
            std::vector<int> availableHouseIndices;
            for (int i = 0; i < houseZones.size(); ++i) {
                if (houseZones[i].state == NORMAL || houseZones[i].state == WARNING) {
                    availableHouseIndices.push_back(i);
                }
            }

            if (!availableHouseIndices.empty()) {
                int randomIndex = rand() % availableHouseIndices.size();
                int houseToOverloadIndex = availableHouseIndices[randomIndex];

                HouseZone& houseToOverload = houseZones[houseToOverloadIndex];
                houseToOverload.state = OVERLOADED;
                houseToOverload.stateChangeTime = currentTime;
                houseToOverload.showPowerCutPrompt = true;
                houseToOverload.isManualCut = false; // It's an automatic overload trigger
                houseIndexToCutPower = houseToOverloadIndex; // Set global index for modal
                AddLog("FORCING " + houseToOverload.name + " into OVERLOADED state.");
                SpawnOverloadCircles(houseToOverloadIndex, circleVertices, circleIndices, circleSize, circleColor);
            } else {
                AddLog("No available houses to overload. All are in POWER_CUT or COOLDOWN.");
            }
        }


        // --- Simulation Logic: Update House Loads and States (Modified) ---
        for (int i = 0; i < houseZones.size(); ++i) {
            HouseZone& house = houseZones[i];

            // Only fluctuate load if not in power cut
            if (house.state != POWER_CUT) {
                // Dynamic load fluctuation (using sine wave with random offset for variety)
                // This still runs, but the primary state transition to OVERLOADED is now controlled by the new logic above.
                float fluctuationFactor = (sin(currentTime * (0.5f + i * 0.1f) + (float)i * 2.0f) + 1.0f) / 2.0f; // 0.0 to 1.0
                house.currentLoad = house.maxLoad * (0.3f + 0.7f * fluctuationFactor); // Load between 30% and 100% of maxLoad
                house.currentLoad = glm::clamp(house.currentLoad, 0.0f, house.maxLoad); // Ensure it stays within bounds
            } else {
                house.currentLoad = 0.0f; // No load during power cut
            }

            // State transitions (modified to react to current state, not initiate OVERLOAD/WARNING from load)
            glm::vec3 newColor = house.shape.color; // Keep current color by default

            switch (house.state) {
                case NORMAL:
                    // Houses are primarily set to NORMAL by the new overload management logic or from COOLDOWN.
                    // Keep the color strictly green when in NORMAL state.
                    newColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green
                    break;

                case WARNING:
                    // This state is now mostly for visual feedback or during a transition
                    // if a house was previously overloaded and its load dropped, or if
                    // the new overload management logic explicitly sets it to WARNING.
                    newColor = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow
                    // If load drops below warning, it can go back to NORMAL.
                    if (house.currentLoad < house.warningThreshold) {
                        house.state = NORMAL;
                        house.stateChangeTime = currentTime;
                        AddLog(house.name + " returned to NORMAL from WARNING (load dropped).");
                    }
                    break;

                case OVERLOADED:
                    newColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red
                    // Automatic power cut after 10 seconds if no manual action AND prompt is not currently active
                    if (!house.showPowerCutPrompt && (currentTime - house.stateChangeTime >= 10.0)) {
                        house.state = POWER_CUT;
                        house.stateChangeTime = currentTime;
                        house.isManualCut = false;
                        AddLog(house.name + ": Automatic power cut due to prolonged overload.");
                        ClearOverloadCirclesForHouse(i); // Clear circles on power cut
                    }
                    break;

                case POWER_CUT:
                    newColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark Gray
                    if (currentTime - house.stateChangeTime >= 5.0) { // 5-second cooldown
                        house.state = COOLDOWN;
                        house.stateChangeTime = currentTime;
                        AddLog(house.name + ": Power cut cooldown started.");
                    }
                    break;

                case COOLDOWN:
                    newColor = glm::vec3(0.5f, 0.5f, 0.5f); // Gray (during cooldown)
                    if (currentTime - house.stateChangeTime >= 5.0) { // Another 5 seconds for cooldown to finish
                        house.state = NORMAL; // Return to normal after cooldown
                        house.stateChangeTime = currentTime;
                        AddLog(house.name + ": Power restored. Returning to NORMAL.");
                    }
                    break;
            }
            house.shape.color = newColor; // Update house color
        }

        // --- ImGui UI Rendering ---
        ImGui::Begin("Power Grid Controls");
        ImGui::Text("Simulation Parameters");
        ImGui::Separator();

        // Display controls for each house zone
        for (int i = 0; i < houseZones.size(); ++i) {
            HouseZone& house = houseZones[i];
            ImGui::PushID(i); // Unique ID for each house's widgets

            ImGui::Text("%s (Load: %.0f%%)", house.name.c_str(), house.currentLoad * 100.0f);
            ImGui::SameLine();

            // Display state with color
            ImVec4 stateColor;
            switch (house.state) {
                case NORMAL: stateColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break; // Green
                case WARNING: stateColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
                case OVERLOADED: stateColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Red
                case POWER_CUT: stateColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break; // Dark Gray
                case COOLDOWN: stateColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; // Light Gray
            }
            ImGui::TextColored(stateColor, "State: %s",
                               (house.state == NORMAL ? "NORMAL" :
                                house.state == WARNING ? "WARNING" :
                                house.state == OVERLOADED ? "OVERLOADED" :
                                house.state == POWER_CUT ? "POWER CUT" : "COOLDOWN"));
            ImGui::SameLine();

            if (house.state == OVERLOADED && ImGui::Button("Manual Shed")) {
                house.state = POWER_CUT;
                house.stateChangeTime = currentTime;
                house.isManualCut = true;
                house.showPowerCutPrompt = false; // Close prompt if open
                AddLog(house.name + ": Manual power cut initiated.");
                ClearOverloadCirclesForHouse(i); // Clear circles on manual power cut
            } else if (house.state == POWER_CUT || house.state == COOLDOWN) {
                ImGui::Text("Power Off"); // Indicate power is off
            } else {
                ImGui::Text("         "); // Placeholder for alignment
            }

            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // --- Power Cut Confirmation Modal ---
        // Only open the popup if a house needs a prompt AND it's not already open for this house
        if (houseIndexToCutPower != -1 && houseZones[houseIndexToCutPower].showPowerCutPrompt && !ImGui::IsPopupOpen("Power Cut Confirmation")) {
            ImGui::OpenPopup("Power Cut Confirmation");
            overloadPromptTime = currentTime; // Record time when modal opened
        }

        if (ImGui::BeginPopupModal("Power Cut Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            HouseZone& house = houseZones[houseIndexToCutPower]; // Use the global index
            ImGui::Text("House %s is overloaded!", house.name.c_str());
            ImGui::Text("Do you want to cut power to prevent damage?");

            if (ImGui::Button("Yes, Cut Power", ImVec2(120, 0))) {
                house.state = POWER_CUT;
                house.stateChangeTime = currentTime;
                house.isManualCut = true;
                house.showPowerCutPrompt = false; // Close prompt
                AddLog(house.name + ": Manual power cut confirmed.");
                ClearOverloadCirclesForHouse(houseIndexToCutPower); // Clear circles
                houseIndexToCutPower = -1; // Reset global index
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No, Continue", ImVec2(120, 0))) {
                house.showPowerCutPrompt = false; // Dismiss prompt
                AddLog(house.name + ": Manual power cut declined. Monitoring...");
                houseIndexToCutPower = -1; // Reset global index
                ImGui::CloseCurrentPopup();
            }

            // The automatic power cut logic (10 seconds timeout) is handled in the main simulation loop
            // for the OVERLOADED state, after the prompt is dismissed (showPowerCutPrompt = false).
            // This ensures consistent behavior whether the user clicks "No" or ignores the prompt.

            ImGui::EndPopup();
        }


        // --- Simulation Log Window ---
        ImGui::Begin("Simulation Log");
        for (const auto& msg : logMessages) {
            ImGui::TextUnformatted(msg.c_str());
        }
        ImGui::End();


        glClear(GL_COLOR_BUFFER_BIT); // Clear OpenGL buffer
        shader.use(); // Use your custom shader

        // --- Update and draw all animated circles ---
        for (auto& animatedCircle : animatedCircles) {
            // Check if the target house is in power cut, if so, hide the circle
            // Only hide if the target house is valid and in POWER_CUT state
            if (animatedCircle.targetHouseIndex != -1 && houseZones[animatedCircle.targetHouseIndex].state == POWER_CUT) {
                animatedCircle.shape.size = 0.0f; // Make it disappear
            } else {
                animatedCircle.shape.size = circleSize; // Restore size if not cut
            }

            // Use double for fmod with currentTime
            double cycleTime = fmod(currentTime + animatedCircle.delayOffset, animatedCircle.pathDuration);
            float progress = static_cast<float>(cycleTime / animatedCircle.pathDuration); // Cast to float for glm::vec3 interpolation

            glm::vec3 currentCirclePos = animatedCircle.startPos + (animatedCircle.endPos - animatedCircle.startPos) * progress;

            animatedCircle.shape.position = currentCirclePos;
            animatedCircle.shape.draw(shader);
        }

        // --- Update and draw overload circles ---
        for (auto& overloadCircle : overloadCircles) {
            if (overloadCircle.isActive) {
                // Use double for fmod with currentTime
                double cycleTime = fmod(currentTime + overloadCircle.delayOffset, overloadCircle.pathDuration);
                float progress = static_cast<float>(cycleTime / overloadCircle.pathDuration); // Cast to float for glm::vec3 interpolation

                glm::vec3 currentCirclePos = overloadCircle.startPos + (overloadCircle.endPos - overloadCircle.startPos) * progress;

                overloadCircle.shape.position = currentCirclePos;
                overloadCircle.shape.size = circleSize * 1.5f; // Make overload circles slightly larger
                overloadCircle.shape.draw(shader);
            }
        }


        // Draw static scene elements
        wires.draw(shader);
        generatorShape.draw(shader);
        transmitter1Shape.draw(shader);
        transmitter2Shape.draw(shader);

        // Draw house shapes (their colors are updated based on load)
        for (auto& house : houseZones) {
            house.shape.draw(shader);
        }

        // Render ImGui draw data (always last to be on top)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- ImGui Shutdown ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // --- End ImGui Shutdown ---

    glfwTerminate();
    return 0;
}
