#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor; // Final output color

// G-Buffer Textures bound via Descriptor Set
layout(set = 0, binding = 0) uniform sampler2D texPosition;
layout(set = 0, binding = 1) uniform sampler2D texNormal;
layout(set = 0, binding = 2) uniform sampler2D texAlbedoSpec;

// Light Data (Example: Array of point lights)
struct PointLight {
    vec4 position; // w component could be radius
    vec4 color;    // w component could be intensity
};

layout(set = 1, binding = 0) readonly buffer LightData {
   PointLight lights[];
} lightData;

// Camera Data (optional, for view position)
layout(set = 2, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj; // Could extract viewPos from inverse(view)[3]
} cameraData;


void main() {
    // Sample G-Buffer using screen coordinates
    // Use texelFetch if input attachments, texture(sampler, uv) if sampled images
    vec3 fragPos   = texture(texPosition, inUV).rgb;
    vec3 normal    = texture(texNormal, inUV).rgb;
    vec4 albedoSpec= texture(texAlbedoSpec, inUV);
    vec3 albedo    = albedoSpec.rgb;
    float specular = albedoSpec.a;

    // Example: Simple Directional Light
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.5));
    vec3 lightColor = vec3(1.0);
    float ambient = 0.1;
    float diff = max(dot(normal, -lightDir), 0.0);
    //vec3 diffuse = diff * lightColor;
    vec3 diffuse = vec3(0,0,0);

    // Add point light contributions (loop through lightData.lights)
    // ... Calculate attenuation, diffuse, specular for each light ...

    // Combine lighting
    vec3 finalColor = albedo * (ambient + diffuse /* + point light contributions */);
    // Add specular highlights
    // ...

    outFragColor = vec4(finalColor, 1.0);

    // Apply Tone Mapping if rendering to HDR buffer first
    // ...
}
