#version 330
in vec3 vPos;
in vec3 vCol;
in vec2 vTex;
in vec3 vNorm;

uniform mat4 MVP;
uniform mat4 model;

out vec3 ourColor;
out vec2 TexCoord;
out vec3 normal;
out vec3 fragPos;

void main()
{
    fragPos = vec3(model * vec4(vPos/300.0, 1.0));
    gl_Position = MVP * vec4(vPos/300.0, 1.0);
    ourColor = vCol;
    TexCoord = vTex;
    normal = mat3(model) * vNorm;
}
