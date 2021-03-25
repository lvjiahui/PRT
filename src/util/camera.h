#ifndef CAMERA_H
#define CAMERA_H

#include "lib/mathlib.h"


// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.3f;
const float ZOOM        =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    Vec3 Position;
    Vec3 Front;
    Vec3 Up;
    Vec3 Right;
    Vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    float lastX;
    float lastY;
    bool firstMouse = true;

    // constructor with vectors
    Camera(Vec3 position = Vec3(0.0f, 0.0f, 0.0f), Vec3 up = Vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(Vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
		//Front = glm::normalize(Position);
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
		Right = cross(Front, WorldUp).normalize();  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = cross(Right, Front).normalize();
        //updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(Vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = Vec3(posX, posY, posZ);
		//Front = glm::normalize(Position);
        WorldUp = Vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
		Right = cross(Front, WorldUp).normalize();  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = cross(Right, Front).normalize();
        //updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    Mat4 GetViewMatrix()
    {
		float length = sqrt(Position.x * Position.x + Position.y * Position.y + Position.z * Position.z);
        return Mat4::look_at(Position, Position+Front*length, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
		//updateCameraVectors();
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch -= yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
		// calculate the new Front vector
		Vec3 front;
		float length = sqrt(Position.x * Position.x + Position.y * Position.y + Position.z * Position.z);
		front.x = length* cos(Radians(Yaw)) * cos(Radians(Pitch));
		front.y = length * sin(Radians(Pitch));
		front.z = length * sin(Radians(Yaw)) * cos(Radians(Pitch));
		Position = front;

        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
		yoffset = yoffset / 2;
		Position -= (Front * yoffset);
		//updateCameraVectors();
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        Front = (-Position).normalize();
        // also re-calculate the Right and Up vector
        Right = cross(Front, WorldUp).normalize();  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = cross(Right, Front).normalize();
    }
};
#endif