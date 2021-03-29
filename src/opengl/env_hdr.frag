#version 330 core
out vec4 FragColor;
in vec3 CubePos;

uniform sampler2D equirectangularMap;
uniform bool tonemap;
uniform bool gamma;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(CubePos)); // make sure to normalize CubePos
    vec3 color = texture(equirectangularMap, uv).rgb;
    if(tonemap)
    color = color / (color + vec3(1.0));
    if(gamma)
    color = pow(color, vec3(1.0/2.2)); 
    
    FragColor = vec4(color, 1.0);
}