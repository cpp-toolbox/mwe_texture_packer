#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // Add stb_image for texture loading
#include "graphics/texture_packer/texture_packer.hpp"

float positions[] = {
    // Square 0 (left, centered at x = -1.0)
    -1.0f, 0.25f, 0.0f,  // Top-left
    -1.0f, -0.25f, 0.0f, // Bottom-left
    -0.5f, -0.25f, 0.0f, // Bottom-right
    -0.5f, 0.25f, 0.0f,  // Top-right

    // Square 1 (centered, centered at x = 0.0)
    -0.25f, 0.25f, 0.0f,  // Top-left
    -0.25f, -0.25f, 0.0f, // Bottom-left
    0.25f, -0.25f, 0.0f,  // Bottom-right
    0.25f, 0.25f, 0.0f,   // Top-right

    // Square 2 (right, centered at x = 1.0)
    0.5f, 0.25f, 0.0f,  // Top-left
    0.5f, -0.25f, 0.0f, // Bottom-left
    1.0f, -0.25f, 0.0f, // Bottom-right
    1.0f, 0.25f, 0.0f   // Top-right
};

// Function to populate texCoords array using TexturePacker
std::pair<std::vector<float>, std::vector<unsigned int>>
generate_tex_coords(TexturePacker &texture_packer, const std::vector<std::string> &file_paths) {
    std::vector<float> tex_coords;
    std::vector<unsigned int> texture_indices;
    int count = 0;
    for (const auto &file_path : file_paths) {
        PackedTextureSubTexture texture;
        if (count == 1) {
            texture = texture_packer.get_packed_texture_sub_texture_atlas("assets/fonts/times_64_sdf_atlas.png", "A");
        } else {
            texture = texture_packer.get_packed_texture_sub_texture(file_path);
        }

        // Each sub-texture has a vector of 4 glm::vec2 representing the corners
        for (const auto &coord : texture.texture_coordinates) {
            tex_coords.push_back(coord.x);
            tex_coords.push_back(coord.y);
        }

        for (int i = 0; i < 4; i++) {
            texture_indices.push_back(texture.packed_texture_index);
        }
        count++;
    }

    return {tex_coords, texture_indices};
}

unsigned int indices[] = {
    0, 1, 2,  3,  0, 2, // Square 0
    4, 5, 6,  7,  4, 6, // Square 1
    8, 9, 10, 11, 8, 10 // Square 2
};

// Vertex shader source code
const char *vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    layout(location = 2) in float aTexIndex;

    out vec2 TexCoord;
    flat out int TexIndex;

    void main()
    {
        TexCoord = aTexCoord;
        TexIndex = int(aTexIndex);  // Pass texture index as an integer
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); 
    }
)";

// Fragment shader source code
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    flat in int TexIndex;

    uniform sampler2DArray textureArray;

    void main()
    {
        FragColor = texture(textureArray, vec3(TexCoord, TexIndex)); // Use array layer TexIndex
    }
)";

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // Set the OpenGL viewport to match the new window size
    glViewport(0, 0, width, height);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use the core profile

    // Create window
    GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL Texture Array", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    TexturePacker texture_packer("assets/packed_textures/packed_texture.json",
                                 {"assets/packed_textures/packed_texture_0.png",
                                  "assets/packed_textures/packed_texture_1.png",
                                  "assets/packed_textures/packed_texture_2.png"});

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Vertex Array Object setup
    GLuint VAO, VBO_positions, VBO_texCoords, VBO_textureIndices, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_positions);
    glGenBuffers(1, &VBO_texCoords);
    glGenBuffers(1, &VBO_textureIndices);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Position buffer setup
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // File paths to generate texCoords for
    std::vector<std::string> file_paths = {
        "assets/textures/SBS - Tiny Texture Pack - 512x512/512x512/Grass/Grass_10-512x512.png",
        "assets/fonts/times_64_sdf_atlas.png",
        "assets/textures/SBS - Tiny Texture Pack - 512x512/512x512/Grass/Grass_11-512x512.png"};

    // Generate texture coordinates array
    auto [texCoords, texIndices] = generate_tex_coords(texture_packer, file_paths);

    for (const auto &f : texIndices) {
        std::cout << f << ", " << std::endl;
    }

    // Texture coordinates buffer setup
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texCoords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texCoords.size(), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    // Texture indices buffer setup
    glBindBuffer(GL_ARRAY_BUFFER, VBO_textureIndices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * texIndices.size(), texIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(unsigned int), (void *)0);
    glEnableVertexAttribArray(2);

    // Element buffer setup
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    texture_packer.bind_texture_array();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
