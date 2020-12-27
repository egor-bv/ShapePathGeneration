

// Problem: s0 == s1 every time? might just be bad tests...
vec4 Emulated3D_Sample(BgfxSampler2D tiled_texture, vec3 uvw, uvec2 tile_size, uvec2 map_size)
{
    uvw = saturate(uvw);
    uvec3 size3d = uvec3(tile_size, map_size.x * map_size.y);
    vec3 puvw = uvw * size3d;
    
    uint tile_index = floor(puvw.z - 0.5f);

    uvec2 map_xy0 = uvec2(tile_index % map_size.x, tile_index / map_size.x);
    uvec2 map_xy1 = uvec2((tile_index + 1) % map_size.x, (tile_index + 1) / map_size.x);
    
    vec2 pxy0 = map_xy0 * tile_size + puvw.xy;
    vec2 pxy1 = map_xy1 * tile_size + puvw.xy;

    vec2 xy0 = pxy0 / (map_size * tile_size);
    vec2 xy1 = pxy1 / (map_size * tile_size);

    vec4 s0 = bgfxTexture2D(tiled_texture, xy0);
    vec4 s1 = bgfxTexture2D(tiled_texture, xy1);

    float a0 = (tile_index + 1.5f) - puvw.z;

    vec4 result = mix(s0, s1, 1-a0);
    //result.g = tile_index;
    //result.b = a0;
    //result.r = map_xy0.y;
    // result = s0;

    //result.rg = pxy0;
    //result.ba = pxy1;
    return result;

}


// tile = xy layer, tiles are arranged in a map
ivec2 Emulated3D_StoreCoord(ivec3 iuvw, uvec2 tile_size, uvec2 map_size)
{
    uint tile_index = iuvw.z;
    uvec2 map_xy= uvec2(tile_index % map_size.x, tile_index / map_size.x);

    ivec2 result;
    result = ivec2(map_xy* tile_size + iuvw.xy);
    return result;
}