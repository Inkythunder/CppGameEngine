#version 330
out vec4 FragColor;

in vec2 TexCoord;
in vec3 normal;
in vec3 fragPos;

uniform sampler2D ourTexture;
uniform vec3 vPoint;
uniform vec3 ka;
uniform vec3 kd;
uniform float textureless;

void main()
{
    vec4 texColour = texture(ourTexture, TexCoord) + vec4(textureless, textureless, textureless, 0.0);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(vPoint - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    FragColor = vec4(texColour.rgb * kd * diff, 1.0);
}
