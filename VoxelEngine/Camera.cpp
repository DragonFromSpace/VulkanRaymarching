#include "pch.h"
#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	:m_Forward(glm::vec3(0.0f, 0.0f, -1.0f))
{
	m_Position = position;
	m_WorldUp = up;
	m_Yaw = yaw;
	m_Pitch = pitch;
	updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void Camera::ProcessKeyboard(Camera_Movement direction)
{
	float velocity = m_MovementSpeed;
	switch (direction)
	{
	case FORWARD:
		m_Position += m_Forward * velocity;
		break;
	case BACKWARD:
		m_Position -= m_Forward * velocity;
		break;
	case LEFT:
		m_Position += m_Right * velocity;
		break;
	case RIGHT:
		m_Position -= m_Right * velocity;
		break;
	case UP:
		m_Position += m_WorldUp * velocity;
		break;
	case DOWN:
		m_Position -= m_WorldUp * velocity;
		break;
	}
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset)
{
	xOffset *= m_MouseSensitivity;
	yOffset *= m_MouseSensitivity;

	m_Yaw += xOffset;
	m_Pitch += yOffset;

	if (m_Pitch > 89.0f)
		m_Pitch = 89.0f;
	if (m_Pitch < -89.0f)
		m_Pitch = -89.0f;

	updateCameraVectors();
}

void Camera::updateCameraVectors()
{
	glm::vec3 forward;
	forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	forward.y = sin(glm::radians(m_Pitch));
	forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Forward = glm::normalize(forward);
	
	m_Right = glm::normalize(glm::cross(m_Forward, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Forward, m_Right));
}