#version 450

// https://www.shadertoy.com/view/4dX3zl

layout(binding = 0) uniform UniformBufferObject {
    vec3 pos;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

float sdSphere(vec3 p, float d) { return length(p) - d; } 

float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0));
}

bool getVoxel(ivec3 c) {
	vec3 p = vec3(c) + vec3(0.5);
	float d = min(max(-sdSphere(p, 7.5), sdBox(p, vec3(6.0))), -sdSphere(p, 25.0));
	return d < 0.0;
}

void main() {
    mat4 PInv = inverse(ubo.proj);
    mat4 VInv = inverse(ubo.view);

	vec2 pxNDS = UV*2. - 1.;
	vec3 pointNDS = vec3(pxNDS, -1.);
	vec4 pointNDSH = vec4(pointNDS, 1.0);
	vec4 dirEye = PInv * pointNDSH;
	dirEye.w = 0.;
	vec3 dirWorld = (VInv * dirEye).xyz;
    vec3 rayDir = normalize(dirWorld.xyz);
    vec3 rayPos = ubo.pos;

    ivec3 mapPos = ivec3(floor(rayPos + 0.));

	vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
	
	ivec3 rayStep = ivec3(sign(rayDir));

	vec3 sideDist = (sign(rayDir) * (vec3(mapPos) - rayPos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist; 

    outColor = vec4(0);

    bvec3 mask;

    for (int i = 0; i < 250; i++) {
        if (getVoxel(mapPos)) continue;
		if (sideDist.x < sideDist.y) {
			if (sideDist.x < sideDist.z) {
				sideDist.x += deltaDist.x;
				mapPos.x += rayStep.x;
				mask = bvec3(true, false, false);
			}
			else {
				sideDist.z += deltaDist.z;
				mapPos.z += rayStep.z;
				mask = bvec3(false, false, true);
			}
		}
		else {
			if (sideDist.y < sideDist.z) {
				sideDist.y += deltaDist.y;
				mapPos.y += rayStep.y;
				mask = bvec3(false, true, false);
			}
			else {
				sideDist.z += deltaDist.z;
				mapPos.z += rayStep.z;
				mask = bvec3(false, false, true);
			}
		}
    }
    
    vec3 color;
	if (mask.x) {
		color = vec3(0.5);
	}
	if (mask.y) {
		color = vec3(1.0);
	}
	if (mask.z) {
		color = vec3(0.75);
	}

    outColor = vec4(color, 1);
}