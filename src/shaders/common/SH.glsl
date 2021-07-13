uniform sampler3D SH_volume0; //Ar
uniform sampler3D SH_volume1; //Ag
uniform sampler3D SH_volume2; //Ab
uniform sampler3D SH_volume3; //Br
uniform sampler3D SH_volume4; //Bg
uniform sampler3D SH_volume5; //Bb
uniform sampler3D SH_volume6; //C 

const float PI = 3.14159265359;

const float scene_size = 6;

float min3(vec3 v) {
  return min (min (v.x, v.y), v.z);
}

vec3 SH_Irad(vec4 N, vec3 Pos) 
{
    N = N.zxyw; // openGL to directX
    
    vec3 SHCoord = (Pos - (-1*scene_size)) / (2*scene_size); //[0, 1]

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
    // const vec3 rad = x+y+z;
    // if (min3(rad)>0) return rad;
    // return vec3(1,0,0);
}