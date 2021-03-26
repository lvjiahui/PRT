#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "model.h"
#include <assimp/postprocess.h>

// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.


static glm::vec3 aiVec(aiVector3D aiv) { return glm::vec3(aiv.x, aiv.y, aiv.z); }

// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
Mesh processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    std::vector<Mesh::Vert> vertices;
    std::vector<Mesh::Index> indices;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        auto vpos = mesh->mVertices[i];
        // normals
        aiVector3D vnorm{};
        if (mesh->HasNormals())
        {
            vnorm = mesh->mNormals[i];
        }
        vertices.push_back({ aiVec(vpos), aiVec(vnorm) });

    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // return a mesh object created from the extracted mesh data
    return Mesh(std::move(vertices), std::move(indices));
}

void processNode(Model& model, aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(model, node->mChildren[i], scene);
    }

}

void Model::loadModel(std::string const& path)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        fmt::print("ERROR::ASSIMP:: {}\n", importer.GetErrorString());
        return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(*this, scene->mRootNode, scene);
}