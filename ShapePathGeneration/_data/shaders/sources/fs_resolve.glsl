$input v_texcoord0

#include "bgfx_shader.sh"
#include "shaderlib.sh"

#include "image_effects.glsl"

SAMPLER2D(s_input, 0);

vec3 sample(vec2 uv, int offset_x, int offset_y, float w, inout float wacc)
{
    vec4 v = texture2D(s_input, uv + vec2(offset_x, offset_y) / textureSize(s_input, 0));
    wacc += w * v.w;
    return v.xyz * w * v.w;
    // texelFetch(s_input, tc + ivec2(offset_x, offset_y), 0);
}

float Weight(float d)
{
    // return sin(2 * d) / (2 * d); // Lanczos
    return exp2(-0.7 * d * d); //Gauss
}


vec3 resolve_13(vec2 uv)
{
    float wacc = 0;
    float wa = Weight(0);
    float wb = Weight(1);
    float wc = Weight(1.41421356);
    float wd = Weight(2);

    vec3 a0 = sample(uv, +0, +0, wa, wacc);
    vec3 b0 = sample(uv, +0, +1, wb, wacc);
    vec3 b1 = sample(uv, +0, -1, wb, wacc);
    vec3 b2 = sample(uv, +1, +0, wb, wacc);
    vec3 b3 = sample(uv, -1, +0, wb, wacc);
    vec3 c0 = sample(uv, -1, +1, wc, wacc);
    vec3 c1 = sample(uv, -1, -1, wc, wacc);
    vec3 c2 = sample(uv, +1, -1, wc, wacc);
    vec3 c3 = sample(uv, +1, +1, wc, wacc);
    vec3 d0 = sample(uv, -2, +0, wd, wacc);
    vec3 d1 = sample(uv, +2, +0, wd, wacc);
    vec3 d2 = sample(uv, +0, +2, wd, wacc);
    vec3 d3 = sample(uv, +0, -2, wd, wacc);


    vec3 result = (a0
        + (b0 + b1 + b2 + b3)
        + (c0 + c1 + c2 + c3)
        + (d0 + d1 + d2 + d3))/ (wacc);
    return result;

}

void main()
{
    vec2 uv = v_texcoord0;
    vec2 input_size = textureSize(s_input, 0);
    vec2 out_size = u_viewRect.zw;

    vec3 out_color = resolve_13(uv);

    out_color = toGamma(ACESFitted(out_color));
    gl_FragColor = vec4(out_color, 1.0f);
}   