#include "load.h"
#include "platform/platform.h"
#include <sf_libs/stb_image.h>


Tex2D load_hdr(std::string path){
    Tex2D tex;
    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
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
