#ifndef SHAPE_H
#define SHAPE_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../src/shader.h" // Assuming shader.h is in the parent directory of shape.h

class Shape
{
public:
    // Constructor to initialize shape properties and OpenGL buffers
    Shape(const std::vector<float>& vertices,
          const std::vector<GLuint>& indices,
          glm::vec3 position,
          float size,
          glm::vec3 color,
          GLenum drawMode = GL_TRIANGLES); // Default draw mode is triangles

    // Method to draw the shape using the provided shader
    void draw(Shader& shader);

    // Destructor to clean up OpenGL resources
    ~Shape();

    // Public member variables for position, size, and color
    // These are made public so they can be directly modified for animation
    glm::vec3 position;
    float size;
    glm::vec3 color; // Moved to public section

private:
    // OpenGL buffer IDs
    GLuint VAO, VBO, EBO;

    // Store original vertex and index data for buffer setup
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    // Drawing mode
    GLenum drawMode;

    // Helper method to set up OpenGL VAO, VBO, and EBO
    void setupBuffers();
};

#endif // SHAPE_H
