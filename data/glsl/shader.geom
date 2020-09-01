#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 v_rgba[];
layout (location = 1) noperspective in vec4 v_dp[];

layout (location = 0) out vec4 g_rgba;
layout (location = 1) noperspective out vec4 g_dp;

void main(void)
{
	for(int i=0; i<gl_in.length(); i++)
	{
		if(v_dp[i].y < 0.3){
			return;
		}
	}
	for(int i=0; i<gl_in.length(); i++)
	{
		gl_Position = gl_in[i].gl_Position;
		g_rgba = v_rgba[i];
		g_dp = v_dp[i];
		EmitVertex();
	}
	EndPrimitive();
}