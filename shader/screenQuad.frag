#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 pos;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

void main() {
    mat4 PVInv = inverse(ubo.proj * ubo.view);
    vec4 pointNDSH = vec4(UV * 2.0 - 1.0, -1.0, 1.0);
    vec3 dir = normalize((PVInv * pointNDSH).xyz);
    vec3 origin = ubo.pos;

    outColor = vec4(origin, 1.0);
    return;

    for (int i = 0; i < 30; ++i) {
        vec3 pos = origin + i * dir;
        ivec3 cell = ivec3(pos);

        if (cell.x % 10 == 0 || cell.y % 10 == 0 || cell.z % 10 == 0) {
            outColor = vec4(1);
            break;
        }
    }
}