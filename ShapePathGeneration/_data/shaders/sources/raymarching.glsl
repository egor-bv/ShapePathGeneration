#ifndef RAYMARCHING_HEADER_GUARD
#define RAYMARCHING_HEADER_GUARD

// Must #define SDF_SCENE(p) -> vec2(distance, material)
#define SDF_SCENE_D(p) SDF_SCENE(p).x

#define STEP_LIMIT 128
#define CONE_EPSILON 0.0003


// returns t = distance traveled along the ray
vec2 TraceRay(vec3 ro, vec3 rd, float tmax)
{
    float t = 0;
    float mtl_index = 0;

    LOOP
    for (int i = 0; i < STEP_LIMIT; i++)
    {
        vec3 p = ro + rd * t;
        vec2 dm = SDF_SCENE(p);
        float d = dm.x;
        float m = dm.y;

        // Hit
        if (d < t * CONE_EPSILON)
        {
            mtl_index = m;
            // mtl_index = i;
            break;

        }

        // Miss
        if (t >= tmax)
        {
            mtl_index = 0;
            break;
        }

        t += d;
    }
    
    return vec2(t, mtl_index);
}

#if 1
vec3 EstimateNormal(vec3 p)
{
    const float h = 0.0001;
    const vec2 k = vec2(1, -1);
    return normalize(
        k.xyy * SDF_SCENE_D(p + k.xyy * h) +
        k.yyx * SDF_SCENE_D(p + k.yyx * h) +
        k.yxy * SDF_SCENE_D(p + k.yxy * h) +
        k.xxx * SDF_SCENE_D(p + k.xxx * h)
    );
}
#endif

vec3 EstimateNormalFull( in vec3 p )
{
    const float eps = 0.0001; // or some other value
    const vec2 h = vec2(eps,0);
    return normalize( vec3(SDF_SCENE_D(p+h.xyy) - SDF_SCENE_D(p-h.xyy),
                           SDF_SCENE_D(p+h.yxy) - SDF_SCENE_D(p-h.yxy),
                           SDF_SCENE_D(p+h.yyx) - SDF_SCENE_D(p-h.yyx) ) );
}



#endif //RAYMARCHING_HEADER_GUARD