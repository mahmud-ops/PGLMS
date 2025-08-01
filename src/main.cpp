#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

// Vertex Shader Source Code
const char *vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
})";

// Fragment Shader Source Code
const char *fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);
})";

// Utility to check shader compile/link errors
void checkCompileErrors(GLuint shader, string type)
{
    int success;
    char infoLog[512];

    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            cerr << type << " Shader Compilation Error:\n"
                 << infoLog << endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            cerr << "Shader Linking Error:\n"
                 << infoLog << endl;
        }
    }
}

// Callback to resize viewport with window
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height); // adjust viewport on window resize
}

int main()
{
    // Initialize GLFW and set OpenGL version
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL 3.x
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Core profile = no deprecated functions

    // Create window and OpenGL context
    GLFWwindow *window = glfwCreateWindow(960, 540, "Mahmud's OpenGL", NULL, NULL);
    if (window == NULL)
    {
        cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL function pointers using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Vertex positions (6 points)
    float vertices[] = {
        -0.25f, 0.0f, 0.0f,   // left
        0.25f, 0.0f, 0.0f,    // right
        0.0f, 0.7f, 0.0f,     // top
        0.125f, 0.35f, 0.0f,  // inner-right
        -0.125f, 0.35f, 0.0f, // inner-left
        0.0f, 0.0f, 0.0f      // inner-bottom
    };

    // Indices to form three triangles
    GLuint indices[] = {
        0, 5, 4, // lower left triangle
        1, 5, 3, // lower right triangle
        2, 3, 4  // upper triangle
    };

    // Vertex Shader setup
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    // Fragment Shader setup
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    // Shader Program linking
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    // Clean up shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate and bind VAO
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Generate and bind VBO, upload vertex data
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Generate and bind EBO, upload index data
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind VBO and VAO (EBO stays bound to VAO!)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set the background clear color
    glClearColor(0.1f, 0.3f, 0.5f, 1.0f);

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT); // Clear screen
        glUseProgram(shaderProgram);  // Use custom shader

        glBindVertexArray(VAO);                              // Bind geometry
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0); // Draw using indices

        glfwSwapBuffers(window); // Show frame
        glfwPollEvents();        // Handle window events
    }

    // Cleanup GPU resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Destroy window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}