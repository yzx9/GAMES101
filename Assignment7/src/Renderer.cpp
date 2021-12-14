//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <shared_mutex>
#include <chrono>
#include <thread>
#include "Scene.hpp"
#include "Renderer.hpp"
#include "ThreadPool.hpp"

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

constexpr int THREAD_LIMIT = 16;

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

    ThreadPool pool(THREAD_LIMIT);
    pool.init();

    //std::mutex mutex;
    std::shared_mutex done;

    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            done.lock_shared();
            pool.submit([&, i, j] {
                // generate primary ray direction
                float x = (2 * (i + 0.5) / float(scene.width) - 1) * scale * imageAspectRatio;
                float y = (1 - 2 * (j + 0.5) / float(scene.height)) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                Vector3f color{ 0, 0, 0 };
                for (int k = 0; k < spp; k++) {
                    color += scene.castRay(Ray(eye_pos, dir), 0) / spp;
                }
                framebuffer[j * scene.width + i] = color;

                done.unlock_shared();
            });
        }
    }
    
    constexpr float TRACE_STAGE_PROGRESS = 0.95;
    constexpr float STORAGE_STAGE_PROGRESS = 1 - TRACE_STAGE_PROGRESS;
    int total = scene.height * scene.width;
    while (!done.try_lock()) {
        UpdateProgress(TRACE_STAGE_PROGRESS * (total - pool.count() - THREAD_LIMIT) / total);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    UpdateProgress(TRACE_STAGE_PROGRESS);

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

        if ((i & 0x0400) == 0) {
            UpdateProgress(TRACE_STAGE_PROGRESS + STORAGE_STAGE_PROGRESS * static_cast<float>(i) / total);
        }
    }
    fclose(fp);
    pool.shutdown();
    UpdateProgress(1.f);
}
