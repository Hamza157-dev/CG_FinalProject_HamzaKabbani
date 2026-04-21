#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>

const int WIDTH = 800;
const int HEIGHT = 600;

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;
out vec3 fragPos;
out vec3 fragNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragPos = vec3(model * vec4(aPos, 1.0));
    vertexColor = aColor;
    // Simple normal: faces pointing up get full light
    fragNormal = vec3(0.0, 1.0, 0.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec3 fragPos;
in vec3 fragNormal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {
    // Ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * vertexColor;
    FragColor = vec4(result, 1.0);
}
)";

// ----------------------------
// Camera state
// ----------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ----------------------------
// Mouse callback
// ----------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y goes bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Prevent flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}



// ----------------------------
// Mesh
// ----------------------------
struct Mesh { GLuint VAO, VBO, EBO; };

struct Collectible {
    glm::vec3 position;
    bool collected = false;
};

std::vector<Collectible> collectibles = {
    { glm::vec3(2.0f, -0.5f,  1.0f) },
    { glm::vec3(-1.0f, -0.5f,  2.0f) },
    { glm::vec3(3.0f, -0.5f, -1.0f) },
    { glm::vec3(-0.5f, -0.5f, -1.0f) },
    { glm::vec3(0.0f, -0.5f,  3.5f) },
};

struct WallBox {
    float minX, maxX, minZ, maxZ;
};

std::vector<WallBox> walls = {
    // Outer walls
    {-5.0f, -4.7f, -5.0f,  5.0f},  // left
    { 4.7f,  5.0f, -5.0f,  5.0f},  // right
    {-5.0f,  5.0f, -5.0f, -4.7f},  // back
    {-5.0f,  5.0f,  4.7f,  5.0f},  // front
    // Inner walls
    {-3.0f, -2.7f, -2.0f,  2.0f},
    { 1.0f,  1.3f,  0.0f,  4.0f},
    {-1.0f,  3.0f, -4.0f, -3.7f},
    { 1.0f,  4.0f,  2.0f,  2.3f},
};

int score = 0;

Mesh createBox(float r, float g, float b) {
    float vertices[] = {
        -0.5f, 0.0f, -0.5f,  r, g, b,
         0.5f, 0.0f, -0.5f,  r, g, b,
         0.5f, 0.0f,  0.5f,  r, g, b,
        -0.5f, 0.0f,  0.5f,  r, g, b,
        -0.5f, 1.0f, -0.5f,  r * 0.8f, g * 0.8f, b * 0.8f,
         0.5f, 1.0f, -0.5f,  r * 0.8f, g * 0.8f, b * 0.8f,
         0.5f, 1.0f,  0.5f,  r * 0.8f, g * 0.8f, b * 0.8f,
        -0.5f, 1.0f,  0.5f,  r * 0.8f, g * 0.8f, b * 0.8f,
    };
    unsigned int indices[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,1,5, 5,4,0,
        2,3,7, 7,6,2,
        0,3,7, 7,4,0,
        1,2,6, 6,5,1
    };
    Mesh mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}



void drawMesh(Mesh& mesh, GLuint shader, glm::vec3 position, glm::vec3 scale) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


// ----------------------------
// Input: WASD movement
// ----------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 3.0f * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flatFront, cameraUp));

    glm::vec3 newPos = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) newPos += speed * flatFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) newPos -= speed * flatFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) newPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) newPos += right * speed;

    // AABB collision — player radius 0.3
    float radius = 0.3f;
    bool blocked = false;
    for (auto& w : walls) {
        if (newPos.x + radius > w.minX && newPos.x - radius < w.maxX &&
            newPos.z + radius > w.minZ && newPos.z - radius < w.maxZ) {
            blocked = true;
            break;
        }
    }

    if (!blocked)
        cameraPos = newPos;
}


GLuint compileShader() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Maze Game", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // enable vsync
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Hide and capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    GLuint shader = compileShader();
    Mesh floorMesh = createBox(0.4f, 0.7f, 0.4f);
    Mesh wallMesh = createBox(0.6f, 0.5f, 0.4f);
    Mesh collectMesh = createBox(1.0f, 0.84f, 0.0f); // gold color

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glUniform3f(glGetUniformLocation(shader, "lightPos"), 0.0f, 3.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shader, "lightColor"), 1.0f, 1.0f, 0.9f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Floor
        drawMesh(floorMesh, shader, glm::vec3(-5.0f, -1.0f, -5.0f), glm::vec3(10.0f, 0.1f, 10.0f));

        // Outer walls
        drawMesh(wallMesh, shader, glm::vec3(-5.0f, -1.0f, -5.0f), glm::vec3(0.3f, 2.0f, 10.0f));
        drawMesh(wallMesh, shader, glm::vec3(4.7f, -1.0f, -5.0f), glm::vec3(0.3f, 2.0f, 10.0f));
        drawMesh(wallMesh, shader, glm::vec3(-5.0f, -1.0f, -5.0f), glm::vec3(10.0f, 2.0f, 0.3f));
        drawMesh(wallMesh, shader, glm::vec3(-5.0f, -1.0f, 4.7f), glm::vec3(10.0f, 2.0f, 0.3f));

        // Inner maze walls
        drawMesh(wallMesh, shader, glm::vec3(-3.0f, -1.0f, -2.0f), glm::vec3(0.3f, 2.0f, 4.0f));
        drawMesh(wallMesh, shader, glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.3f, 2.0f, 4.0f));
        drawMesh(wallMesh, shader, glm::vec3(-1.0f, -1.0f, -4.0f), glm::vec3(4.0f, 2.0f, 0.3f));
        drawMesh(wallMesh, shader, glm::vec3(1.0f, -1.0f, 2.0f), glm::vec3(3.0f, 2.0f, 0.3f));

        // Update window title as HUD
        std::string title = "Maze Game | Score: " + std::to_string(score) + "/" + std::to_string(collectibles.size());

        // Check win condition
        if (score == (int)collectibles.size()) {
            title = "YOU WIN! All collectibles found! Press ESC to exit.";
        }

        glfwSetWindowTitle(window, title.c_str());

        // Draw collectibles
        for (auto& c : collectibles) {
            if (!c.collected) {
                drawMesh(collectMesh, shader, c.position, glm::vec3(0.4f, 0.4f, 0.4f));
            }
        }

        // Check collection
        for (auto& c : collectibles) {
            if (!c.collected) {
                glm::vec2 playerXZ = glm::vec2(cameraPos.x, cameraPos.z);
                glm::vec2 collectXZ = glm::vec2(c.position.x, c.position.z);
                float dist = glm::length(playerXZ - collectXZ);
                if (dist < 0.8f) {
                    c.collected = true;
                    score++;
                    std::cout << "Collected! Score: " << score << "/" << collectibles.size() << "\n";
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
