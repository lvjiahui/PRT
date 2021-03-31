#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
  
// material parameters
uniform vec3 albedo;
uniform bool metal;
uniform vec3 F0;

uniform vec3 cameraPos;
uniform bool tonemap;
uniform bool gamma;

uniform samplerCube environment;
uniform samplerCube irradiance;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - WorldPos);
    vec3 R = reflect(-V, N); 
    vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);        

    vec3 color = F * texture(environment, R).rgb; //specular
    if(!metal)
        color += albedo/PI * (1 - F) * texture(irradiance, N).rgb; //lambertian

    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 