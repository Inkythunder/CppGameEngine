#include "obj_loader.h"
#include "engine.h"
#include <fstream>

ObjMaterial::ObjMaterial()
    : light_ambient{0, 0, 0},
      light_diffuse{0, 0, 0},
      light_specular{0, 0, 0},
      specular_exponent(0.f) {}


std::string remove_spaces(const std::string& string) {

    size_t spaces_back = 0;
    size_t spaces_front = 0;

    for(size_t i = 0; i < string.size(); i++) {
        if(string[i] != ' ' && string[i] != '\t' && string[i] != '\r') {
            break;
        }
        spaces_front++;
    }
    for(size_t i = string.size(); i > 0; i--) {
        if(string[i-1] != ' ' && string[i-1] != '\t' && string[i-1] != '\r') {
            break;
        }
        spaces_back++;
    }

    return string.substr(spaces_front, string.size() - spaces_back - spaces_front);
}

void convert_to_key_and_values(const std::string& line, std::string& key, std::string& values) {

    std::string::const_iterator it = line.begin();

    bool push_to_key = true;
    while(it != line.end()) {

        if(push_to_key) {
            if(*it == ' ' || *it == '\t') {
                push_to_key = false;
            } else {
                key.push_back(*it);
            }
        } else {
            values.push_back(*it);
        }
        it++;
    }
}

std::vector<std::string> separate_line(const std::string& line, const char separator) {

    std::string token;
    std::vector<std::string> all_tokens;

    for(size_t i=0; i < line.size(); i++) {
        if(line[i] == separator) {
            all_tokens.push_back(token);
            token.clear();
        } else {
            token.push_back(line[i]);
        }
    }
    all_tokens.push_back(token);
    return all_tokens;
}

void loadMaterialFile(const std::filesystem::path& filename, std::map<std::string, ObjMaterial>& materials) {

    const std::filesystem::path parent_path = filename.parent_path();

    std::cout << "Opening mtl " << filename <<std::endl;
    std::ifstream file(filename);
    if(!file) {
        std::cout << "Failed to open material file" << std::endl;
        throw std::runtime_error("Opening mtl file failed");
    }

    file.exceptions(std::ifstream::badbit);

    std::string line;
    ObjMaterial new_material;
    bool new_material_is_set = false;

    std::cout << "Processing material file..." << std::endl;
    while(getline(file, line)) {

        line = remove_spaces(line);

        std::string key;
        std::string value;

        convert_to_key_and_values(line, key, value);
        value = remove_spaces(value);

        if(key == "newmtl") {

            if(new_material_is_set) {
                materials.insert({new_material.name, new_material});
            }
            new_material = ObjMaterial();
            new_material.name = value;
            new_material_is_set = true;

        } else if(key == "Ka") {
            float a, b, c;
            sscanf(value.c_str(), "%f %f %f", &a, &b, &c);
            Vec3 ambient = {a, b, c};
            new_material.light_ambient = ambient;

        } else if(key == "Kd") {
            float a, b, c;
            sscanf(value.c_str(), "%f %f %f", &a, &b, &c);
            Vec3 diffuse = {a, b, c};
            new_material.light_diffuse = diffuse;

        } else if(key == "Ks") {
            float a, b, c;
            sscanf(value.c_str(), "%f %f %f", &a, &b, &c);
            Vec3 specular = {a, b, c};
            new_material.light_specular = specular;

        } else if(key == "Ns") {
            float ns = stof(value);
            new_material.specular_exponent = ns;

        } else if(key == "map_Kd") {
            std::string texture = value;
            new_material.diffuse_texture = parent_path / texture;
        }
    }
    if(new_material_is_set) {
        materials.insert({new_material.name, new_material});
    }
    std::cout << "Finished loading material :3" << std::endl;
}

ObjModel loadObjFile(const std::filesystem::path& filename, std::map<std::string, ObjMaterial>& materials) {

    const std::filesystem::path parent_path = filename.parent_path();
    std::cout << "Opening " << filename << std::endl;
    std::ifstream file(filename);
    if(!file) {
        std::cout << "Failed to open file" << std::endl;
        throw std::runtime_error("Opening file failed");
    }

    file.exceptions(std::ifstream::badbit);

    std::string line;
    ObjModel model;
    std::vector<Vec3> file_vertices;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;
    ObjMesh* current_mesh_ptr = nullptr;

    Vec3 max_size;
    Vec3 min_size;

    std::cout << "Processing object file..." << std::endl;
    while(getline(file, line)) {

        line = remove_spaces(line);

        std::string key;
        std::string value;
        convert_to_key_and_values(line, key, value);
        value = remove_spaces(value);

        if(key == "mtllib") {
            const std::filesystem::path mtl_file = parent_path / value;
            std::cout << "mtllib = " << value << std::endl;
            std::cout << "mtl_file = " << mtl_file << std::endl;
            loadMaterialFile(mtl_file, materials);

        } /*else if(key == "g") {
            if(!current_mesh.vertices.empty()) {
                model.meshes[current_mesh.name] = current_mesh;
            }
            current_mesh.name = value;
            current_mesh.vertices.clear();

        } */else if(key == "v") {
            float x, y, z;
            sscanf(value.c_str(), "%f %f %f", &x, &y, &z);
            Vec3 position = {x, y, z};
            file_vertices.push_back(position);

        } else if(key == "vt") {
            float u, v;
            sscanf(value.c_str(), "%f %f", &u, &v);
            uvs.push_back({u, v});

        } else if(key == "vn") {
            float x, y, z;
            sscanf(value.c_str(), "%f %f %f", &x, &y, &z);
            normals.push_back({x, y, z});

        } else if (key == "usemtl") {
            current_mesh_ptr = nullptr;

            if(model.meshes.find(value) == model.meshes.end()) {
                ObjMesh new_mesh;
                new_mesh.material_name = value;
                model.meshes.insert({value, new_mesh});
            }
            current_mesh_ptr = &model.meshes[value];

        } else if(key == "f") {

            std::vector<ObjVertex> face_vertices;

            std::vector<std::string> tokens = separate_line(value, ' ');
            for(size_t i = 0; i < tokens.size(); i++) {

                const std::vector<std::string> face_values = separate_line(tokens[i], '/');

                uint32_t v = 0, t = 0, n = 0;
                ObjVertex vertex;

                v = std::stoul(face_values[0]);
                vertex.position = file_vertices[v - 1];

                if(face_values.size() > 1 && !face_values[1].empty()) {
                    t = std::stoul(face_values[1]);
                    vertex.uv = uvs[t - 1];
                }

                if(face_values.size() > 2 && !face_values[2].empty()) {
                    n = std::stoul(face_values[2]);
                    vertex.normal = normals[n - 1];
                }

                face_vertices.push_back(vertex);

                max_size = max_size.max(vertex.position);
                min_size = min_size.min(vertex.position);
            }

            for(size_t i = 0; i + 2 < face_vertices.size(); i++) {
                current_mesh_ptr->vertices.push_back(face_vertices[0]);
                current_mesh_ptr->vertices.push_back(face_vertices[i + 1]);
                current_mesh_ptr->vertices.push_back(face_vertices[i + 2]);
            }
        }
    }

    std::cout << "Finished loading object :3" << std::endl;

    model.dimensions = max_size - min_size;
    return model;
}
