#pragma once
#include "opengl/gl.h"


class Model 
{
public:
    // model data 
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;
    glm::mat4 Mat_model = glm::mat4(1);

    // constructor, expects a filepath to a 3D model.
    Model(std::string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void render(Shader& shader)
    {
        for (auto& mesh : meshes) {
            mesh.Mat_model = Mat_model;
            mesh.render(shader);
        }
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);
};
