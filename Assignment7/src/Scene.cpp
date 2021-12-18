//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include <limits>
#include "Scene.hpp"

void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
    const Ray& ray,
    const std::vector<Object*>& objects,
    float& tNear, uint32_t& index, Object** hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    auto hit = intersect(ray);
    if (!hit.happened) {
        return { 0, 0, 0 };
    }

    // Direct Light
    Vector3f lightDirect{ 0, 0, 0 };
    if (hit.m->hasEmission()) {
        if (depth != 0) {
            return lightDirect;
        }

        lightDirect += hit.m->getEmission();
    }

    Intersection hitDirect;
    float pdfDirect;
    sampleLight(hitDirect, pdfDirect);
    
    auto vecDirect = hitDirect.coords - hit.coords;
    auto dirDirect = vecDirect.normalized();
    auto disDirect = vecDirect.norm();
    auto disShadow = intersect(Ray(hit.coords, dirDirect)).distance;
    if ((disDirect - disShadow) < std::numeric_limits<float>::epsilon()) {
        lightDirect += hitDirect.emit
            * hit.m->eval(ray.direction, dirDirect, hit.normal)
            * dotProduct(dirDirect, hit.normal)
            * dotProduct(-dirDirect, hitDirect.normal)
            * (1 / disDirect / disDirect)
            * (1 / pdfDirect);
    }
    
    if (get_random_float() > RussianRoulette) {
        return lightDirect;
    }

    // Indirect Light
    auto dirIndirect = hit.m->sample(ray.direction, hit.normal).normalized();
    auto lightIndirect = castRay(Ray(hit.coords, dirIndirect), depth + 1)
        * hit.m->eval(ray.direction, dirIndirect, hit.normal)
        * dotProduct(dirIndirect, hit.normal)
        * (1 / RussianRoulette)
        * (1 / hit.m->pdf(ray.direction, dirIndirect, hit.normal));

    return lightDirect + lightIndirect;
}