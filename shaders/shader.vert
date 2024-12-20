#version 450

layout (set=0,binding =0) uniform UniformBufferObject{
mat4 model;
} ubo;

layout (set = 1,binding = 1) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
} cam;


layout (location = 0) in vec3 position;

void main()
{
	gl_Position =cam.projection* cam.view*
	ubo.model * vec4(position,1.0);
}
