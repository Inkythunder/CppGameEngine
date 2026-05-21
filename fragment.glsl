#version 330
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in vec3 normal;
in vec3 fragPos;

uniform sampler2D ourTexture;
uniform vec3 vPoint;

void main()
{
    vec4 texColour = texture(ourTexture, TexCoord);

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(vPoint - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.9);

    vec3 result = (ambient + diffuse) * texColour.rgb;
    FragColor = vec4(result * ourColor, 1.0);
}
