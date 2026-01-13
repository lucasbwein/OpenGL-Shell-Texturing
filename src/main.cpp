#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
// #include "Model.h"
#include "Mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


/* ----------
To build use:
- cd build
- cmake --build . && ./OpenGlShell
---------- */

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
Camera debugCam;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool useDebugCam = false; // bool for debug perspective
bool flashlightOn = false; // bool for flashlight
bool uiMode = false; // for tabbing out

int numLayers = 80; // instances/layers of hair
float gridFreq = 1500.0f; // hair frequency on object
float strandThickness = 0.9f; // thickness of hair
float furLength = 0.25f; // length of strands

glm::vec3 lastCameraPos = glm::vec3(0.0f); // allows for velocity calculation
glm::vec3 furWindDirection = glm::vec3(0.0f); // used for hair physics

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Allows better scaling to Window Change
    if (width <= 0 || height <= 0) return;

    float baseAspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    float windowAspect  = (float)width  / (float)height;

    int viewpX = 0, viewpY = 0, viewpW = width, viewpH = height;

    if (windowAspect > baseAspect) {
    // Window is wider than base cause pillarbox (bars left/right)
        viewpH = height;
        viewpW = (int)(height * baseAspect);
        viewpX = (width - viewpW) / 2;
        viewpY = 0;
    } else {
    // Window is taller than base cause letterbox (bars top/bottom)
        viewpW = width;
        viewpH = (int)(width / baseAspect);
        viewpX = 0;
        viewpY = (height - viewpH) / 2;
    }

    glViewport(viewpX, viewpY, viewpW, viewpH);
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void processInput(GLFWwindow *window, Camera& camera, float deltaTime){
    static bool fWasPressed = false;
    static bool pWasPressed = false;
    static bool oWasPressed = false;

    // closes window
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)  // shows non polygons
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)  // shows polygons
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)    // moves forwards
        camera.ProcessKeyboardForward(deltaTime);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)    // moves backwards
        camera.ProcessKeyboardBackward(deltaTime);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)    // moves left
        camera.ProcessKeyboardLeft(deltaTime);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)    // moves right
        camera.ProcessKeyboardRight(deltaTime);
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)// moves up
        camera.ProcessKeyboardUp(deltaTime);
    if(glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)  // moves down
        camera.ProcessKeyboardDown(deltaTime);

    // increases and decrease movement speed
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.Speed += 0.25;
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && camera.Speed > 0.25)
        camera.Speed -= 0.25;

    // resets camera, position, and speed
    if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera.Reset();
        camera.firstMouse = true;
    }
    // Tabs Out of Window (UI switch)
    bool oPressed = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;
    if(oPressed && !oWasPressed) {
        uiMode = !uiMode;
        glfwSetInputMode(window, GLFW_CURSOR, uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        camera.firstMouse = true;
        debugCam.firstMouse = true;
    }
    oWasPressed = oPressed;
    // Flashlight
    bool fPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if(fPressed && !fWasPressed) {
        flashlightOn = !flashlightOn; 
    }
    fWasPressed = fPressed;

    // Debug Camera Switch
    bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if(pPressed && !pWasPressed) {
        useDebugCam = !useDebugCam;
    }
    pWasPressed = pPressed;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(uiMode) return;
    Camera &cam = useDebugCam ? debugCam : camera; // chooses correct camera
    cam.ProcessMouseScroll((float)yoffset);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(uiMode) return;
    Camera &cam = useDebugCam ? debugCam : camera; // chooses correct camera
    if (cam.firstMouse) // Makes it so camera doesn't jump on first click
    {
        cam.lastX = (float)xpos;
        cam.lastY = (float)ypos;
        cam.firstMouse = false;
    }

    // uses stored position to calculate offset of mouse movement
    float xoffset = (float)xpos - cam.lastX;
    float yoffset = cam.lastY - (float)ypos;

    cam.lastX = (float)xpos;
    cam.lastY = (float)ypos;

    cam.ProcessMouseMovement(xoffset, yoffset);
}

void updateFurPhysics(Camera& cam, float deltaTime) {
    // calculate velocity based on camera position
    glm::vec3 velocity = (cam.Position - lastCameraPos) / deltaTime;
    lastCameraPos = cam.Position;

    // smooths the wind direction
    float smoothing = 0.9f;
    glm::vec3 targetWind = -velocity;
    furWindDirection = glm::mix(targetWind, furWindDirection, smoothing);

    // Clamp maximum strength
    float maxWindStrength = 2.0f;
    if(glm::length(furWindDirection) > maxWindStrength) {
        furWindDirection = glm::normalize(furWindDirection) * maxWindStrength;
    }
}

int main() {
    // Calls intialization in the if statement
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    // Request an OpenGL 3.3 core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
// --------------------------
    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shell Texturing", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // register callback
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // first person control
// --------------------------
    // Initializes GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on y-axis (before loading model)
    stbi_set_flip_vertically_on_load(true);

// --------------------------
    // load shaders
    Shader cubeShader("../shaders/basic.vert", "../shaders/basic.frag");

// --------------------------
    // Create Sphere Object
    float radius = 1.0f; // radius of circle to draw
    int stacks = 32;     // verticle
    int slices = 32;     // horizontal

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // Gets the Vertices
    for (int i = 0; i <= stacks; i++)
    {
        float v = float(i) / stacks;
        float phi = v * glm::pi<float>(); // 0 --> PI

        for(int j = 0; j <= slices; j++)
        {
            float u = float(j) / slices;
            float theta = u * glm::two_pi<float>();

            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            Vertex vert;
            vert.Position = radius * glm::vec3(x, y, z);
            vert.Normal = glm::normalize(glm::vec3(x, y, z));
            vert.TexCoords = glm::vec2(u, v);

            vertices.push_back(vert);
        }    
    }
    // Gets Sphere Indices
    for(int i = 0; i < stacks; i++) {
        for(int j = 0; j < slices; j++) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, 3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };  
    glm::vec3 pointLightColors[] = {
        glm::vec3(1.0f, 0.6f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0, 0.0),
        glm::vec3(0.2f, 0.2f, 1.0f)
    };

    std::vector<Vertex> squareVertices = {
        // position              // normal           // texcoords
        {{-0.5f, -0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    };
    std::vector<unsigned int> squareIndices = {
        0, 1, 2,
        2, 3, 0
    };

    // Create Square Object
    unsigned int quadVAO, quadVBO, quadEBO;

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, squareVertices.size() * sizeof(Vertex),
                squareVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, squareIndices.size() * sizeof(unsigned int),
                squareIndices.data(), GL_STATIC_DRAW);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Position)
    );
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Normal));
    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glBindVertexArray(0);


    // initlaize VBO and VAO
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // bind vertex array object 
    glBindVertexArray(VAO);

    // copy vertices into buffer (VBO) for use   
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // set VAO -- position layout(location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    // set VAO -- color attribute layout(location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // set VAO -- UV layout(location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    
    // -----------------------
    cubeShader.use();

    glBindBuffer(GL_ARRAY_BUFFER, 0); // safely unbind 
    glBindVertexArray(0); // unbind VAO

    glEnable(GL_DEPTH_TEST); // enables Z-buffer test

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float currFrame = glfwGetTime();     // current time
        deltaTime = currFrame - lastFrame;   // time between frames
        lastFrame = currFrame;               // time of last frame

        // FPS => Better Practice is using Debug program
        static int frames = 0;
        static double lastTime = 0.0;
        frames++;
        if (glfwGetTime() - lastTime >= 1){
            double fps = frames / (glfwGetTime() - lastTime);
            frames = 0;
            lastTime = glfwGetTime();

            std::string title = "FPS: " + std::to_string((int)fps);
            glfwSetWindowTitle(window, title.c_str());
        }

        // used in debugging
        if (!useDebugCam) {
            debugCam.Position = camera.Position + glm::vec3(10, 10, 10);
            debugCam.LookAt(camera.Position);
        }
        // gets correct camera to use
        Camera& activeCam = useDebugCam ? debugCam : camera;

        processInput(window, activeCam, deltaTime);
        updateFurPhysics(activeCam, deltaTime);

        // Set clear color and clear 
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // background color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use our shader program and draw first triangle
        cubeShader.use();
        cubeShader.setFloat("currFrame", currFrame);

        cubeShader.setVec3("viewPos", activeCam.Position);

        // light properties
        glm::vec3 lightColor = glm::vec3(1.0f);

        glm::vec3 diffuseColor = lightColor   * glm::vec3(0.4f); // decrease the influence
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.05f); // low influence

        // directional
        cubeShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f); 
        cubeShader.setVec3("dirLight.ambient", ambientColor);
        cubeShader.setVec3("dirLight.diffuse", diffuseColor);
        cubeShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        
        // Point Light 1
        cubeShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        cubeShader.setFloat("pointLights[0].constant",  1.0f);
        cubeShader.setFloat("pointLights[0].linear",    0.09f);
        cubeShader.setFloat("pointLights[0].quadratic", 0.032f);
        cubeShader.setVec3("pointLights[0].ambient", ambientColor);
        cubeShader.setVec3("pointLights[0].diffuse", diffuseColor);
        cubeShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        

        // Spot Light
        cubeShader.setVec3("spotLight.position", camera.Position);
        cubeShader.setVec3("spotLight.direction", camera.Front); 
        cubeShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        cubeShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        cubeShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        cubeShader.setFloat("spotLight.constant", 1.0f);
        cubeShader.setFloat("spotLight.linear", 0.09f);
        cubeShader.setFloat("spotLight.quadratic", 0.032f);
        cubeShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        cubeShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
        cubeShader.setInt("spotLight.FlashLightEnable", flashlightOn ? 1 : 0);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = activeCam.GetViewMatrix();

        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        projection = glm::perspective(
            glm::radians(activeCam.Fov),
            aspect,
            0.1f, 100.0f
        );

        // Orthographic Projection
        // float size = 5.0f; // zoom level
        // projection = glm::ortho(
        //     -size * aspect,  size * aspect,
        //     -size,           size,
        //     0.1f,            100.0f
        // );

    
        cubeShader.setMat4("model", model);
        cubeShader.setMat4("view", view);
        cubeShader.setMat4("projection", projection);

        glDisable(GL_CULL_FACE);
        // glDepthMask(GL_TRUE);
        glDepthMask(GL_FALSE);
        // glDisable(GL_BLEND);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Emulates Resting Wind
        glm::vec3 ambientWind = glm::vec3(sin(currFrame * 0.5f) * 0.3, 0.0f,
                                          cos(currFrame * 0.7f) * 0.0f);
        glm::vec3 totalWind = furWindDirection + ambientWind;

        cubeShader.setVec3("baseColor", glm::vec3(0.8f, 0.7f, 0.6f));
        cubeShader.setInt("uNumLayers", numLayers);
        cubeShader.setFloat("uFurLength", 0.15f);
        cubeShader.setVec3("uWindDirection", totalWind);
        cubeShader.setVec3("uGravity", glm::vec3(0.0f, -1.0f, 0.0f));

        cubeShader.setFloat("uStrandThickness", strandThickness);
        cubeShader.setFloat("uGridFrequency", gridFreq);

        // Draws the circle
        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)indices.size(),
                                GL_UNSIGNED_INT, 0, numLayers);
        
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        // cubeShader.setMat4("model", model);
        // Draws Flat Square
        // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // 
        // glBindVertexArray(quadVAO);
        // glDrawElementsInstanced(
        //     GL_TRIANGLES,
        //     (GLsizei)squareIndices.size(),
        //     GL_UNSIGNED_INT,
        //     0,
        //     numLayers        
        // );
        glBindVertexArray(0);

        // Swap front and back buffers
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
    }

    // de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}