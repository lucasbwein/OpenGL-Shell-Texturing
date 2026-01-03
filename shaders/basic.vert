#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 FragPos;
out vec3 vLocalPos;
flat out int vInstanceID;

out float vLayer;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int uNumLayers;
uniform float uFurLength;

uniform vec3 uGravity;
uniform vec3 uWindDirection;

void main()
{
    float layer = (uNumLayers > 1) // ensures that if layer less than 1 will be 0
        ? float(gl_InstanceID) / float(uNumLayers - 1)
        : 0.0;
    vLayer = layer;

    vec3 shellPos = aPos + aNormal * (layer * uFurLength); // base shell position

    float layerSquared = layer * layer; // used to bend like hair rather than uniform
    shellPos += uGravity * layerSquared * 0.1; // applies minimal gravity
    // Applies wind direction based on movement
    shellPos += uWindDirection * layerSquared * 0.15;

    vInstanceID = gl_InstanceID;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vNormal = normalMatrix * aNormal;   

    vLocalPos = aPos;
    vTexCoord = aTexCoord;

    vec4 worldPos = model * vec4(shellPos, 1.0); // uses shell position instead
    FragPos = worldPos.xyz;

    gl_Position = projection * view * worldPos;
}