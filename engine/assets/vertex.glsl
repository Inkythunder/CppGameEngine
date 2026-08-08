#version 330
in vec3 vPos;
in vec2 vTex;
in vec3 vNorm;

uniform mat4 VP;
uniform mat4 model;

out vec2 TexCoord;
out vec3 normal;
out vec3 fragPos;

void main()
{
    fragPos = vec3(model * vec4(vPos, 1.0));
    gl_Position = VP * model * vec4(vPos, 1.0);
    TexCoord = vTex;
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    normal = normalize(normalMatrix * vNorm);
}
