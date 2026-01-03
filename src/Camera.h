#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float yaw;
    float pitch;
    float Fov;

    float lastX;
    float lastY;

    float Speed;
    float MouseSensitivity;
    bool firstMouse = true;
    
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f))
        : Position(pos),
          Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
          yaw(-90.0f),
          pitch(0.0f),
          Speed(2.0f),
          Fov(45.0f),
          firstMouse(true),
          MouseSensitivity(0.1f) {
            updateCameraVectors();
          }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboardForward(float deltaTime) {
        Position += Front * Speed * deltaTime;
    }
    void ProcessKeyboardBackward(float deltaTime) {
        Position -= Front * Speed * deltaTime;
    }
    void ProcessKeyboardLeft(float deltaTime) {
        Position -= Right * Speed * deltaTime;
    }
    void ProcessKeyboardRight(float deltaTime) {
        Position += Right * Speed * deltaTime;
    }
    void ProcessKeyboardUp(float deltaTime) {
        Position += Up * Speed * deltaTime;
    }
    void ProcessKeyboardDown(float deltaTime) {
        Position -= Up * Speed * deltaTime;
    }

    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        Fov -= yoffset;
        if (Fov < 1.0f)  Fov = 1.0f;
        if (Fov > 45.0f) Fov = 45.0f;   
    }

    void Reset()
    {
        Position = glm::vec3(0.0f, 0.0f, 3.0f);
        WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        Fov = 45.0f;
        Speed = 2.5;

        updateCameraVectors();  
    }

    // fixes jumping between cams by updating camera position
    void LookAt(glm::vec3 targetPosition)
    {
        glm::vec3 dir = normalize(targetPosition - Position);

        yaw = glm::degrees(std::atan2(dir.z, dir.x));
        pitch = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        // pitch = clamp(pitch, -89°, +89°); another way to clamp
        updateCameraVectors();
    }


private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
#endif

// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
// {
//     fov -= (float)yoffset;
//     if (fov < 1.0f)
//         fov = 1.0f;
//     if (fov > 45.0f)
//         fov = 45.0f;
// }
// void mouse_callback(GLFWwindow* window, double xpos, double ypos)
// {
//     if (firstMouse)
//     {
//         lastX = xpos;
//         lastY = ypos;
//         firstMouse = false;
//     }

//     float xoffset = xpos - lastX;
//     float yoffset = lastY - ypos;
//     lastX = xpos;
//     lastY = ypos;

//     float sensitivity = 0.1f;
//     xoffset *= sensitivity;
//     yoffset *= sensitivity;

//     yaw += xoffset;
//     pitch += yoffset;

//     if (pitch > 89.0f)
//         pitch = 89.0f;
//     if (pitch < -89.0f)
//         pitch = -89.0f;

//     glm::vec3 direction;
//     direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
//     direction.y = sin(glm::radians(pitch));
//     direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
//     Front = glm::normalize(direction);
// }