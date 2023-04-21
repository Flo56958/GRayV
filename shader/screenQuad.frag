#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 pos;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

void main() {
    mat4 PInv = inverse(ubo.proj);
    mat4 VInv = inverse(ubo.view);

    float fovx = 3.1415 / 4;
    float fovy = 720/1280.0 * fovx;

    float x = 2 * (UV.x - 0.5) * tan(fovx);
    float y = 2 * (UV.y - 0.5) * tan(fovy);

    vec4 eye = vec4(x,y,-1,1);
    vec3 dir = normalize((VInv * eye).xyz);
    vec3 origin = ubo.pos;

    outColor = vec4(0);

    for (int i = 0; i < 250; ++i) {
        vec3 pos = origin + (i * dir) / 25;
        ivec3 cell = ivec3(pos);


        if (length(cell - ivec3(10, 0, 0)) < 3) {

            vec3 hit = cell - pos;
            vec3 n;
            if (abs(hit.x) > abs(hit.y) && abs(hit.x) > abs(hit.z)) {
                hit.x > 0 ? n = vec3(1,0,0) : n = vec3(-1,0,0);
            } else if (abs(hit.y) > abs(hit.x) && abs(hit.y) > abs(hit.z)) {
                hit.y > 0 ? n = vec3(0,1,0) : n = vec3(0,-1,0);
            } else if (abs(hit.z) > abs(hit.y) && abs(hit.z) > abs(hit.x)) {
                hit.z > 0 ? n = vec3(0,0,1) : n = vec3(0,0,1);
            }
            outColor = vec4(clamp(dot(n, vec3(1,1,0)), 0, 1)) + vec4(0.2);
            break;
        }
    }
}