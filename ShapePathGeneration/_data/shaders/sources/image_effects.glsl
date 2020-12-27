#ifndef IMAGE_EFFECTS_HEADER_GUARD
#define IMAGE_EFFECTS_HEADER_GUARD


/* --------------------
        Tonemapping
   -------------------- */

uniform vec4 u_TonemapParams;

#define TonemapMethod u_TonemapParams.x
#define TONEMAP_NONE        0.0f
#define TONEMAP_REINHARD    1.0f
#define TONEMAP_ACES        2.0f

#define WhitePoint u_TonemapParams.y 

// simple ACES
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

// more accurate ACES
// NOTE: are in column major form, so mul order is reversed (?)

CONST(mat3) ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
CONST(mat3) ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    // NOTE: multiplication order is imporant and depends on the backend
    color = mul(ACESInputMat, color);
    color = RRTAndODTFit(color);
    color = mul(ACESOutputMat, color);
    color = clamp(color, 0.0, 1.0);

    return color;
}


/* --------------------
        Film grain
   -------------------- */

// Good reference, but maybe too complicated?
// https://www.shadertoy.com/view/Mly3Rw
// Image imperfections + film grain

// NOTE: those are tuned for integer scale, not 0-1
// #include "hash.glsl"

#if 1
// Hash functions from here:
// https://www.shadertoy.com/view/Mt2yzK
// https://www.shadertoy.com/view/4djSRW

#define HASHSCALE1 443.8975
#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
#define HASHSCALE4 vec3(443.897, 441.423, 437.195, 444.129)


//----------------------------------------------------------------------------------------
//  1 out, 1 in...
float hash11(float p)
{
    vec3 p3  = fract(vec3_splat(p) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float hash12(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//  2 out, 1 in...
vec2 hash21(float p)
{
    vec3 p3 = fract(vec3_splat(p) * HASHSCALE3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
    p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//  1 out, 3 in...
float hash13(vec3 p3)
{
    p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

///  3 out, 3 in...
vec3 hash33(vec3 p3)
{
    p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}

#endif


// Gaussian
float normpdf(in float x, in float sigma)
{
    return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}


// Cosine palettes

vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

#endif