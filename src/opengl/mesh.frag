#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
// in vec3 Sh_color;  
  
// material parameters
uniform vec3 albedo;
uniform bool metal;
uniform float roughness;
uniform vec3 F0;

uniform mat4 envRotate;
uniform vec3 cameraPos;
uniform bool tonemap;
uniform bool gamma;
uniform bool diffuse;
uniform bool specular;
uniform bool sh;
// uniform sampler3D sh_volume0;
// uniform sampler3D sh_volume1;
// uniform sampler3D sh_volume2;
// uniform sampler3D sh_volume3;
// uniform sampler3D sh_volume4;
// uniform sampler3D sh_volume5;
// uniform sampler3D sh_volume6;
// uniform sampler3D sh_volume7;
// uniform sampler3D sh_volume8;
// const float c1 = 0.429043, c2 = 0.511664,c3 = 0.743125, c4 = 0.886227, c5 = 0.247708;

uniform sampler3D SH_volume0; //Ar
uniform sampler3D SH_volume1; //Ag
uniform sampler3D SH_volume2; //Ab
uniform sampler3D SH_volume3; //Br
uniform sampler3D SH_volume4; //Bg
uniform sampler3D SH_volume5; //Bb
uniform sampler3D SH_volume6; //C 
const float scene_size = 6;
vec3 ShadeIrad(vec4 N) 
{
    N = N.zxyw; // openGL to directX
    
    vec3 SHCoord = (WorldPos - (-1*scene_size)) / (2*scene_size); //[0, 1]

    // const vec3 L00 =  texture(sh_volume0, SHCoord).rgb;
    // const vec3 L1n1 = texture(sh_volume1, SHCoord).rgb;
    // const vec3 L10 =  texture(sh_volume2, SHCoord).rgb;
    // const vec3 L1p1 = texture(sh_volume3, SHCoord).rgb;
    // const vec3 L2n2 = texture(sh_volume4, SHCoord).rgb;
    // const vec3 L2n1 = texture(sh_volume5, SHCoord).rgb;
    // const vec3 L20 =  texture(sh_volume6, SHCoord).rgb;
    // const vec3 L2p1 = texture(sh_volume7, SHCoord).rgb;
    // const vec3 L2p2 = texture(sh_volume8, SHCoord).rgb;

    // mat4 M_r = mat4(c1*L2p2.r,  c1*L2n2.r, c1*L2p1.r, c2*L1p1.r,   // 1. column
    //                 c1*L2n2.r, -c1*L2p2.r, c1*L2n1.r, c2*L1n1.r,    // 2. column
    //                 c1*L2p1.r,  c1*L2n1.r,  c3*L20.r,  c2*L10.r,    // 3. column
    //                 c2*L1p1.r,  c2*L1n1.r,  c2*L10.r,  c4*L00.r - c5*L20.r);

    // mat4 M_g = mat4(c1*L2p2.g,  c1*L2n2.g, c1*L2p1.g, c2*L1p1.g,   // 1. column
    //                 c1*L2n2.g, -c1*L2p2.g, c1*L2n1.g, c2*L1n1.g,    // 2. column
    //                 c1*L2p1.g,  c1*L2n1.g,  c3*L20.g,  c2*L10.g,    // 3. column
    //                 c2*L1p1.g,  c2*L1n1.g,  c2*L10.g,  c4*L00.g - c5*L20.g);

    // mat4 M_b = mat4(c1*L2p2.b,  c1*L2n2.b, c1*L2p1.b, c2*L1p1.b,   // 1. column
    //                 c1*L2n2.b, -c1*L2p2.b, c1*L2n1.b, c2*L1n1.b,    // 2. column
    //                 c1*L2p1.b,  c1*L2n1.b,  c3*L20.b,  c2*L10.b,    // 3. column
    //                 c2*L1p1.b,  c2*L1n1.b,  c2*L10.b,  c4*L00.b - c5*L20.b);

    // return vec3(dot(N, M_r * N), dot(N, M_g * N), dot(N, M_b * N));

    const vec4 Ar = texture(SH_volume0, SHCoord);
    const vec4 Ag = texture(SH_volume1, SHCoord);
    const vec4 Ab = texture(SH_volume2, SHCoord);
    const vec4 Br = texture(SH_volume3, SHCoord);
    const vec4 Bg = texture(SH_volume4, SHCoord);
    const vec4 Bb = texture(SH_volume5, SHCoord);
    const vec3 C  = texture(SH_volume6, SHCoord).rgb;

    const vec3 x = vec3(dot(Ar, N), dot(Ag, N), dot(Ab, N));
    const vec4 BN = N.xxyz * N.yzzz;
    const vec3 y = vec3(dot(Br, BN), dot(Bg, BN), dot(Bb, BN));
    const vec3 z = C * (N.x*N.x - N.y*N.y);
    // return max(x+0.001*y+0.001*z, vec3(0,0,0));
    return max(x+y+z, vec3(0,0,0));
}

uniform sampler2D brdfLUT;
uniform samplerCube prefilterMap;
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
    vec3 color = vec3(0.0);
    vec3 specular_color = vec3(0.0);
    vec3 diffuse_color = vec3(0.0);
    if (specular){
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL  specular part.
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, vec3(envRotate*vec4(R, 1.0)),  roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        specular_color = prefilteredColor * (F * brdf.x + brdf.y); //specular
    }
    if(diffuse && !metal){
        if(sh)
            // diffuse_color = albedo * Sh_color; //sh contains 1/PI
            diffuse_color = albedo * ShadeIrad(envRotate*vec4(N, 1.0)) / PI;
        else
            diffuse_color = albedo/PI * texture(irradiance, vec3(envRotate*vec4(N, 1.0))).rgb; //lambertian
    }
    if(specular && diffuse)
        color = specular_color + (1-F)*diffuse_color;
    else if(specular)
        color = specular_color;
    else if(diffuse)
        color = diffuse_color;

    color = max(color, vec3(0));
    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 