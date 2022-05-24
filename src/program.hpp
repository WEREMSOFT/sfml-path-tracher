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

class Program;

struct PthreadParams
{
    float start;
    float end;
    Program *program;
};

class Program
{
    sf::RenderWindow *window;
    glm::vec3 cameraPosition;
    Canvas canvas;
    glm::vec3 sphereCenter;
    glm::vec3 sphereCenter2;
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
    Program() : cameraPosition(0, 0.1, 0), sphereCenter(0.5, 0.25, 1), sphereCenter2(-0.5, 0.25, 1)
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

        pthtreadCount = fmin(MAX_PTHREAD_COUNT, (uint)sysconf(_SC_NPROCESSORS_ONLN) + 1);

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
            pParams->program->sphereCenter2.x = glm::sin(phase) * 0.5;
            pParams->program->sphereCenter2.z = glm::cos(phase) * 0.5 + 2;

            pParams->program->sphereCenter.x = glm::sin(phase + glm::pi<float>()) * 0.5;
            pParams->program->sphereCenter.z = glm::cos(phase + glm::pi<float>()) * 0.5 + 2;

            for (float y = pParams->end; y > pParams->start; y -= pParams->program->incY)
            {
                for (float x = -1.0; x < 1.0; x += pParams->program->incX)
                {
                    float distance = pParams->program->rayMarch(pParams->program->cameraPosition, glm::vec3(x, y, 1.0));
                    float adjustedDistance = glm::min<float>(distance * 100.f, 255.0);
                    auto rayDirection = glm::normalize(glm::vec3(x, y, 1.0) - pParams->program->cameraPosition);
                    auto impactPoint = pParams->program->cameraPosition + rayDirection * distance;
                    glm::vec3 normal = pParams->program->getNormal(impactPoint);
                    sf::Color color(
                        (normal.x < 0 ? 0 : normal.x) * 255.f,
                        (normal.y < 0 ? 0 : normal.y) * 255.f,
                        (normal.z < 0 ? 0 : normal.z) * 255.f,
                        255);

                    sf::Color depth(adjustedDistance, adjustedDistance, adjustedDistance, 255);
                    auto finalColor = depth;
                    if (x < 0)
                        finalColor = color;
                    pParams->program->canvas.setPixel((x * SCREEN_WIDTH + SCREEN_WIDTH) / 2, (y * -SCREEN_HEIGHT + SCREEN_HEIGHT) / 2, finalColor);
                }
            }
            pthread_barrier_wait(&pParams->program->barrierEnd);
        }
        return NULL;
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
        static float minDistance = 0.001;
        static int maxSteps = 100;
        auto rayDirection = glm::normalize(screenPosition - rayOrigin);
        float distanceToScene = 0;
        float distanceToOrigin = 0;

        for (int i = 0; i < maxSteps; i++)
        {
            rayOrigin = rayOrigin + rayDirection * distanceToScene;
            distanceToScene = distance(rayOrigin);
            distanceToOrigin += distanceToScene;
            if (distanceToScene < minDistance || distanceToOrigin > maxDistance)
                break;
        }

        return distanceToOrigin;
    }

    float distance(glm::vec3 rayOrigin)
    {
        float sphereRadius = .5f;

        auto distanceToSphere = glm::length(rayOrigin - sphereCenter) - sphereRadius;
        auto distanceToSphere2 = glm::length(rayOrigin - sphereCenter2) - sphereRadius;

        return glm::min(distanceToSphere2, glm::min(rayOrigin.y, distanceToSphere));
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
