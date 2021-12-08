//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <functional>
#include <shared_mutex>
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
    int m = 0;

    ThreadPool pool(THREAD_LIMIT);
    pool.init();

    //std::mutex mutex;
    std::shared_mutex done;
    int total = scene.height * scene.width * spp;
    int count = total;

    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            done.lock_shared();
            pool.submit([&, i, j, m] {
                // generate primary ray direction
                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                    imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));

                Vector3f color{ 0, 0,0 };
                for (int k = 0; k < spp; k++) {
                    color += scene.castRay(Ray(eye_pos, dir), 0) / spp;
                }
                framebuffer[m] = color;

                done.unlock_shared();
            });
            m++;
        }
        UpdateProgress(j / (float)scene.height);
    }
    done.lock();
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        constexpr int length = 3;
        unsigned char color[length]{
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f)),
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f)),
            (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f))
        };
        fwrite(color, sizeof(unsigned char), length, fp);
    }
    fclose(fp);
    pool.shutdown();
}
