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

// Room boundaries — player stays inside these
const float ROOM_MIN = -9.5f;
const float ROOM_MAX = 9.5f;

float timeRemaining = 60.0f;
bool  gameover = false;
bool  gameWon = false;
int   score = 0;

// ----------------------------
// Shaders
// ----------------------------
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
    fragPos     = vec3(model * vec4(aPos, 1.0));
    vertexColor = aColor;
    fragNormal  = vec3(0.0, 1.0, 0.0);
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
    vec3 ambient  = 0.45 * lightColor;
    vec3 norm     = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;
    FragColor = vec4((ambient + diffuse) * vertexColor, 1.0);
}
)";

// ----------------------------
// Camera
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoff = (xpos - lastX) * 0.1f;
    float yoff = (lastY - ypos) * 0.1f;
    lastX = xpos; lastY = ypos;
    yaw += xoff;
    pitch += yoff;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

// ----------------------------
// Input — clamp to room bounds
// ----------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 4.0f * deltaTime;
    glm::vec3 flat = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flat, cameraUp));
    glm::vec3 newPos = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) newPos += speed * flat;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) newPos -= speed * flat;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) newPos -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) newPos += speed * right;

    // Inner walls (centered positions from drawMesh):
    // Wall A: center(-5, 0.5, -5), scale(8, 3, 0.5) -> X from -9 to -1, Z from -5.25 to -4.75
    if (newPos.z > -5.25f && newPos.z < -4.75f && newPos.x > -9.0f && newPos.x < -1.0f)
        newPos.z = cameraPos.z;
    // Wall B: center(2, 0.5, 5), scale(8, 3, 0.5) -> X from -2 to 6, Z from 4.75 to 5.25
    if (newPos.z > 4.75f && newPos.z < 5.25f && newPos.x > -2.0f && newPos.x < 6.0f)
        newPos.z = cameraPos.z;
    // Wall C: center(-5, 0.5, -5), scale(0.5, 3, 8) -> X from -5.25 to -4.75, Z from -9 to -1
    if (newPos.x > -5.25f && newPos.x < -4.75f && newPos.z > -9.0f && newPos.z < -1.0f)
        newPos.x = cameraPos.x;
    // Wall D: center(5, 0.5, 2), scale(0.5, 3, 8) -> X from 4.75 to 5.25, Z from -2 to 6
    if (newPos.x > 4.75f && newPos.x < 5.25f && newPos.z > -2.0f && newPos.z < 6.0f)
        newPos.x = cameraPos.x;

    // Clamp inside room (do this BEFORE assigning)
    float r = 0.4f;
    newPos.x = glm::clamp(newPos.x, ROOM_MIN + r, ROOM_MAX - r);
    newPos.z = glm::clamp(newPos.z, ROOM_MIN + r, ROOM_MAX - r);

    cameraPos = newPos;
}

// ----------------------------
// Mesh
// ----------------------------
struct Mesh { GLuint VAO, VBO, EBO; };

// Builds a box: bottom-left-back corner at (-0.5, 0, -0.5), top at (0.5, 1, 0.5)
// Apply position + scale via model matrix in drawMesh
Mesh createBox(float r, float g, float b) {
    float v[] = {
            -0.5f, -0.5f, -0.5f,  r, g, b,
             0.5f, -0.5f, -0.5f,  r, g, b,
             0.5f, -0.5f,  0.5f,  r, g, b,
            -0.5f, -0.5f,  0.5f,  r, g, b,
            -0.5f,  0.5f, -0.5f,  r * 0.75f, g * 0.75f, b * 0.75f,
             0.5f,  0.5f, -0.5f,  r * 0.75f, g * 0.75f, b * 0.75f,
             0.5f,  0.5f,  0.5f,  r * 0.75f, g * 0.75f, b * 0.75f,
            -0.5f,  0.5f,  0.5f,  r * 0.75f, g * 0.75f, b * 0.75f,
    };
    unsigned int idx[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,1,5, 5,4,0,
        2,3,7, 7,6,2,
        0,3,7, 7,4,0,
        1,2,6, 6,5,1
    };
    Mesh m;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glGenBuffers(1, &m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return m;
}

// position = bottom-left-back corner of the box
void drawMesh(Mesh& mesh, GLuint shader, glm::vec3 center, glm::vec3 scale) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

GLuint compileShader() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ----------------------------
// Collectibles
// ----------------------------
struct Collectible { glm::vec3 pos; bool collected = false; };

std::vector<Collectible> collectibles = {
    { glm::vec3( 7.0f, -0.6f,  7.0f) },  // top-right zone
    { glm::vec3(-7.0f, -0.6f,  7.0f) },  // top-left zone
    { glm::vec3( 7.0f, -0.6f, -7.0f) },  // bottom-right zone
    { glm::vec3(-7.0f, -0.6f, -7.0f) },  // bottom-left zone
    { glm::vec3( 0.0f, -0.6f,  3.0f) },  // center corridor
};

// ----------------------------
// Main
// ----------------------------
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // REQUIRED on Mac

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Maze Game", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    GLuint shader = compileShader();

    Mesh floorMesh = createBox(0.35f, 0.65f, 0.35f); // green
    Mesh wallMesh = createBox(0.55f, 0.45f, 0.35f); // tan
    Mesh ceilMesh = createBox(0.50f, 0.50f, 0.55f); // grey-blue ceiling
    Mesh collectMesh = createBox(1.0f, 0.80f, 0.0f);  // gold

    glm::mat4 projection = glm::perspective(
        glm::radians(70.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    // Room is 20x20: from -10 to +10 on X and Z
    // Floor  Y: -1.0 (bottom of box) to -0.9 (top, scale Y=0.1)
    // Walls  Y: -1.0 (bottom) to  2.0 (top, scale Y=3.0)
    // Ceiling Y: 2.0 to 2.1

    while (!glfwWindowShouldClose(window)) {
        float now = glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);

        glClearColor(0.15f, 0.15f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shader, "lightPos"), 0.0f, 3.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shader, "lightColor"), 1.0f, 1.0f, 0.9f);

        // Floor: center at (0, -1, 0)
        drawMesh(floorMesh, shader, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(20.0f, 0.1f, 20.0f));

        // Ceiling: center at (0, 2, 0)
        drawMesh(ceilMesh, shader, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(20.0f, 0.1f, 20.0f));

        // Left wall:  center at (-10, 0.5, 0)
        drawMesh(wallMesh, shader, glm::vec3(-10.0f, 0.5f, 0.0f), glm::vec3(0.5f, 3.0f, 20.0f));

        // Right wall: center at (+10, 0.5, 0)
        drawMesh(wallMesh, shader, glm::vec3(10.0f, 0.5f, 0.0f), glm::vec3(0.5f, 3.0f, 20.0f));

        // Back wall:  center at (0, 0.5, -10)
        drawMesh(wallMesh, shader, glm::vec3(0.0f, 0.5f, -10.0f), glm::vec3(20.0f, 3.0f, 0.5f));

        // Front wall: center at (0, 0.5, +10)
        drawMesh(wallMesh, shader, glm::vec3(0.0f, 0.5f, 10.0f), glm::vec3(20.0f, 3.0f, 0.5f));

        // Inner maze walls WITH GAPS
           // Horizontal wall at Z=-5: left half only (gap on right side)
        drawMesh(wallMesh, shader, glm::vec3(-5.0f, 0.5f, -5.0f), glm::vec3(8.0f, 3.0f, 0.5f));
        // Horizontal wall at Z=+5: right half only (gap on left side)
        drawMesh(wallMesh, shader, glm::vec3(2.0f, 0.5f, 5.0f), glm::vec3(8.0f, 3.0f, 0.5f));
        // Vertical wall at X=-5: top half only (gap on bottom side)
        drawMesh(wallMesh, shader, glm::vec3(-5.0f, 0.5f, -5.0f), glm::vec3(0.5f, 3.0f, 8.0f));
        // Vertical wall at X=+5: bottom half only (gap on top side)
        drawMesh(wallMesh, shader, glm::vec3(5.0f, 0.5f, 2.0f), glm::vec3(0.5f, 3.0f, 8.0f));

        // Collectibles
        for (auto& c : collectibles)
            if (!c.collected)
                drawMesh(collectMesh, shader, c.pos, glm::vec3(0.4f, 0.4f, 0.4f));

        // ?? Collection check
        for (auto& c : collectibles) {
            if (!c.collected) {
                glm::vec2 pXZ = glm::vec2(cameraPos.x, cameraPos.z);
                glm::vec2 cXZ = glm::vec2(c.pos.x, c.pos.z);
                if (glm::length(pXZ - cXZ) < 0.9f) {
                    c.collected = true;
                    score++;
                    std::cout << "Collected! " << score << "/5\n";
                }
            }
        }

        // ?? Timer
        if (!gameover && !gameWon) {
            timeRemaining -= deltaTime;
            if (timeRemaining <= 0.0f) { timeRemaining = 0.0f; gameover = true; }
        }
        if (score == (int)collectibles.size()) gameWon = true;

        // ?? HUD
        std::string title;
        if (gameWon)  title = "YOU WIN! All coins collected! Press ESC.";
        else if (gameover) title = "GAME OVER! Time ran out! Press ESC.";
        else               title = "Score: " + std::to_string(score) + "/5  |  Time: "
            + std::to_string((int)timeRemaining) + "s";
        glfwSetWindowTitle(window, title.c_str());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
