$input v_texcoord0

#include "bgfx_compute.sh"
#include "shaderlib.sh"

#include "emulated_3d.glsl"
#include "sd_and_ray_functions.glsl"


IMAGE2D_WR(i_sdf, r32f, 0);
uniform vec4 u_tile_coord;
uniform vec4 u_tile_map_size;
#define TileSize uvec2(u_tile_map_size.xy)
#define MapSize uvec2(u_tile_map_size.zw)

NUM_THREADS(4, 4, 4)
void main()
{
    uvec3 offset = uvec3(0, 0, 0);
    offset.x = floatBitsToUint(u_tile_coord.x);
    offset.y = floatBitsToUint(u_tile_coord.y);
    offset.z = floatBitsToUint(u_tile_coord.z);

    ivec3 uvw = gl_GlobalInvocationID + offset;


    ivec2 uv = Emulated3D_StoreCoord(uvw, TileSize, MapSize);
    uvec3 VolumeSize = uvec3(TileSize, MapSize.x * MapSize.y);

    vec3 norm_uvw = vec3(gl_GlobalInvocationID + offset) / VolumeSize;
    vec3 centered_uvw = (norm_uvw - vec3_splat(0.5f)); // [-0.5f, 0.5f]
    vec3 p = centered_uvw;

    float d = 10e8;
    
    d = length(centered_uvw) - 0.44f;
    d = sdCappedCylinder(p, 0.35f, 0.2f);
    d = opSmoothSubtraction(sdTorus(p, vec2(0.3f, 0.15f)), d, 0.3f);
    d = opSmoothUnion(d, sdTorus(p, vec2(0.2f, 0.1f)), 0.2f);
    
    // d = max(d, -sdTorus(p, vec2(0.4, 0.35f)));
    // d = max(d, -sdCappedCylinder(p, 0.4f, 0.2f));
    //d = min(d, sdSphere(p, 0.1f));
    // d = length(p) - 0.38f;

    imageStore(i_sdf, uv, d);

}   