#pragma once
enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
public:
	Camera(glm::vec3 position = glm::vec3(0,0,0), glm::vec3 up = glm::vec3(0,1,0) , float yaw = -90.0f, float pitch = 0.0f);

	glm::mat4 GetViewMatrix();

	void ProcessKeyboard(Camera_Movement direction);
	void ProcessMouseMovement(float xOffset, float yOffset);


private:
	void updateCameraVectors();

	glm::vec3 m_Position;
	glm::vec3 m_Forward;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	float m_Yaw = -90.0f;
	float m_Pitch = 0.0f;

	float m_MovementSpeed = 0.1f;
	float m_MouseSensitivity = 0.1f;
};