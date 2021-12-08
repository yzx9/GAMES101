//
// Created by Göksu Güvendiren on 2019-05-14.
//

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
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
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
    // TO DO Implement Path Tracing Algorithm here

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

        auto dirEyeToLight = hit.coords - ray.origin;
        lightDirect += hit.m->getEmission() / dotProduct(dirEyeToLight, dirEyeToLight) * 15000;
    }

    Intersection hitDirect;
    float pdfDirect;
    sampleLight(hitDirect, pdfDirect);

    auto dirDirect = (hitDirect.coords - hit.coords).normalized();
    auto disDirect = (hitDirect.coords - hit.coords).norm();
    auto hitShadow = intersect(Ray(hit.coords, dirDirect));
    if ((hitShadow.coords - hitDirect.coords).norm() < EPSILON) {
        lightDirect += hitDirect.emit
            * hit.m->eval(ray.direction, dirDirect, hit.normal)
            * dotProduct(dirDirect, hit.normal)
            * dotProduct(-dirDirect, hitDirect.normal)
            / (disDirect * disDirect)
            / pdfDirect;
    }
    
    if (get_random_float() > RussianRoulette) {
        return lightDirect;
    }

    // Indirect Light
    Vector3f lightIndirect{ 0, 0, 0 };
    auto dirIndirect = hit.m->sample(ray.direction, hit.normal);
    Ray rayIndirect(hit.coords, dirIndirect);
    lightIndirect += castRay(rayIndirect, depth + 1)
        * hit.m->eval(ray.direction, dirIndirect, hit.normal)
        * dotProduct(ray.direction, hit.normal)
        / hit.m->pdf(ray.direction, dirIndirect, hit.normal)
        / RussianRoulette;

    return lightDirect + lightIndirect;
}