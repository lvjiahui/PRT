
//hack 
vec3 get_albedo(vec3 pos)
{
    vec3 albedo = vec3(0.4,0.4,0.4);
    if(pos.x > 5.9){
        albedo = vec3(0.1);
        albedo[(int(pos.y/6+100) + int(pos.z/6+100))% 3] += 0.7;
    }
    return albedo;
}
