$output v_texcoord0

#include "bgfx_shader.sh"
#include "shaderlib.sh"

void main()
{
    // NOTE: this makes a triangle in CCW order, beware of culling
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_texcoord0.x = (x+1.0)*0.5;
    
    // NOTE: flipping y
    v_texcoord0.y = (-y+1.0)*0.5;
    
    // NOTE: z is set to far plane
    gl_Position = vec4(x, y, 1.0f, 1.0f);
}