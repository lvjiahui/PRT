#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
  
uniform vec3 lightPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    FragColor = vec4(Normal, 1.0);
} 