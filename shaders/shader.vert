#version 450

layout (binding =0) uniform UniformBufferObject{
mat4 model;
} ubo;

layout (location = 0) in vec3 position;

void main()
{
	gl_Position = ubo.model * vec4(position,1.0);
}
