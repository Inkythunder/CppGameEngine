#include <vector>
#include "standard_shapes.h"

void construct_plane(std::vector<ObjVertex>& vertices) {

    const glm::vec3 n(0, 1, 0);
    ObjVertex c0{ {-1, 0, -1}, n, {0, 1} };
    ObjVertex c1{ {-1, 0,  1}, n, {0, 0} };
    ObjVertex c2{ { 1, 0,  1}, n, {1, 0} };
    ObjVertex c3{ { 1, 0, -1}, n, {1, 1} };

    vertices.push_back(c0);
    vertices.push_back(c1);
    vertices.push_back(c2);
    vertices.push_back(c0);
    vertices.push_back(c2);
    vertices.push_back(c3);
};

void construct_cube(std::vector<ObjVertex>& vertices) {

    glm::vec3 cube_corners[8] = {
        {-1, -1, -1}, // left top back
        {-1, -1, 1}, // left top front
        {-1, 1, -1}, // left bottom back
        {-1, 1, 1}, // left bottom front
        {1, -1, -1}, // right top back
        {1, -1, 1}, // right top front
        {1, 1, -1}, // right bottom back
        {1, 1, 1} // right bottom front
    };

    struct Face {
        glm::vec3 normal;
        int corners[4];
    };

    const Face faces[6] = {
        {{ 1,  0,  0}, {5, 4, 6, 7}}, // right
        {{-1,  0,  0}, {0, 1, 3, 2}}, // left
        {{ 0,  1,  0}, {3, 7, 6, 2}}, // top
        {{ 0, -1,  0}, {0, 4, 5, 1}}, // bottom
        {{ 0,  0,  1}, {1, 5, 7, 3}}, // front
        {{ 0,  0, -1}, {4, 0, 2, 6}}  // back
    };

    const glm::vec2 uvs[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };

    for(const Face& face : faces) {
        ObjVertex c0{ cube_corners[face.corners[0]], face.normal, uvs[0] };
        ObjVertex c1{ cube_corners[face.corners[1]], face.normal, uvs[1] };
        ObjVertex c2{ cube_corners[face.corners[2]], face.normal, uvs[2] };
        ObjVertex c3{ cube_corners[face.corners[3]], face.normal, uvs[3] };

        vertices.push_back(c0);
        vertices.push_back(c1);
        vertices.push_back(c2);
        vertices.push_back(c0);
        vertices.push_back(c2);
        vertices.push_back(c3);
    }
}
