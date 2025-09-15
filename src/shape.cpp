#include "shape.h" // Include the header file for the Shape class
#include <glm/gtc/matrix_transform.hpp> // Required for glm::translate, glm::scale (though not directly used in this version, good practice for transformations)

// Constructor for the Shape class
// It initializes the member variables with the provided data and then sets up the OpenGL buffers.
Shape::Shape(const std::vector<float>& vertices, // Vertex data
             const std::vector<GLuint>& indices,   // Index data
             glm::vec3 position,                   // Position of the shape
             float size,                           // Scale/size of the shape
             glm::vec3 color,                      // Color of the shape
             GLenum drawMode,                      // OpenGL drawing mode (e.g., GL_TRIANGLES, GL_LINES)
             GLuint textureID,                     // Texture ID
             bool useTexture)                      // Whether to use texture or color
    : position(position), size(size), color(color), textureID(textureID), useTexture(useTexture), drawMode(drawMode), vertices(vertices), indices(indices)
{
    setupBuffers(); // Call the helper function to set up VAO, VBO, EBO
}

// Private helper method to set up OpenGL buffers (VAO, VBO, EBO)
void Shape::setupBuffers()
{
    // Generate and bind a Vertex Array Object (VAO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); // Generate a Vertex Buffer Object (VBO)
    glGenBuffers(1, &EBO); // Generate an Element Buffer Object (EBO)

    glBindVertexArray(VAO); // Bind the VAO to make it the active one

    // Bind the VBO and send the vertex data to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Bind the EBO and send the index data to the GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Configure the vertex attributes (how OpenGL should interpret the vertex data)
    // Attribute 0 (position): 3 floats per vertex, GL_FLOAT type, not normalized, 5*sizeof(float) stride, no offset
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // Enable the vertex attribute array

    // Attribute 1 (texture coordinates): 2 floats per vertex, GL_FLOAT type, not normalized, 5*sizeof(float) stride, offset 3 floats
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1); // Enable the texture coordinate attribute array

    glBindVertexArray(0); // Unbind the VAO to prevent accidental modifications
}

// Draw method for the Shape
// It sets uniforms in the shader and then draws the shape.
void Shape::draw(Shader& shader)
{
    shader.use();

    // Set the uniform variables in the shader
    shader.setVec3("uOffset", position); // Pass the shape's position (offset) to the shader
    shader.setFloat("uScale", size);     // Pass the shape's size (scale) to the shader
    shader.setVec3("uColor", color);     // Pass the shape's color to the shader
    shader.setBool("useTexture", useTexture); // Pass whether to use texture

    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader.setInt("ourTexture", 0);
    }

    // Bind the VAO before drawing
    glBindVertexArray(VAO);
    // Draw the elements using the specified draw mode, number of indices, and data type
    glDrawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); // Unbind VAO

    if (useTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Destructor for the Shape class
// It cleans up the OpenGL resources (VAO, VBO, EBO) when the Shape object is destroyed.
Shape::~Shape()
{
    glDeleteVertexArrays(1, &VAO); // Delete the VAO
    glDeleteBuffers(1, &VBO);      // Delete the VBO
    glDeleteBuffers(1, &EBO);      // Delete the EBO
}
