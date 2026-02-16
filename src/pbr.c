#include "../include/engine.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265359f

void pbr_init_material(PBRMaterial* mat) {
    mat->albedo = (ColorF){1.0f, 1.0f, 1.0f, 1.0f};
    mat->metallic = 0.0f;
    mat->roughness = 0.5f;
    mat->ao = 1.0f;
    mat->emissive_strength = 0.0f;
    mat->albedo_map = -1;
    mat->normal_map = -1;
    mat->metallic_map = -1;
    mat->roughness_map = -1;
    mat->ao_map = -1;
    mat->emissive_map = -1;
}

// Trowbridge-Reitz GGX normal distribution
float pbr_distribution_ggx(Vec3 normal, Vec3 halfway, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float ndot_h = fmaxf(vec3_dot(normal, halfway), 0.0f);
    float ndot_h2 = ndot_h * ndot_h;
    
    float num = a2;
    float denom = ndot_h2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    
    return num / denom;
}

// Smith's Schlick-GGX geometry function
static float geometry_schlick_ggx(float ndot_v, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    
    float num = ndot_v;
    float denom = ndot_v * (1.0f - k) + k;
    
    return num / denom;
}

float pbr_geometry_smith(Vec3 normal, Vec3 view_dir, Vec3 light_dir, float roughness) {
    float ndot_v = fmaxf(vec3_dot(normal, view_dir), 0.0f);
    float ndot_l = fmaxf(vec3_dot(normal, light_dir), 0.0f);
    float ggx2 = geometry_schlick_ggx(ndot_v, roughness);
    float ggx1 = geometry_schlick_ggx(ndot_l, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
Vec3 pbr_fresnel_schlick(float cos_theta, Vec3 f0) {
    float factor = powf(1.0f - cos_theta, 5.0f);
    return (Vec3){
        f0.x + (1.0f - f0.x) * factor,
        f0.y + (1.0f - f0.y) * factor,
        f0.z + (1.0f - f0.z) * factor
    };
}

// Cook-Torrance BRDF
ColorF pbr_calculate_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                              Vec3 light_dir, ColorF light_color) {
    Vec3 halfway = vec3_normalize(vec3_add(view_dir, light_dir));
    
    // Calculate reflectance at normal incidence
    Vec3 f0 = {0.04f, 0.04f, 0.04f};
    if (mat->metallic > 0.0f) {
        f0.x = mat->albedo.r * mat->metallic + f0.x * (1.0f - mat->metallic);
        f0.y = mat->albedo.g * mat->metallic + f0.y * (1.0f - mat->metallic);
        f0.z = mat->albedo.b * mat->metallic + f0.z * (1.0f - mat->metallic);
    }
    
    // Cook-Torrance BRDF
    float ndf = pbr_distribution_ggx(normal, halfway, mat->roughness);
    float g = pbr_geometry_smith(normal, view_dir, light_dir, mat->roughness);
    Vec3 f = pbr_fresnel_schlick(fmaxf(vec3_dot(halfway, view_dir), 0.0f), f0);
    
    Vec3 k_s = f;
    Vec3 k_d = {
        (1.0f - k_s.x) * (1.0f - mat->metallic),
        (1.0f - k_s.y) * (1.0f - mat->metallic),
        (1.0f - k_s.z) * (1.0f - mat->metallic)
    };
    
    float ndot_l = fmaxf(vec3_dot(normal, light_dir), 0.0f);
    float ndot_v = fmaxf(vec3_dot(normal, view_dir), 0.0f);
    
    // Specular
    Vec3 numerator = vec3_mul(f, ndf * g);
    float denominator = 4.0f * ndot_v * ndot_l + 0.0001f;
    Vec3 specular = vec3_mul(numerator, 1.0f / denominator);
    
    // Combine
    ColorF result;
    result.r = (k_d.x * mat->albedo.r / PI + specular.x) * light_color.r * ndot_l;
    result.g = (k_d.y * mat->albedo.g / PI + specular.y) * light_color.g * ndot_l;
    result.b = (k_d.z * mat->albedo.b / PI + specular.z) * light_color.b * ndot_l;
    result.a = 1.0f;
    
    // Apply ambient occlusion
    result.r *= mat->ao;
    result.g *= mat->ao;
    result.b *= mat->ao;
    
    // Add emissive
    result.r += mat->albedo.r * mat->emissive_strength;
    result.g += mat->albedo.g * mat->emissive_strength;
    result.b += mat->albedo.b * mat->emissive_strength;
    
    return result;
}

// Image-based lighting (simplified)
ColorF pbr_image_based_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                                IrradianceProbe* probe) {
    if (!probe) {
        return (ColorF){0.0f, 0.0f, 0.0f, 1.0f};
    }
    
    // Sample irradiance from probe based on normal direction
    // Simplified: use dominant direction
    int dominant_dir = 0;
    float max_component = fabsf(normal.x);
    
    if (fabsf(normal.y) > max_component) {
        dominant_dir = fabsf(normal.y) > 0 ? 2 : 3;
        max_component = fabsf(normal.y);
    }
    if (fabsf(normal.z) > max_component) {
        dominant_dir = fabsf(normal.z) > 0 ? 4 : 5;
    }
    
    ColorF irradiance = probe->irradiance[dominant_dir];
    
    // Diffuse IBL
    Vec3 f0 = {0.04f, 0.04f, 0.04f};
    if (mat->metallic > 0.0f) {
        f0.x = mat->albedo.r * mat->metallic + f0.x * (1.0f - mat->metallic);
        f0.y = mat->albedo.g * mat->metallic + f0.y * (1.0f - mat->metallic);
        f0.z = mat->albedo.b * mat->metallic + f0.z * (1.0f - mat->metallic);
    }
    
    Vec3 k_s = pbr_fresnel_schlick(fmaxf(vec3_dot(normal, view_dir), 0.0f), f0);
    Vec3 k_d = {
        (1.0f - k_s.x) * (1.0f - mat->metallic),
        (1.0f - k_s.y) * (1.0f - mat->metallic),
        (1.0f - k_s.z) * (1.0f - mat->metallic)
    };
    
    ColorF diffuse;
    diffuse.r = k_d.x * mat->albedo.r * irradiance.r;
    diffuse.g = k_d.y * mat->albedo.g * irradiance.g;
    diffuse.b = k_d.z * mat->albedo.b * irradiance.b;
    diffuse.a = 1.0f;
    
    // Apply AO
    diffuse.r *= mat->ao;
    diffuse.g *= mat->ao;
    diffuse.b *= mat->ao;
    
    return diffuse;
}
