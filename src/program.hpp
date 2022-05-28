#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.hpp"
#include "canvas.hpp"
#include <glm/glm.hpp>
#include <threads.h>
#include <unistd.h>

#define MAX_PTHREAD_COUNT 50
#define SURFACE_MIN_DIST 0.01f

// In case we need more presicion
typedef glm::vec3 vec3;
typedef float fp_t;

class Program;

struct PthreadParams
{
    fp_t start;
    fp_t end;
    Program *program;
};

struct Scene
{
    vec3 cameraPosition = {0, 3.f, -1.f};
    vec3 sphereCenter = {0.5, 0.2, .5f};
    vec3 sphereCenter2 = {-.5, 0.2, 1.f};
    vec3 lightPosition = {0.f, 1.f, 0.f};
    fp_t sphereRadius = 0.1;
    fp_t sphere2Radius = 0.1;
    glm::vec2 torusRadius = {.2f, .1f};
};

class Program
{
    Scene scene;
    sf::RenderWindow *window;
    Canvas canvas;
    fp_t phase = 0;
    sf::Text fps;
    sf::Font font;
    fp_t incX = 2.f / SCREEN_WIDTH;
    fp_t incY = 2.f / SCREEN_HEIGHT;
    fp_t dt;

    uint pthtreadCount = 0;
    pthread_barrier_t barrierStart, barrierEnd;
    PthreadParams pthreadParams[MAX_PTHREAD_COUNT];
    pthread_t pthreadIds[MAX_PTHREAD_COUNT];

public:
    Program()
    {
        window = new sf::RenderWindow(sf::VideoMode(
                                          SCREEN_WIDTH * WINDOW_RATIO, SCREEN_HEIGHT * WINDOW_RATIO),
                                      "Path Tracer!!");

        // window->setFramerateLimit(60);
        font.loadFromFile("resources/JetBrainsMono-Regular.ttf");
        fps.setFont(font);
        fps.setFillColor(sf::Color(255, 0, 0));
        fps.setOutlineThickness(1.f);
        fps.setOutlineColor(sf::Color(0, 0, 0));
        fps.setCharacterSize(24);

        pthtreadCount = fmin(MAX_PTHREAD_COUNT, (uint)sysconf(_SC_NPROCESSORS_ONLN) + 3);

        pthread_barrier_init(&barrierStart, NULL, pthtreadCount + 1);
        pthread_barrier_init(&barrierEnd, NULL, pthtreadCount + 1);

        fp_t step = 2.0 / pthtreadCount;
        for (uint i = 0; i < pthtreadCount; i++)
        {
            pthreadParams[i].start = -1 + step * i;
            pthreadParams[i].end = -1 + step * (i + 1);
            pthreadParams[i].program = this;
            pthread_create(&pthreadIds[i], NULL, renderSlice, &pthreadParams[i]);
        }
    }

    void runMainLoop(void)
    {
        sf::Clock clock;
        char fpsString[500] = "";

        while (window->isOpen())
        {
            dt = clock.getElapsedTime().asSeconds();
            clock.restart();
            snprintf(fpsString, 500, "fps: %.2f\nFrameTime: %.12f", 1.f / dt, dt);
            fps.setString(sf::String(fpsString));
            checkExitConditions();

            renderScene();

            canvas.draw(window);
            window->draw(fps);
            window->display();
        }
    }

private:
    static void *renderSlice(void *params)
    {
        PthreadParams *pParams = (PthreadParams *)params;
        while (1)
        {
            pthread_barrier_wait(&pParams->program->barrierStart);
            for (fp_t y = pParams->end; y > pParams->start; y -= pParams->program->incY)
            {
                for (fp_t x = -1.0; x < 1.0; x += pParams->program->incX)
                {
                    auto distance = pParams->program->rayMarch(pParams->program->scene.cameraPosition, vec3(x, y, 1));
                    auto rayDirection = glm::normalize(vec3(x, y, 1.0) - pParams->program->scene.cameraPosition);
                    auto impactPoint = pParams->program->scene.cameraPosition + rayDirection * distance;

                    auto lightComponent = glm::min<fp_t>(glm::max<fp_t>(pParams->program->getLight(impactPoint), 0), 255.f);

                    sf::Color color(lightComponent * 255, lightComponent * 255, lightComponent * 255);
                    pParams->program->canvas.setPixel((x * SCREEN_WIDTH + SCREEN_WIDTH) / 2,
                                                      (y * -SCREEN_HEIGHT + SCREEN_HEIGHT) / 2,
                                                      color);
                }
            }
            pthread_barrier_wait(&pParams->program->barrierEnd);
        }
        return NULL;
    }

    fp_t getLight(vec3 point)
    {
        auto normal = getNormal(point);
        auto lightRay = glm::normalize(scene.lightPosition - point);
        fp_t lightIntensity = glm::clamp(glm::dot(normal, lightRay), 0.f, 1.0f);

        fp_t distanceToLIght = rayMarch(point + normal * .1f, scene.lightPosition);

        if (distanceToLIght < glm::length(scene.lightPosition - point))
        {
            lightIntensity *= 0.5;
        }
        return lightIntensity;
    }

    vec3 getNormal(vec3 point)
    {
        auto dist = distance(point);
        static fp_t e = 0.01;

        vec3 normal = dist - vec3(distance(point - vec3(e, 0, 0)),
                                  distance(point - vec3(0, e, 0)),
                                  distance(point - vec3(0, 0, e)));

        return glm::normalize(normal);
    }

    void renderScene()
    {
        static fp_t phase = 0.0;
        phase += dt * 1.0;
        scene.sphereCenter2.x = glm::sin(phase) * 0.2;
        scene.sphereCenter2.z = glm::cos(phase) * 0.2 + 1;

        scene.sphereCenter.x = glm::sin(phase + glm::pi<fp_t>()) * 0.2;
        scene.sphereCenter.z = glm::cos(phase + glm::pi<fp_t>()) * 0.2 + 1;

        scene.lightPosition.x = glm::cos(phase * 0.5 + glm::pi<fp_t>()) * 3.f;
        scene.lightPosition.z = glm::sin(phase * 0.5 + glm::pi<fp_t>()) * 3.f;

        scene.cameraPosition.y = 1.5 + glm::sin(phase * 0.2 + glm::pi<fp_t>());

        scene.torusRadius.x = 1.f + glm::sin(phase * 0.1 + glm::pi<fp_t>());

        pthread_barrier_wait(&barrierStart);
        pthread_barrier_wait(&barrierEnd);
    }

    fp_t rayMarch(vec3 rayOrigin, vec3 screenPosition)
    {
        // beyond this distance, we asume the ray didn't hit anything
        static fp_t maxDistance = 50.0;
        static int maxSteps = 30;
        auto rayDirection = glm::normalize(screenPosition - rayOrigin);
        fp_t distanceToScene = 0;
        fp_t distanceToOrigin = 0;

        for (int i = 0; i < maxSteps; i++)
        {
            rayOrigin = rayOrigin + rayDirection * distanceToScene;
            distanceToScene = distance(rayOrigin);
            distanceToOrigin += distanceToScene;
            if (distanceToScene < SURFACE_MIN_DIST || distanceToOrigin > maxDistance)
                break;
        }

        return distanceToOrigin;
    }

    fp_t distance(vec3 rayOrigin)
    {
        auto distanceToSphere = glm::length(rayOrigin - scene.sphereCenter) - scene.sphereRadius;
        auto distanceToSphere2 = glm::length(rayOrigin - scene.sphereCenter2) - scene.sphere2Radius;
        auto distanceToTorus = distanceTorus(rayOrigin - vec3(0, .5f, 1.0), scene.torusRadius);
        auto distanceToBox = distanceBox(rayOrigin - vec3(-.5f, .2f, 1.0), (vec3){.1f, .1f, .1f});
        auto mergedTorusAndBox = glm::mix(distanceToBox, distanceToSphere, 0.5);
        return smin(smin(smin(distanceToSphere2, smin(rayOrigin.y, distanceToSphere, .25), .25), distanceToTorus, .25), distanceToBox, .25);
        // return glm::min(glm::min(glm::min(distanceToSphere2, glm::min(rayOrigin.y, distanceToSphere)), distanceToTorus), distanceToBox);
        // return smin(smin(distanceToSphere, distanceToSphere2, 0.5), rayOrigin.y, .5);
    }

    fp_t smin(fp_t a, fp_t b, fp_t k)
    {
        auto h = glm::clamp(0.5 + 0.5 * (b - a) / k, 0., 1.);
        return glm::mix(b, a, h) - k * h * (1. - h);
    }

    fp_t distanceTorus(vec3 point, glm::vec2 r)
    {
        fp_t x = glm::length(glm::vec2(point.x, point.z)) - r.x;
        return glm::length(glm::vec2(x, point.y + .35f)) - r.y;
    }

    fp_t distanceBox(vec3 point, vec3 size)
    {
        vec3 vec = glm::max(glm::abs(point) - size, 0.f);
        return glm::length(vec);
    }

    void checkExitConditions(void)
    {
        static sf::Event evt;
        while (window->pollEvent(evt))
        {
            if (evt.type == sf::Event::Closed)
            {
                window->close();
                return;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
        {
            window->close();
            return;
        }
    }
};
