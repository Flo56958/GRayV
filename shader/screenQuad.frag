#version 450

#define M_PI 3.141592
#define MAX_SAMPLES 4
#define MAX_STEPS 200

layout(binding = 0) uniform UniformBufferObject {
	ivec3 screen;
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
	return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

bool getVoxel(ivec3 c) {
	vec3 p = vec3(c) + vec3(0.5);
	float d = min(max(-sdSphere(p, 3.5), sdBox(p, vec3(6.0))), -sdSphere(p, 25.0));
	return d < 0.0;
}

bool isWater(ivec3 c) {
	vec3 p = vec3(c) + vec3(0.5);
	float d = max(-sdSphere(p, 3.5), sdBox(p, vec3(6.0)));
	return d < 0.0;
}

uint pcg(uint v) {
	uint state = v * uint(747796405) + uint(2891336453);
	uint word = ((state >> ((state >> uint(28)) + uint(4))) ^ state) * uint(277803737);
	return (word >> uint(22)) ^ word;
}

float prng (float p) {
	return float(pcg(uint(p))) / float(uint(0xffffffff));
}

vec3 cosineSampleHemisphere(vec3 n, inout float seed)
{
	vec2 u = fract(sin(vec2(seed+=0.1,seed+=0.1)) * vec2(43758.5453123, 22578.1459123));
	float r = sqrt(u.x);
	float theta = 2.0 * M_PI * u.y;
	vec3  B = normalize( cross( n, vec3(0.0,1.0,1.0) ) );
	vec3  T = cross( B, n );
	return normalize(r * sin(theta) * B + sqrt(1.0 - u.x) * n + r * cos(theta) * T);
}

void restartDDA(ivec3 currentVoxel, inout vec3 rayPos, vec3 rayDir, vec3 newRayDir, bvec3 mask, inout vec3 deltaDist, inout ivec3 step, inout vec3 sideDist) {
	float d = 0.0f;
	vec3 dist = sideDist - deltaDist;
	if (mask.x) {
		d = dist.x;
	}
	if (mask.y) {
		d = dist.y;
	}
	if (mask.z) {
		d = dist.z;
	}
	rayPos = rayPos + rayDir * d + 0.01 * newRayDir;
	//length of ray from one x or y-side to next x or y-side
	deltaDist = abs(vec3(length(newRayDir)) / newRayDir);

	//length of ray from current position to next x or y-side
	step = ivec3(sign(newRayDir));
	sideDist = (step * (vec3(currentVoxel) - rayPos) + (step * 0.5f) + 0.5f) * deltaDist;
}

vec3 refractRay(vec3 rayDir, vec3 normal, float ior1, float ior2) {
	float frac = ior1 / ior2;
	float cos_theta = dot(-rayDir, normal);
	float sin_2_theta = frac * frac * (1 - cos_theta * cos_theta);

	if (ior1 > ior2) {
		if (asin(ior2 / ior1) <= acos(cos_theta)) { // total internal reflection
			return normalize(rayDir + 2 * (cos_theta + 0.1f * prng(cos_theta)) * normal);
		}
	}

	return normalize(frac * rayDir + (frac * cos_theta - sqrt(1 - sin_2_theta)) * normal);
}

vec3 mask2normal(vec3 rayDir, bvec3 mask) {
	vec3 normal = vec3(0.0f);
	if (mask.x) normal.x = 1;
	if (mask.y) normal.y = 1;
	if (mask.z) normal.z = 1;

	return normalize(-1 * sign(rayDir) * normal);
}

void main() {
	mat4 PInv = inverse(ubo.proj);
	mat4 VInv = inverse(ubo.view);
	int width = ubo.screen.x;
	int height = ubo.screen.y;

	vec2 shiftedUV = UV;
	float seed = length(ubo.view[3]) + ubo.screen.z / 1000.0f;
	outColor = vec4(0);

	for (int sampling = 0; sampling < MAX_SAMPLES; ++sampling) {
		shiftedUV = UV + (vec2(prng(shiftedUV.x + seed * sampling) - 0.5f) / width, (prng(shiftedUV.y + seed * sampling) - 0.5f) / height);
		vec4 dirEye = PInv * vec4(shiftedUV * 2.0f - 1.0f, -1.0f, 1.0f);
		dirEye.w = 0.;
		vec3 dirWorld = (VInv * dirEye).xyz;
		vec3 rayDir = normalize(dirWorld.xyz);
		vec3 rayPos = ubo.pos;
		ivec3 currentVoxel = ivec3(floor(rayPos + 0.0f));
		bvec3 mask = bvec3(false, false, false);
		vec3 deltaDist, sideDist;
		ivec3 step;

		restartDDA(currentVoxel, rayPos, vec3(0.0f), rayDir, mask, deltaDist, step, sideDist);

		vec3 throughput = vec3(1);

		const vec3 water_col = vec3(0.75f, 0.94f, 1.0f) * 0.9f;

		// perform DDA
		bool last_water = isWater(currentVoxel);
		int i = 0;
		int totalReflectionCount = 0;
		for (; i < MAX_STEPS; ++i) {
			bool water = isWater(currentVoxel);
			if (!water && getVoxel(currentVoxel)) {
				vec3 hit_n = mask2normal(rayDir, mask);
				if (hit_n.y != 0)
					break;
				
				float seed = fract(length(sideDist)) * ubo.screen.z;
				vec3 newRayDir = cosineSampleHemisphere(hit_n, seed);
				throughput *= dot(newRayDir, hit_n);
				restartDDA(currentVoxel, rayPos, rayDir, newRayDir, mask, deltaDist, step, sideDist);
			} else {
				if (!last_water && water) {
					vec3 newRayDir = refractRay(rayDir, mask2normal(rayDir, mask), 1.000293f, 1.333f);
					throughput *= 0.98;
					restartDDA(currentVoxel, rayPos, rayDir, newRayDir, mask, deltaDist, step, sideDist);
				} else if (last_water && !water) {
					vec3 newRayDir = refractRay(rayDir, mask2normal(rayDir, mask), 1.333f, 1.000293f);
					throughput *= 0.98;
					if (dot(newRayDir, rayDir) >= 0) ++totalReflectionCount;
					if (totalReflectionCount < 9)
						restartDDA(currentVoxel, rayPos, rayDir, newRayDir, mask, deltaDist, step, sideDist);
				}
			}

			last_water = water;

			if (sideDist.x < sideDist.y) {
				if (sideDist.x < sideDist.z) {
					sideDist.x += deltaDist.x;
					currentVoxel.x += step.x;
					mask = bvec3(true, false, false);
					//if (water) throughput *= water_col;
				}
				else {
					sideDist.z += deltaDist.z;
					currentVoxel.z += step.z;
					mask = bvec3(false, false, true);
					//if (water) throughput *= water_col;
				}
			}
			else {
				if (sideDist.y < sideDist.z) {
					sideDist.y += deltaDist.y;
					currentVoxel.y += step.y;
					mask = bvec3(false, true, false);
					//if (water) throughput *= water_col;
				}
				else {
					sideDist.z += deltaDist.z;
					currentVoxel.z += step.z;
					mask = bvec3(false, false, true);
					//if (water) throughput *= water_col;
				}
			}
		}
		if (i < MAX_STEPS) {
			outColor += vec4(throughput, 1.0f);
		}
	}

	outColor /= MAX_SAMPLES;
}