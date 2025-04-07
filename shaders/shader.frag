#version 450

//
//layout (set=2,binding =0) uniform FragUbo{
//vec4 color;
//} fragData;
//
//
//layout (location = 0) in vec3 fragPosition;
//layout (location = 1) in vec3 fragNormal;
//
layout (location = 0) out vec4 color;

void main()
{
//vec4 lightColor = vec4(1.0, 1.0, 1.0,1.0);
//float ambientStrength =0.1;
//vec4 ambient = ambientStrength * lightColor;
//
//vec3 norm = normalize(fragNormal);
//vec3 lightPos = vec3(2.0,2.0,2.0);
//vec3 lightDir = normalize(lightPos - fragPosition);
//float diff = max(dot(norm, lightDir), 0.0);
//vec4 diffuse = diff* lightColor;
//
//color =(ambient+diffuse) * fragData.color;
color = vec4(1.0,0,0,1.0);

}
