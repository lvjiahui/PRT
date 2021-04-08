#version 430 core
out vec4 FragColor;
in vec3 CubeTexPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(-v.x, -v.z), acos(v.y));
    uv *= invAtan;
    uv += vec2(0.5, 0);
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(CubeTexPos)); // make sure to normalize CubeTexPos
    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}