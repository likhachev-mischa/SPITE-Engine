#include "AffineTransform.h"

glm::mat4 affine::identity()
{
	return glm::mat4(1, 0, 0, 0,
	                 0, 1, 0, 0,
	                 0, 0, 1, 0,
	                 0, 0, 0, 1);
}

glm::mat4 affine::translation(float x, float y)
{
	return glm::mat4(1, 0, 0, 0,
	                 0, 1, 0, 0,
	                 0, 0, 1, 0,
	                 x, y, 0, 1);
}

glm::mat4 affine::rotationX(float angle)
{
	return glm::mat4(1, 0, 0, 0,
	                 0, glm::cos(angle), -glm::sin(angle), 0,
	                 0, glm::sin(angle), glm::cos(angle), 0,
	                 0, 0, 0, 1);
}

glm::mat4 affine::rotationY(float angle)
{
	return glm::mat4(glm::cos(angle), 0, glm::sin(angle), 0,
	                 0, 1, 0, 0,
	                 -glm::sin(angle), 0, cos(angle), 0,
	                 0, 0, 0, 1);
}

glm::mat4 affine::rotationZ(float angle)
{
	return glm::mat4(glm::cos(angle), -glm::sin(angle), 0, 0,
	                 glm::sin(angle), glm::cos(angle), 0, 0,
	                 0, 0, 1, 0,
	                 0, 0, 0, 1);
}

glm::mat4 affine::scale(float value)
{
	return glm::mat4(value, 0, 0, 0,
	                 0, value, 0, 0,
	                 0, 0, value, 0,
	                 0, 0, 0, 1);
}
