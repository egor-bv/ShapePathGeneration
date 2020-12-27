$input v_texcoord0

#include "bgfx_shader.sh"
#include "shaderlib.sh"

#include "image_effects.glsl"

SAMPLER2D(s_input, 0);


void main()
{
    vec2 uv = v_texcoord0;
    vec3 in_color = texture2D(s_input, uv).rgb;
    vec3 out_color = vec3_splat(0.5f);

    out_color = in_color;
    gl_FragColor = vec4(out_color, 1.0f);
}   