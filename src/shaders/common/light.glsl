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

struct ParalLight {
    vec3 intensity; 
    vec3 direction;
};

vec3 Eval_CastLight(const CastLight light, const vec3 Pos, const vec3 N){
    // check if lighting is inside the spotlight cone
    vec3 lightDir = normalize(light.position - Pos);
    float theta = dot(lightDir, normalize(-light.direction)); 
    if(theta > light.cutOff)
    {
        float incoming_cos = max(dot(lightDir, N),0);
        float dist = length(light.position - Pos);
        float softedge = (theta-light.cutOff)/(1-light.cutOff);
        vec3 radiance = softedge * light.intensity * incoming_cos / (dist*dist);
        return radiance;
    }
    return vec3(0,0,0);
}

vec3 Eval_PointLight(const PointLight light, const vec3 Pos, const vec3 N){
    if (light.intensity != vec3(0,0,0)){
        float incoming_cos = max(dot(normalize(light.position - Pos), N),0);
        float dist = length(light.position - Pos);
        vec3 radiance = light.intensity * incoming_cos / (dist*dist);
        return radiance;
    }
    return vec3(0,0,0);
}

vec3 Eval_ParalLight(const ParalLight light, const vec3 N){
    float incoming_cos = max(dot(light.direction, N),0);
    return light.intensity * incoming_cos;
}
