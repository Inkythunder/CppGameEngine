// unittest.cpp — exercise the loadObjFile() API from obj_loader.h.
//
// Build:    g++ -std=c++17 unittest.cpp obj_loader.cpp -o unittest
// Run:      ./unittest
//
// Writes small .obj/.mtl files to ./test_files/ at startup, then loads them
// and checks the resulting Model. Exits on the first failing assertion.

#include "obj_loader.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// =====================================================================
// Tiny test harness — exits on first failure as requested.
// =====================================================================

static int          g_test_num     = 0;
static const char  *g_current_test = "<none>";

static void fail_header(int line) {
    std::cerr << "\n[FAIL] test \"" << g_current_test
              << "\" at unittest.cpp:" << line << "\n";
}

#define BEGIN_TEST(name)                                                       \
    do {                                                                       \
        ++g_test_num;                                                          \
        g_current_test = (name);                                               \
        std::cout << "[" << g_test_num << "] " << g_current_test << " ... "    \
                  << std::flush;                                               \
    } while (0)

#define END_TEST() std::cout << "OK\n"

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fail_header(__LINE__);                                             \
            std::cerr << "  CHECK failed: " #cond "\n";                        \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

#define CHECK_EQ(a, b)                                                         \
    do {                                                                       \
        auto _va = (a);                                                        \
        auto _vb = (b);                                                        \
        if (!(_va == _vb)) {                                                   \
            fail_header(__LINE__);                                             \
            std::cerr << "  CHECK_EQ failed: " #a " == " #b "\n";              \
            std::cerr << "    lhs = " << _va << "\n";                          \
            std::cerr << "    rhs = " << _vb << "\n";                          \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

static bool feq(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) < eps;
}

#define CHECK_FEQ(a, b)                                                        \
    do {                                                                       \
        float _fa = (a);                                                       \
        float _fb = (b);                                                       \
        if (!feq(_fa, _fb)) {                                                  \
            fail_header(__LINE__);                                             \
            std::cerr << "  CHECK_FEQ failed: " #a " ~= " #b "\n";             \
            std::cerr << "    lhs = " << _fa << "\n";                          \
            std::cerr << "    rhs = " << _fb << "\n";                          \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

#define CHECK_VEC3(v, x, y, z)                                                 \
    do {                                                                       \
        CHECK_FEQ((v).a, (x));                                                 \
        CHECK_FEQ((v).b, (y));                                                 \
        CHECK_FEQ((v).c, (z));                                                 \
    } while (0)

#define CHECK_VEC2(v, x, y)                                                    \
    do {                                                                       \
        CHECK_FEQ((v).a, (x));                                                 \
        CHECK_FEQ((v).b, (y));                                                 \
    } while (0)

// =====================================================================
// File helpers
// =====================================================================

static void write_file(const fs::path &path, const std::string &content) {
    std::ofstream f(path);
    if (!f) {
        std::cerr << "Could not open " << path << " for writing\n";
        std::exit(2);
    }
    f << content;
}

// =====================================================================
// Tests on standard .obj data (these should pass on a correct loader)
// =====================================================================

// 1. Simplest possible mesh: one triangle, full v/vt/vn indices.
static void test_basic_triangle() {
    BEGIN_TEST("basic triangle (v/vt/vn)");

    const std::string content =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vt 1.0 0.0\n"
        "vt 0.0 1.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1/1/1 2/2/1 3/3/1\n";
    write_file("test_files/triangle.obj", content);

    Model m = loadObjFile("test_files/triangle.obj");
    CHECK_EQ(m.meshes.size(), (size_t)1);

    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)3);
    CHECK_VEC3(mesh.vertices[0].position, 0.0f, 0.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[1].position, 1.0f, 0.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[2].position, 0.0f, 1.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[0].normal,   0.0f, 0.0f, 1.0f);
    CHECK_VEC2(mesh.vertices[1].uv,       1.0f, 0.0f);

    END_TEST();
}

// 2. A 4-vertex face must be triangulated into 2 triangles (fan from v0).
static void test_quad_triangulation() {
    BEGIN_TEST("quad face is triangulated into 2 triangles");

    const std::string content =
        "v -1.0 -1.0 0.0\n"
        "v  1.0 -1.0 0.0\n"
        "v  1.0  1.0 0.0\n"
        "v -1.0  1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vt 1.0 0.0\n"
        "vt 1.0 1.0\n"
        "vt 0.0 1.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1/1/1 2/2/1 3/3/1 4/4/1\n";
    write_file("test_files/quad.obj", content);

    Model m = loadObjFile("test_files/quad.obj");
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)6);

    // Triangle 1: v0, v1, v2
    CHECK_VEC3(mesh.vertices[0].position, -1.0f, -1.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[1].position,  1.0f, -1.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[2].position,  1.0f,  1.0f, 0.0f);
    // Triangle 2: v0, v2, v3
    CHECK_VEC3(mesh.vertices[3].position, -1.0f, -1.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[4].position,  1.0f,  1.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[5].position, -1.0f,  1.0f, 0.0f);

    END_TEST();
}

// 3. A 5-vertex face becomes 3 triangles.
static void test_pentagon_triangulation() {
    BEGIN_TEST("pentagon face is triangulated into 3 triangles");

    const std::string content =
        "v  0.0 0.0 0.0\n"
        "v  1.0 0.0 0.0\n"
        "v  1.5 1.0 0.0\n"
        "v  0.5 1.5 0.0\n"
        "v -0.5 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1/1/1 2/1/1 3/1/1 4/1/1 5/1/1\n";
    write_file("test_files/pentagon.obj", content);

    Model m = loadObjFile("test_files/pentagon.obj");
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)9); // 3 triangles * 3 verts

    END_TEST();
}

// 4. `g` directives split geometry into separate named meshes.
static void test_multiple_groups() {
    BEGIN_TEST("multiple `g` groups produce separate meshes");

    const std::string content =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "v 2.0 0.0 0.0\n"
        "v 3.0 0.0 0.0\n"
        "v 2.0 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "g first\n"
        "f 1/1/1 2/1/1 3/1/1\n"
        "g second\n"
        "f 4/1/1 5/1/1 6/1/1\n";
    write_file("test_files/groups.obj", content);

    Model m = loadObjFile("test_files/groups.obj");
    CHECK_EQ(m.meshes.size(), (size_t)2);
    CHECK(m.meshes.find("first")  != m.meshes.end());
    CHECK(m.meshes.find("second") != m.meshes.end());
    CHECK_EQ(m.meshes.at("first").vertices.size(),  (size_t)3);
    CHECK_EQ(m.meshes.at("second").vertices.size(), (size_t)3);
    CHECK_VEC3(m.meshes.at("second").vertices[0].position, 2.0f, 0.0f, 0.0f);

    END_TEST();
}

// =====================================================================
// Edge cases / tests targeted at specific bugs in obj_loader.cpp
// =====================================================================

// 5. Blank lines and `#` comments are normal in real .obj files.
//
//    Targets remove_spaces(): the back-scan loop is
//        for (size_t i = string.size()-1; i >= 0; i--)
//    `i >= 0` is always true for size_t, and for an empty line
//    `string.size()-1` underflows to SIZE_MAX before the loop body
//    even runs, producing UB on the string[i] read.
static void test_comments_and_blank_lines() {
    BEGIN_TEST("comments and blank lines do not crash the parser");

    const std::string content =
        "# Exported by some tool\n"
        "\n"                              // blank line in the middle
        "v 0.0 0.0 0.0\n"
        "# midway comment\n"
        "v 1.0 0.0 0.0\n"
        "\n"
        "v 0.0 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1/1/1 2/1/1 3/1/1\n";
    write_file("test_files/comments.obj", content);

    Model m = loadObjFile("test_files/comments.obj");
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)3);
    CHECK_VEC3(mesh.vertices[0].position, 0.0f, 0.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[1].position, 1.0f, 0.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[2].position, 0.0f, 1.0f, 0.0f);

    END_TEST();
}

// 6. Lines with BOTH leading and trailing whitespace.
//
//    Targets remove_spaces() length math:
//        substr(spaces_front, string.size() - spaces_back)
//    should subtract spaces_front too. With only leading or only
//    trailing whitespace this happens to give the right answer; with
//    both, trailing whitespace survives.
//
//    So "   g my_group   " becomes key="g", value="my_group   ",
//    and the mesh is stored under "my_group   " (with trailing spaces),
//    making the lookup for "my_group" fail.
static void test_leading_and_trailing_whitespace() {
    BEGIN_TEST("lines with both leading AND trailing whitespace");

    const std::string content =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "   g my_group   \n"
        "f 1/1/1 2/1/1 3/1/1\n";
    write_file("test_files/lead_trail_ws.obj", content);

    Model m = loadObjFile("test_files/lead_trail_ws.obj");
    CHECK_EQ(m.meshes.size(), (size_t)1);
    CHECK(m.meshes.find("my_group") != m.meshes.end());
    CHECK_EQ(m.meshes.at("my_group").vertices.size(), (size_t)3);

    END_TEST();
}

// 7. Faces in `v//n` form — legal .obj when there are no UVs.
//
//    Targets the face-parsing block. When face_values[1] is empty,
//    `t` stays 0. The code then reads `uvs[t - 1]`, and because `t`
//    is uint32_t the subtraction wraps to UINT32_MAX. With uvs empty
//    this is far out of bounds.
static void test_face_without_uv() {
    BEGIN_TEST("face indices without UV (v//n form)");

    const std::string content =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1//1 2//1 3//1\n";
    write_file("test_files/no_uv.obj", content);

    Model m = loadObjFile("test_files/no_uv.obj");
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)3);
    CHECK_VEC3(mesh.vertices[0].position, 0.0f, 0.0f, 0.0f);
    CHECK_VEC3(mesh.vertices[0].normal,   0.0f, 0.0f, 1.0f);

    END_TEST();
}

// 8. Faces in `v` form — legal .obj when there are no UVs or normals.
//
//    Same uvs[t-1] / normals[n-1] underflow issue as test 7, doubled.
static void test_face_only_position() {
    BEGIN_TEST("face indices with positions only (v form)");

    const std::string content =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3\n";
    write_file("test_files/only_pos.obj", content);

    Model m = loadObjFile("test_files/only_pos.obj");
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices.size(), (size_t)3);
    CHECK_VEC3(mesh.vertices[0].position, 0.0f, 0.0f, 0.0f);

    END_TEST();
}

// 9. Materials referenced via `mtllib`/`usemtl`.
//
//    Targets the most severe bug in the file: in loadMaterialFile()
//        Material new_material;
//        bool new_material_is_set = false;
//    are declared *inside* the while-loop. They are reset on every
//    iteration, so the push_back in the `newmtl` branch always sees
//    `new_material_is_set == false`, and the final material is never
//    pushed after the loop ends either. The materials vector is
//    always empty in the version under test.
static void test_material_loading() {
    BEGIN_TEST("materials from .mtl are loaded and indexed");

    const std::string mtl =
        "newmtl Red\n"
        "Ka 0.1 0.0 0.0\n"
        "Kd 0.8 0.0 0.0\n"
        "Ks 1.0 1.0 1.0\n"
        "Ns 32.0\n"
        "\n"
        "newmtl Blue\n"
        "Ka 0.0 0.0 0.1\n"
        "Kd 0.0 0.0 0.8\n"
        "Ks 0.5 0.5 0.5\n"
        "Ns 16.0\n";
    write_file("test_files/two_mats.mtl", mtl);

    const std::string obj =
        "mtllib two_mats.mtl\n"
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vt 0.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "usemtl Blue\n"
        "f 1/1/1 2/1/1 3/1/1\n";
    write_file("test_files/with_mtl.obj", obj);

    Model m = loadObjFile("test_files/with_mtl.obj");
    CHECK_EQ(m.materials.size(), (size_t)2);
    CHECK_EQ(m.materials[0].name, std::string("Red"));
    CHECK_EQ(m.materials[1].name, std::string("Blue"));
    CHECK_VEC3(m.materials[0].light_diffuse, 0.8f, 0.0f, 0.0f);
    CHECK_VEC3(m.materials[1].light_diffuse, 0.0f, 0.0f, 0.8f);
    CHECK_FEQ(m.materials[0].specular_exponent, 32.0f);
    CHECK_FEQ(m.materials[1].specular_exponent, 16.0f);

    // `usemtl Blue` should mean faces reference material index 1.
    const Mesh &mesh = m.meshes.begin()->second;
    CHECK_EQ(mesh.vertices[0].material_index, (uint32_t)1);

    END_TEST();
}

// 10. map_Kd diffuse texture path in .mtl.
static void test_map_kd() {
    BEGIN_TEST("map_Kd diffuse texture path is parsed");

    const std::string mtl =
        "newmtl Tex\n"
        "Kd 1.0 1.0 1.0\n"
        "map_Kd brick.png\n";
    write_file("test_files/tex.mtl", mtl);

    const std::string obj =
        "mtllib tex.mtl\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vn 0 0 1\n"
        "usemtl Tex\n"
        "f 1/1/1 2/1/1 3/1/1\n";
    write_file("test_files/with_tex.obj", obj);

    Model m = loadObjFile("test_files/with_tex.obj");
    CHECK_EQ(m.materials.size(), (size_t)1);
    CHECK_EQ(m.materials[0].diffuse_texture, std::string("brick.png"));

    END_TEST();
}

// =====================================================================
// main
// =====================================================================
int main() {
    try {
        fs::create_directories("test_files");
    } catch (const std::exception &e) {
        std::cerr << "Could not create test_files/: " << e.what() << "\n";
        return 2;
    }

    // Standard format — should pass on a correct loader.
    test_basic_triangle();
    test_quad_triangulation();
    test_pentagon_triangulation();
    test_multiple_groups();

    // Edge cases / bugs visible in the current obj_loader.cpp.
    test_comments_and_blank_lines();
    test_leading_and_trailing_whitespace();
    test_face_without_uv();
    test_face_only_position();
    test_material_loading();
    test_map_kd();

    std::cout << "\nAll " << g_test_num << " tests passed.\n";
    return 0;
}
