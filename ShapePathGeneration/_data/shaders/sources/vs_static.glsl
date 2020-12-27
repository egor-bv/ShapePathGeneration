$input  a_position, a_normal
$output v_normal, v_view

#include "bgfx_shader.sh"
#include "shaderlib.sh"

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0f));
    
    // view space normal
    v_normal    = mul(u_modelView, vec4(a_normal.xyz, 0.0f)).xyz;

    // view space position
    v_view      = mul(u_modelView,     vec4(a_position.xyz, 1.0f)).xyz;

}