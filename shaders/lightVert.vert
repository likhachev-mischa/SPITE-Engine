#version 450

layout(location = 0) out vec2 outUV;

void main() {
    // Full-screen triangle/quad generation
    // Output UVs to interpolate across the screen for G-Buffer sampling
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);

    // Handle Vulkan's inverted Y coordinate if necessary
    //#ifdef VULKAN
    //gl_Position.y = -gl_Position.y;
    //#endif
}