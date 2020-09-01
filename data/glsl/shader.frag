#version 450

layout (location = 0) in vec4 g_rgba;
layout (location = 1) noperspective in vec4 g_dp;

layout (location = 0) out vec4 f_rgba;
layout (location = 1) out vec4 f_dp;

void main() {
    f_rgba = g_rgba;
    f_dp = g_dp;
}
