#include "image.h"
#include "spherical_harmonics.h"
#include <sf_libs/stb_image.h>
#include <numeric>
#include <execution>
namespace sh {

HDR_Image::HDR_Image(std::string path) {
    //stbi_set_flip_vertically_on_load(true);
    int nrComponents;
    pixels_ = stbi_loadf(path.c_str(), &width_, &height_, &nrComponents, 0);
    assert(nrComponents == 3);
}

HDR_Image::~HDR_Image(){
    stbi_image_free(pixels_);
}

int HDR_Image::width() const { return width_; }

int HDR_Image::height() const { return height_; }

Eigen::Array3f HDR_Image::GetPixel(int x, int y) const {
  int index = 3*(x + y * width_);
  return Eigen::Array3f{pixels_[index+0], pixels_[index+1], pixels_[index+2]};
}

void HDR_Image::SetPixel(int x, int y, const Eigen::Array3f& v) {
  int index = 3*(x + y * width_);
  pixels_[index+0] = v.x();
  pixels_[index+1] = v.y();
  pixels_[index+2] = v.z();
}

void HDR_Image::SetAll(std::function<Eigen::Array3f(double, double)> fun)
{
    //for (int t = 0; t < height(); t++) {
    //    double theta = sh::ImageYToTheta(t, height());
    //    for (int p = 0; p < width(); p++) {
    //        double phi = sh::ImageXToPhi(p, width());
    //        SetPixel(p, t, fun(phi, theta));
    //    }
    //}
    std::vector<int> line;
    for (int t = 0; t < height(); t++) line.push_back(t);
    std::for_each(std::execution::par, line.begin(), line.end(), [&fun, this](int t) {
        double theta = sh::ImageYToTheta(t, height());
        for (int p = 0; p < width(); p++) {
            double phi = sh::ImageXToPhi(p, width());
            SetPixel(p, t, fun(phi, theta));
        }
    });
 }



}  // namespace sh