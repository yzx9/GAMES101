//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include <thread>
#include <future>
#include "Scene.hpp"
#include "Renderer.hpp"

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    // change the spp value to change sample ammount
    constexpr int spp = 16;
    std::cout << "SPP: " << spp << "\n";

    std::vector<Vector3f> framebuffer(scene.width * scene.height);
    Vector3f eye_pos(278, 273, -800);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;

    int total = scene.height * scene.width;
    std::atomic_int count = total;

    auto castRay = [&](int i, int j) {
        // generate primary ray direction
        auto x = static_cast<float>((2 * (i + 0.5) / static_cast<float>(scene.width) - 1) * scale * imageAspectRatio);
        auto y = static_cast<float>((1 - 2 * (j + 0.5) / static_cast<float>(scene.height)) * scale);

        Vector3f dir = normalize(Vector3f(-x, y, 1));
        Vector3f color{ 0, 0, 0 };
        for (int k = 0; k < spp; k++) {
            color += scene.castRay(Ray(eye_pos, dir), 0) / spp;
        }
        framebuffer[j * scene.width + i] = color;
        count--;
    };

    std::vector<std::future<void>> futures;
    for (int j = 0; j < scene.height; ++j) {
        for (int i = 0; i < scene.width; ++i) {
            futures.emplace_back(std::async(std::launch::async, castRay, i, j));
        }
    }
    
    while (count != 0) {
        UpdateProgress(static_cast<float>(total - count) / total);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < total; ++i) {
        constexpr int length = 3;
        unsigned char color[length]{
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f)),
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f)),
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f))
        };
        fwrite(color, sizeof(unsigned char), length, fp);
    }
    fclose(fp);
    UpdateProgress(1.f);
}
