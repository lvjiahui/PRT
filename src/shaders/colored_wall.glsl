
//hack 
vec3 get_albedo(vec3 pos)
{
    vec3 albedo = vec3(0.8);
    if(pos.x > 6){
        albedo = vec3(0.1);
        albedo[(int(pos.y/2+100) + int(pos.z/2+100))% 3] += 0.8;
    }
    return albedo;
}
