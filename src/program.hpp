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

class Program;

struct PthreadParams
{
    float start;
    float end;
    Program *program;
};

struct Scene
{
    glm::vec3 cameraPosition = {0, 1.f, -1.f};
    glm::vec3 sphereCenter = {0.5, 0.2, .5f};
    glm::vec3 sphereCenter2 = {-.5, 0.2, 1.f};
    glm::vec3 lightPosition = {0, 5, -5.2};
    float sphereRadius = 0.1;
    float sphere2Radius = 0.1;
};

class Program
{
    Scene scene;
    sf::RenderWindow *window;
    Canvas canvas;
    float phase = 0;
    sf::Text fps;
    sf::Font font;
    float incX = 2.f / SCREEN_WIDTH;
    float incY = 2.f / SCREEN_HEIGHT;
    float dt;

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

        pthtreadCount = 13; // fmin(MAX_PTHREAD_COUNT, (uint)sysconf(_SC_NPROCESSORS_ONLN) + 1);

        pthread_barrier_init(&barrierStart, NULL, pthtreadCount + 1);
        pthread_barrier_init(&barrierEnd, NULL, pthtreadCount + 1);

        float step = 2.0 / pthtreadCount;
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
        float phase = 0;
        while (1)
        {
            pthread_barrier_wait(&pParams->program->barrierStart);
            phase += pParams->program->dt * 1.0;
            pParams->program->scene.sphereCenter2.x = glm::sin(phase) * 0.2;
            pParams->program->scene.sphereCenter2.z = glm::cos(phase) * 0.2 + 1;

            pParams->program->scene.sphereCenter.x = glm::sin(phase + glm::pi<float>()) * 0.2;
            pParams->program->scene.sphereCenter.z = glm::cos(phase + glm::pi<float>()) * 0.2 + 1;

            for (float y = pParams->end; y > pParams->start; y -= pParams->program->incY)
            {
                for (float x = -1.0; x < 1.0; x += pParams->program->incX)
                {
                    float distance = pParams->program->rayMarch(pParams->program->scene.cameraPosition, glm::vec3(x, y, 1));
                    auto rayDirection = glm::normalize(glm::vec3(x, y, 1.0) - pParams->program->scene.cameraPosition);
                    auto impactPoint = pParams->program->scene.cameraPosition + rayDirection * distance;

                    auto lightComponent = glm::min<float>(glm::max<float>(pParams->program->getLight(impactPoint), 0), 255.f);

                    sf::Color color(lightComponent * 250, lightComponent * 250, lightComponent * 250);
                    pParams->program->canvas.setPixel((x * SCREEN_WIDTH + SCREEN_WIDTH) / 2,
                                                      (y * -SCREEN_HEIGHT + SCREEN_HEIGHT) / 2,
                                                      color);
                }
            }
            pthread_barrier_wait(&pParams->program->barrierEnd);
        }
        return NULL;
    }

    float getLight(glm::vec3 point)
    {
        auto normal = getNormal(point);
        auto lightRay = glm::normalize(scene.lightPosition - point);
        auto lightIntensity = glm::clamp(glm::dot(normal, lightRay), 0.f, 1.f);

        float distanceToLIght = rayMarch(point + normal * .1f, lightRay);

        if (distanceToLIght < glm::length(scene.lightPosition - point))
        {
            lightIntensity *= 0.5;
        }
        return lightIntensity;
    }

    glm::vec3 getNormal(glm::vec3 point)
    {
        auto dist = distance(point);
        static float e = 0.01;

        glm::vec3 normal = dist - glm::vec3(distance(point - glm::vec3(e, 0, 0)),
                                            distance(point - glm::vec3(0, e, 0)),
                                            distance(point - glm::vec3(0, 0, e)));

        return glm::normalize(normal);
    }

    void renderScene()
    {
        pthread_barrier_wait(&barrierStart);
        pthread_barrier_wait(&barrierEnd);
    }

    float rayMarch(glm::vec3 rayOrigin, glm::vec3 screenPosition)
    {
        // beyond this distance, we asume the ray didn't hit anything
        static float maxDistance = 100.0;
        static int maxSteps = 1000;
        auto rayDirection = glm::normalize(screenPosition - rayOrigin);
        float distanceToScene = 0;
        float distanceToOrigin = 0;

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

    float distance(glm::vec3 rayOrigin)
    {
        auto distanceToSphere = glm::length(rayOrigin - scene.sphereCenter) - scene.sphereRadius;
        auto distanceToSphere2 = glm::length(rayOrigin - scene.sphereCenter2) - scene.sphere2Radius;
        auto distanceToTorus = distanceTorus(rayOrigin - glm::vec3(0, .5f, 1.0), glm::vec2(.2f, .1f));
        return glm::min(glm::min(distanceToSphere2, glm::min(rayOrigin.y, distanceToSphere)), distanceToTorus);
    }

    float distanceTorus(glm::vec3 point, glm::vec2 r)
    {
        float x = glm::length(glm::vec2(point.x, point.z)) - r.x;
        return glm::length(glm::vec2(x, point.y + .4f)) - r.y;
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
