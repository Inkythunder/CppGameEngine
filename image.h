#pragma once

#include <cstdint>
#include <filesystem>

struct StbiImage {
    uint8_t* texture_data;
    int width;
    int height;
    int nrChannels;

    StbiImage(const std::filesystem::path& path); // call stbi_load

    StbiImage(const StbiImage& other) = delete;

    ~StbiImage(); // call stbi_free

    StbiImage operator=(const StbiImage& other) = delete;
};
