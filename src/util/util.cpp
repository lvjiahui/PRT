#include "util.h"
#include "platform/platform.h"
#include <sf_libs/stb_image.h>
#include "sh/spherical_harmonics.h"
#include <Eigen/Dense>
#include <Eigen/Geometry> 

std::vector<float> rotate_cos_lobe(const glm::vec3& normal){ 
    //Note:looking at -z => looking at -x
    //(x,y,z) => (z,x,y)
    Eigen::Vector3d sh_normal{normal.z, normal.x, normal.y};

    //Looking at -x
    //cosine_lobe point to +Z (theta==0)
    static const std::vector<double> cosine_lobe = { 0.886227, 0.0, 1.02333, 0.0, 0.0, 0.0,
                                          0.495416, 0.0, 0.0 };
    Eigen::Quaterniond rotation;
    rotation.setFromTwoVectors(Eigen::Vector3d::UnitZ(), sh_normal).normalize();

    std::vector<double> rotated_cos(9);
    std::unique_ptr<sh::Rotation> sh_rot(sh::Rotation::Create(
      2, rotation));
    sh_rot->Apply(cosine_lobe, &rotated_cos);

    return std::vector<float>{rotated_cos.begin(), rotated_cos.end()};
}

std::vector<glm::vec3> rotate_sh(const std::vector<glm::vec3>& sh, const glm::mat4& rotation){

    Eigen::Matrix3d mat_zxy;
    Eigen::Matrix3d mat_xyz;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            mat_zxy(i,j) = rotation[j][i];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            mat_xyz((i+1)%3, (j+1)%3) = mat_zxy(i, j);
    Eigen::Quaterniond quat{mat_xyz};
    std::vector<Eigen::Array3f> Esh;
    for (auto vec: sh) Esh.emplace_back(vec.x, vec.y, vec.z);

    std::vector<Eigen::Array3f> rotated_sh(9);
    auto sh_rot = sh::Rotation::Create(2, quat);
    sh_rot->Apply(Esh, &rotated_sh);

    std::vector<glm::vec3> result{};
    for (auto vec: rotated_sh) result.emplace_back(vec.x(), vec.y(), vec.z());
    return result;
}


Tex2D load_hdr(std::string path){
    Tex2D tex;
    // pbr: load the HDR environment map
    // ---------------------------------
    // stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        tex.imagef(width, height, data);
        stbi_image_free(data);
    }
    else
    {
        fmt::print("Failed to load HDR image.\n");
    }
    return tex;
}
