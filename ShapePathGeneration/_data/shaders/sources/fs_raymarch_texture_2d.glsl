$input v_texcoord0

#include "bgfx_shader.sh"
#include "shaderlib.sh"


#include "emulated_3d.glsl"
#include "sd_and_ray_functions.glsl"

SAMPLER2D(i_sdf, 0);

uniform vec4 u_tile_map_size;
#define TileSize uvec2(u_tile_map_size.xy)
#define MapSize uvec2(u_tile_map_size.zw)

uniform vec4 u_cutter_plane;
uniform mat4 u_pointer_inverse_transform;

vec2 SDF_SCENE(vec3 p)
{
    vec3 p_tex = p + vec3_splat(0.5f);
    float d = Emulated3D_Sample(i_sdf, p_tex, TileSize,MapSize).x;
    float m = 1.0f;
    {
        vec3 pp = mul(u_pointer_inverse_transform, vec4(p,1.0f));
        float s = 0.2f;
        float d2 = s*sdArrow(pp/s);
        // d = min(d, d2);
        if (d2 < d) 
        {
            d = d2;
            m = 2.0f;
        }
    }

    return vec2(d, m);
}

#include "raymarching.glsl"

void GenerateRay(vec2 ndc_xy, mat4 ndc_to_world, out vec3 ro, out vec3 rd)
{
    vec4 eye_hom = mul(ndc_to_world, vec4(ndc_xy, 0, 1));
    vec4 target_hom = mul(ndc_to_world, vec4(ndc_xy, 1, 1));
    
    vec3 eye = eye_hom.xyz / eye_hom.w;
    vec3 target = target_hom.xyz / target_hom.w;

    ro = eye;
    rd = normalize(target - eye);
}

float planeIntersect( in vec3 ro, in vec3 rd, in vec4 p )
{
    return -(dot(ro,p.xyz)+p.w)/dot(rd,p.xyz);
}

void main()
{
    vec2 uv = v_texcoord0;
    uv.y = 1.0f - uv.y;
    vec2 resolution = u_viewRect.zw;
    vec3 out_color = vec3(1.4f, 0.4f, 1.8f);

    {
        vec2 uv_ndc = uv * 2 - vec2(1.0f, 1.0f);
        vec3 ro, rd;
        GenerateRay(uv_ndc, u_invViewProj, ro, rd);

        vec3 normal_unused = 0;
        vec2 intersect = boxIntersection(ro, rd,
            vec3(0.5, 0.5, 0.5), normal_unused);

        if (intersect.x == -1.0f && intersect.y == -1.0f)
        {
            out_color = vec3(0, 0, 0);
        }
        else
        {
            float box_near = intersect.x + 0.01;
            float box_far = intersect.y - 0.001;
            
            vec2 trace = TraceRay(ro + rd * box_near, rd, box_far - box_near);
            trace.x += box_near;
            vec3 p = ro + rd * trace.x;
            vec3 normal = EstimateNormal(p);
            vec3 light = vec3(0.6f, 1.0f, 1.3f) * saturate(dot(normal, vec3(1, 1, 1))/2 + 1.0f);
            light += 0.2f;

            float t_plane = planeIntersect(ro, rd, u_cutter_plane);

            if (trace.y > 0)
            {
                out_color = dot(normal, normalize(vec3(0.3, 1.0, 0.2))) + 1.5f;
                out_color *= trace.y == 1.0f ? 
                    vec3(0.01f, 0.1f, 0.01f) : vec3(1.0f, 0.0f, 0.0f);
                // out_color = trace.y / 127;
            }
            else
            {
                out_color = vec3_splat(0.02f);
            }

            {
                float t_proper = trace.x;
                if (t_plane > box_near && t_plane < t_proper && t_plane < box_far)
                {
                    out_color += vec3(0.04, 0.04, 0.04);
                    vec3 p_plane = ro + rd * t_plane;

                    // TODO: use proper distance for intersection effect...
                }
            }
            // out_color.g = normal.y;
            // out_color.b = trace.y;
            // out_color *= trace.y;
            //out_color.b = 0.7f;
            // out_color = normal;
        }



        //out_color = 0;
        //out_color.r = Emulated3D_Sample(i_sdf, vec3(0.5, 0.5, 0.5) - vec3_splat(0.99)/128, uvec2(64, 64), uvec2(8, 8));
    }
    // test:
    vec3 p_test = vec3(4.3, 5.3, 16.7) / uvec3_splat(256); 
    // vec4 r_color = Emulated3D_Sample(i_sdf, p_test, TileSize,MapSize);
    // gl_FragColor = vec4(r_color);
    gl_FragColor = vec4(out_color, 1.0f);
}   