#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  

struct CastLight {
    vec3 intensity;
    vec3 position;  
    vec3 direction;
    float cutOff;
};

struct PointLight {
    vec3 intensity;
    vec3 position;  
};

uniform CastLight light;
uniform PointLight ambient;

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

void main()
{
    vec3 color = vec3(0, 0, 0);
    vec3 N = normalize(Normal);

    // check if lighting is inside the spotlight cone
    vec3 lightDir = normalize(light.position - WorldPos);
    float theta = dot(lightDir, normalize(-light.direction)); 
    if(theta > light.cutOff)
    {
        float incoming_cos = max(dot(lightDir, N),0);
        float dist = length(light.position - WorldPos);
        color = light.intensity * incoming_cos / (dist*dist);
    }

    //ambient
    float incoming_cos = max(dot(normalize(ambient.position - WorldPos), N),0);
    float dist = length(ambient.position - WorldPos);
    color += ambient.intensity * incoming_cos / (dist*dist);

    //SH
    color += ShadeIrad(vec4(N,1));

    //hack 
    vec3 albedo = vec3(1);
    if(WorldPos.x > 6){
        albedo = vec3(0);
        albedo[(int(WorldPos.y/2+100) + int(WorldPos.z/2+100))% 3] = 1;
    }
    color *= albedo;

    // tonemap
        color = color / (color + vec3(1.0));
    // gamma
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1);

}