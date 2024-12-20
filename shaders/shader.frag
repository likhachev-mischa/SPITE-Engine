#version 450

layout (location = 0) out vec4 color;

void main()
{
float ambientStrength =0.1;
vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

vec3 result = ambient * (1.0,0.0,0.0);
color = vec4(result, 1.0);
}
