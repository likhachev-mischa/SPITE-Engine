#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D modelAlbedoTexture;

layout(location = 0) out vec4 outPosition;   // World Position
layout(location = 1) out vec4 outNormal;     // World Normal
layout(location = 2) out vec4 outAlbedoSpec; // Albedo (RGB), Specular Intensity (A)

void main() {

    vec3 albedo = texture(modelAlbedoTexture, inUV).rgb;
    float specularIntensity = 0.5;

    outPosition   = vec4(inPosition, 1.0); 
    outNormal     = vec4(normalize(inNormal), 1.0); 
    outAlbedoSpec = vec4(albedo, specularIntensity);
}
