#version 450

layout (set=0,binding =0) uniform UniformBufferObject{
mat4 viewProjection;
} ubo;

layout(push_constant) uniform pc
{
	mat4 model;
};


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main()
{
	//fragPosition = vec3(ubo.model * vec4(position,1.0));

	//fragNormal = mat3(transpose(inverse(ubo.model))) * normal; 

	//mat4 pos = mat4(1.0);
 	outNormal = normalize(transpose(inverse(mat3(model))) * inNormal);

	vec4 worldPos = model * vec4(inPosition,1.0);
	outPosition = worldPos.xyz;
	outUV = inUV;

	gl_Position =ubo.viewProjection * worldPos;
	//gl_Position = vec4(position,1.0);

}
