#pragma once

#include <vector>
#include <cstdint>
#include <cstring>


struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vec2 {
    float x;
    float y;

    Vec2 operator*(float scalar) {
        return Vec2(x * scalar, y * scalar);
    }

    Vec2 operator/(float scalar) {
        return Vec2(x / scalar, y / scalar);
    }

    Vec2 operator+(Vec2 other) {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 operator-(Vec2 other) {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2 operator*(Vec2 other) {
        return Vec2(x * other.x, y * other.y);
    }

    Vec2(
        float x,
        float y
    ): x(x), y(y) {}

    Vec2(): x(0.0), y(0.0) {}
};

struct loaded_image {
    size_t width;
    size_t height;
    size_t nr_channels;
    std::vector<uint8_t> image_data;

    loaded_image(
        size_t width,
        size_t height,
        size_t nr_channels,
        unsigned char* image_data
    ): width(width), height(height), nr_channels(nr_channels) {
        this->image_data.resize(width * height * nr_channels);
        memcpy(this->image_data.data(), image_data, width * height * nr_channels);
    }
};


/*
 *1. determine width and height of texture to find largest
 *2. resize texture_data_vector to fit grid of images
 *3. if statement to handle 3 or 4 nr_channels
 *      if 4 memcpy img_width * 4 bytes into texture_bytes
 *      if 3, put a 255 after each pixel
 */
