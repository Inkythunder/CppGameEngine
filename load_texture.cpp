#include "load_texture.h"
#include "stb_image.h"

combined_texture_type load_texture(std::vector<std::filesystem::path> textures) {

    combined_texture_type texture_index_uv;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    stbi_set_flip_vertically_on_load(true);

    size_t grid_size = 0;
    while(1) {
        grid_size++;
        if(grid_size * grid_size >= textures.size()) {
            break;
        }
    }

    std::vector<loaded_image> texture_data_vector;
    std::vector<uint8_t> texture_bytes;

    // The biggest width and height in the list of textures
    uint32_t largest_width = 0;
    uint32_t largest_height = 0;

    // find size of the largest texture
    for(size_t i = 0; i < textures.size() ; i++) {
        int width, height, nrChannels;

        unsigned char* texture_data = stbi_load(textures[i].c_str(), &width, &height, &nrChannels, 0);
        loaded_image img(width, height, nrChannels, texture_data);
        texture_data_vector.push_back(img);

        if(largest_width < width) {
            largest_width = width;
        }
        if(largest_height < height) {
            largest_height = height;
        }
    }

    texture_bytes.resize((grid_size * largest_width) * (grid_size * largest_height) * 4);

    // fill the texture vector with bytes from images
    for(size_t i = 0; i < texture_data_vector.size(); i++) {
        loaded_image& img = texture_data_vector[i];

        size_t col = i % grid_size;
        size_t row = i / grid_size;
        size_t cell_top_left_x = col * largest_width;
        size_t cell_top_left_y = row * largest_height;

        size_t combined_width = grid_size * largest_width;

        // if 3 channels (RGB) copy 3 bytes then add 255 for alpha channel
        if(img.nr_channels == 3) {
            for(size_t y = 0; y < img.height; y++) {
                for(size_t x = 0; x < img.width; x++) {

                    size_t source_texture_offset = (y * img.width + x) * 3;
                    size_t destin_texture_offset = ((cell_top_left_y + y) * combined_width + (cell_top_left_x + x)) * 4;

                    memcpy(texture_bytes.data() + destin_texture_offset, img.image_data.data() + source_texture_offset, 3);
                    texture_bytes[destin_texture_offset + 3] = 255;
                }
            }
        }
        // if 4 channels (RGBA) copy 4 bytes
        if(img.nr_channels == 4) {
            for(size_t y = 0; y < img.height; y++) {

                size_t source_texture_offset = y * img.width * 4;
                size_t destin_texture_offset = ((cell_top_left_y + y) * combined_width + cell_top_left_x) * 4;

                memcpy(texture_bytes.data() + destin_texture_offset, img.image_data.data() + source_texture_offset, img.width * 4);
            }
        }
        texture_uvs new_cell(
            Vec2(cell_top_left_x, cell_top_left_y),
            Vec2(cell_top_left_x + img.width, cell_top_left_y + img.height)
        );
        texture_index_uv.material_index_to_uvs[i] = new_cell;
    }

    texture_index_uv.combined_texture = texture;

    return texture_index_uv;
}
