#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out float gPrimitiveID;
// layout (location = 2) out vec4 gAlbedoSpec;

in vec3 Normal;  
in vec3 WorldPos;  

void main()
{
    // store the fragment position vector in the first gbuffer texture
    gPosition = WorldPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // // and the diffuse per-fragment color
    // gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // // store specular intensity in gAlbedoSpec's alpha component
    // gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;

    gPrimitiveID = gl_PrimitiveID;
}