#include "stb_image.h"
#include "image.h"

StbiImage::StbiImage(const std::filesystem::path& path) {

    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(true);
    uint8_t* texture_data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if(!texture_data) {
        throw std::runtime_error("stbi_load failed to load image");
    }

    this->texture_data = texture_data;
    this->width = width;
    this->height = height;
    this->nrChannels = nrChannels;
}

StbiImage::~StbiImage() {
    stbi_image_free(this->texture_data);
}
