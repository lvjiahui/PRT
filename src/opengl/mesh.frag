#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
  
uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform bool tonemap;
uniform bool gamma;

uniform sampler2D equirectangularMap;

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
    vec3 I = normalize(WorldPos - cameraPos);
    vec3 R = reflect(I, normalize(Normal));

    // vec3 color = texture(skybox, R).rgb;
    vec2 uv = SampleSphericalMap(R);
    vec3 color = texture(equirectangularMap, uv).rgb;

    if(tonemap)
    color = color / (color + vec3(1.0));
    if(gamma)
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 