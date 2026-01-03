
#version 330 core
out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct SpotLight {
    vec3 position;
    vec3 direction;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float cutOff;
    float outerCutOff;
    int FlashLightEnable;
};

#define NR_POINT_LIGHTS 4

in vec3 vNormal;
in vec3 FragPos;
in vec2 vTexCoord;
in vec3 vLocalPos;
flat in int vInstanceID;

in float vLayer;

uniform float currFrame;

uniform vec3 viewPos;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

uniform vec3 baseColor;
uniform int uNumLayers;
uniform float uStrandThickness;
uniform float uGridFrequency;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // Basic properties for lighting
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - FragPos);

    float layer = vLayer;

    // Vars for Hair properties
    float strandThickness = uStrandThickness;
    float gridFrequency = uGridFrequency; 
    vec2 uv = vTexCoord * gridFrequency;
    vec2 cell = floor(uv);

    // hash like function for shell texturing
    float height = fract(sin(dot(cell, vec2(37.7, 17.7))) * 43758.5453);

    // Lambertian diffuse => diffuse = max(dot(N, dirToLight), 0)
    vec3 lightDir = normalize(-dirLight.direction);
    float lambertDiffuse = max(dot(norm, lightDir), 0) * 0.5 + 0.5;

    if(vInstanceID == 0) { // draws the base layer (first instance)
        float variation = mix(0.9, 1.1, height);
        vec3 baseResult = baseColor * variation * lambertDiffuse;
        FragColor = vec4(baseResult, 1.0);
        return;
    }
    // makes lower layers thicker
    float earlyLayerBoost = (layer < 0.3) ? 1.0 : 0.7;

    vec2 localUV = fract(uv) * 2.0 - 1.0;
    // used to compare against distance from center
    float distFromCenter = length(localUV); 

    if(height < layer) discard; // Discards based on noise from height

    float remain = height - layer;
    float radius = strandThickness * remain * (1.0 + earlyLayerBoost * 0.3);

    // distance from center > thickness * (random - h)
    if(distFromCenter > radius) discard; // thins out the hair strands

    float viewDot = abs(dot(norm, viewDir));
    float edgeFade = smoothstep(0.0, 0.4, viewDot);
    float layerFade = smoothstep(0.85, 1.0, layer);
    
    float alpha = edgeFade * (1.0 - layerFade); // if opacity too small discard

    if(alpha < 0.01) discard;

    float rootToTip = layer;
    float shade = mix(0.5, 1.0, 1.0 - rootToTip);
    float variation = mix(0.9, 1.1, height);

    // Creates darker base and lighter tips
    vec3 furColor = baseColor * shade * variation; 

    if (spotLight.FlashLightEnable == 1)
        furColor += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(furColor * lambertDiffuse, alpha); 
    
    // -- Testing
    // FragColor = vec4(vec3(layer), 1.0);
    // FragColor = vec4(vec3(height), 1.0);
    // -- Shows UV index
    // FragColor = vec4(vec3(fract(vTexCoord * 200.0) * 2-1, 1.0), 1.0);
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.0f);
    // combine
    vec3 ambient  = light.ambient;
    vec3 diffuse  = light.diffuse  * diff;
    vec3 specular = light.specular * spec;
    return (ambient + diffuse + specular);
}
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    // dims lighting based on distance (attenuation)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                                light.quadratic * (distance * distance));
    vec3 ambient  = light.ambient;
    vec3 diffuse  = light.diffuse  * diff;
    vec3 specular = light.specular * spec;
    ambient  *= attenuation; 
    diffuse  *= attenuation;
    specular *= attenuation; 
    return (ambient + diffuse + specular);
}
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    // dims lighting based on distance (attenuation)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                                light.quadratic * (distance * distance));
    // spotlight intensity                            
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    // creates soft edges for spotlight
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine result
    vec3 ambient  = light.ambient;
    vec3 diffuse  = light.diffuse  * diff;
    vec3 specular = light.specular * spec;
    ambient *= intensity * attenuation;;
    diffuse *= intensity * attenuation;;
    specular *= intensity * attenuation;
    return (ambient + diffuse + specular);
}

// Phase 1: Directional lighting
// vec3 result = CalcDirLight(dirLight, norm, viewDir);  
// Phase 2: Point lights
// for (int i = 0; i < NR_POINT_LIGHTS; i++)
    // result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
// Phase 3: Spot light
// if (spotLight.FlashLightEnable == 1)
    // result += CalcSpotLight(spotLight, norm, FragPos, viewDir);