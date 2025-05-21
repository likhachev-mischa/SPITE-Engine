#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

// G-Buffer Textures bound via Descriptor Set
layout(set = 0, binding = 0) uniform sampler2D texPosition;
layout(set = 0, binding = 1) uniform sampler2D texNormal;
layout(set = 0, binding = 2) uniform sampler2D texAlbedoSpec;

struct PointLight {
    vec4 position; // World space position. w component radius
    vec4 color;    // w component  intensity
};

struct DirectionalLight {
    vec3 direction; // World space direction. 
    vec4 color;     // w component intensity
};

struct SpotLight {
    vec4 position;  // World space position. w component radius
    vec3 direction; // World space direction.
    vec4 color;     // w component intensity
    vec2 cutOffs;   // x is inner cone (cosine), y is outer cone (cosine)
};

layout(set = 1, binding = 0) readonly uniform LightData {
   PointLight pointLights[1]; 
   DirectionalLight directionalLights[1]; 
   SpotLight spotLights[1]; 
} lightData;

layout(push_constant) uniform CameraInfo {
    vec3 cameraPositionWorld;
} cameraInfo;

void main() {
    vec3 fragPosWorld   = texture(texPosition, inUV).rgb;
    vec3 normalWorld    = normalize(texture(texNormal, inUV).rgb); // Ensure normal is normalized
    vec4 albedoSpec = texture(texAlbedoSpec, inUV);
    vec3 albedo     = albedoSpec.rgb;
    float specularStrength = 0.1;

    float ambient = 0.5;

    // Calculate viewDir in WORLD space
    vec3 viewDir = normalize(cameraInfo.cameraPositionWorld - fragPosWorld);

    vec3 pointLightContribution = vec3(0.0);
    int numPointLights = lightData.pointLights.length();

    for (int i = 0; i < numPointLights; ++i) {
        PointLight light = lightData.pointLights[i];
        vec3 lightPosWorld = light.position.xyz;
        float lightRadius = light.position.w;
        vec3 lightColor = light.color.rgb;
        float lightIntensity = light.color.a;

        vec3 toLight = lightPosWorld - fragPosWorld;
        float distanceToLight = length(toLight);
        vec3 pointLightDir = normalize(toLight);

        float attenuation = 1.0;
        if (lightRadius > 0.0) {
            attenuation = 1.0 - smoothstep(0.0, lightRadius, distanceToLight);
            attenuation = clamp(attenuation, 0.0, 1.0);
        } else {
            attenuation = 1.0 / (1.0 + 0.1 * distanceToLight + 0.01 * distanceToLight * distanceToLight);
        }

        float pointDiffuseIntensity = max(dot(normalWorld, pointLightDir), 0.0);
        vec3 pointDiffuse = pointDiffuseIntensity * lightColor;

        vec3 pointSpecular = vec3(0.0);
        if (pointDiffuseIntensity > 0.0) {
            vec3 pointReflectDir = reflect(-pointLightDir, normalWorld);
            float pointSpec = pow(max(dot(viewDir, pointReflectDir), 0.0), 32.0);
            pointSpecular = specularStrength * pointSpec * lightColor;
        }

        pointLightContribution += (pointDiffuse + pointSpecular) * lightIntensity * attenuation;
    }

    // Directional Light Calculation
    vec3 directionalLightContribution = vec3(0.0);
    int numDirectionalLights = lightData.directionalLights.length();
    for (int i = 0; i < numDirectionalLights; ++i) {
        DirectionalLight light = lightData.directionalLights[i];
        vec3 lightDirWorld = normalize(light.direction.xyz);
        vec3 lightColor = light.color.rgb;
        float lightIntensity = light.color.a;

        float diffuseIntensity = max(dot(normalWorld, -lightDirWorld), 0.0);
        vec3 diffuse = diffuseIntensity * lightColor;

        vec3 reflectDir = reflect(lightDirWorld, normalWorld);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = specularStrength * spec * lightColor;

        directionalLightContribution += (diffuse + specular) * lightIntensity;
    }

    // SpotLight Calculation
    vec3 spotLightContribution = vec3(0.0);
    int numSpotLights = lightData.spotLights.length();
    for (int i = 0; i < numSpotLights; ++i) {
        SpotLight light = lightData.spotLights[i];
        vec3 lightPosWorld = light.position.xyz;
        float lightRadius = light.position.w;
        vec3 spotDirWorld = normalize(light.direction.xyz);
        vec3 lightColor = light.color.rgb;
        float lightIntensity = light.color.a;
        float innerCutOffCos = light.cutOffs.x;
        float outerCutOffCos = light.cutOffs.y;

        vec3 toLight = lightPosWorld - fragPosWorld;
        float distanceToLight = length(toLight);
        vec3 spotLightDir = normalize(toLight);

        float attenuation = 1.0;
        if (lightRadius > 0.0) {
            attenuation = 1.0 - smoothstep(0.0, lightRadius, distanceToLight);
            attenuation = clamp(attenuation, 0.0, 1.0);
        } else {
            attenuation = 1.0 / (1.0 + 0.1 * distanceToLight + 0.01 * distanceToLight * distanceToLight);
        }

        float theta = dot(spotLightDir, -spotDirWorld);
        if (theta > outerCutOffCos) {
            float diffuseIntensity = max(dot(normalWorld, spotLightDir), 0.0);
            vec3 diffuse = diffuseIntensity * lightColor;

            vec3 reflectDir = reflect(-spotLightDir, normalWorld);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * lightColor;

            float epsilon = innerCutOffCos - outerCutOffCos;
            float spotEffect = smoothstep(0.0, 1.0, (theta - outerCutOffCos) / epsilon);

            spotLightContribution += (diffuse + specular) * lightIntensity * attenuation * spotEffect;
        }
    }

    vec3 finalColor = albedo * ambient + pointLightContribution + directionalLightContribution + spotLightContribution;

    outFragColor = vec4(finalColor, 1.0);
}
