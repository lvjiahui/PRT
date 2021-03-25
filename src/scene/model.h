#pragma once
#include "platform/gl.h"


class Model 
{
public:
    // model data 
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(std::string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void render(Shader &shader)
    {
        for(auto& mesh : meshes)
            mesh.render(shader);
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);
};
