#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D SceneColor;
layout(set = 0, binding = 1) uniform sampler2D UIColor;

void main() 
{
    vec4 scene = texture(SceneColor, inUV);
    vec4 ui = texture(UIColor, inUV);

    // Simple alpha blending: ui_color * ui_alpha + scene_color * (1 - ui_alpha)
    outFragColor = mix(scene, ui, ui.a);
}
