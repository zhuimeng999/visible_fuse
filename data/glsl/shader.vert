#version 450
#extension GL_EXT_multiview : enable

layout (constant_id = 0) const uint MAX_SOURCE_VIEV_NUM = 10;

layout (binding = 0) uniform UBO
{
    mat4 ref_world_pose;
    mat4 src_world_pose[MAX_SOURCE_VIEV_NUM];
    mat4 ref_src_pose[MAX_SOURCE_VIEV_NUM];
    mat4 src_ref_pose[MAX_SOURCE_VIEV_NUM];
    vec2 ref_size;
    vec2 dst_size;
    uint src_num;
};

layout (binding = 6) uniform sampler2D src_rgbs[MAX_SOURCE_VIEV_NUM];
layout (binding = 7) uniform sampler2D src_depth_probs[MAX_SOURCE_VIEV_NUM];

layout (location = 0) out vec4 v_rgba;
layout (location = 1) noperspective out vec4 v_dp;

const vec2 positions[6] = vec2[](
    vec2(0.5f, 0.5f),
    vec2(1.5f, 0.5f),
    vec2(1.5f, 1.5f),
    vec2(1.5f, 1.5f),
    vec2(0.5f, 1.5f),
    vec2(0.5f, 0.5f)
);

void main() {
    ivec2 srcSize = textureSize(src_rgbs[gl_ViewIndex], 0) - 1;
    uint squarX = gl_InstanceIndex/srcSize.y;
    uint squarY = gl_InstanceIndex%srcSize.y;
    vec2 xy = vec2(squarX, squarY) + positions[gl_VertexIndex];
    v_rgba = texture(src_rgbs[gl_ViewIndex], xy);
    v_dp = texture(src_depth_probs[gl_ViewIndex], xy);
    gl_Position = src_ref_pose[gl_ViewIndex]*vec4(xy*v_dp.x, v_dp.x, 1.0f);
    v_dp.x = gl_Position.z;
    gl_Position.xy = gl_Position.xy/gl_Position.z;
    gl_Position.z = gl_Position.z/2000;
    gl_Position.xy = (2*gl_Position.xy/ref_size - 1);
}