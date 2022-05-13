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

#define PTHREAD_COUNT 17

class Program;

struct PthreadParams
{
    float dt;
    float start;
    float end;
    float incY;
    float incX;
    glm::vec3 sphereCenter;
    glm::vec3 sphereCenter2;
    glm::vec3 cameraPosition;
    pthread_barrier_t *barrierStart, *barrierEnd;
    Canvas *canvas;
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

    pthread_barrier_t barrierStart, barrierEnd;
    PthreadParams pthreadParams[PTHREAD_COUNT];
    pthread_t pthreadIds[PTHREAD_COUNT];

public:
    Program() : cameraPosition(0, 0.5, 0), sphereCenter(0.5, 0.25, 2), sphereCenter2(-0.5, 0.25, 2)
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

        pthread_barrier_init(&barrierStart, NULL, PTHREAD_COUNT + 1);
        pthread_barrier_init(&barrierEnd, NULL, PTHREAD_COUNT + 1);

        float step = 2.0 / PTHREAD_COUNT;
        for (int i = 0; i < PTHREAD_COUNT; i++)
        {
            pthreadParams[i].start = -1 + step * i;
            pthreadParams[i].end = -1 + step * (i + 1);
            pthreadParams[i].sphereCenter = sphereCenter;
            pthreadParams[i].sphereCenter2 = sphereCenter2;
            pthreadParams[i].incY = incY;
            pthreadParams[i].incX = incX;
            pthreadParams[i].barrierStart = &barrierStart;
            pthreadParams[i].barrierEnd = &barrierEnd;
            pthreadParams[i].cameraPosition = cameraPosition;
            pthreadParams[i].canvas = &canvas;
            pthread_create(&pthreadIds[i], NULL, renderSlice, &pthreadParams[i]);
        }
    }

    void runMainLoop(void)
    {
        sf::Clock clock;
        char fpsString[500] = "";
        while (window->isOpen())
        {
            float dt = clock.getElapsedTime().asSeconds();
            snprintf(fpsString, 500, "fps: %.2f", 1.f / dt);
            fps.setString(sf::String(fpsString));
            clock.restart();
            checkExitConditions();
            renderScene(dt);

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
            pthread_barrier_wait(pParams->barrierStart);

            phase += pParams->dt * 5.0;
            pParams->sphereCenter2.x = glm::sin(phase) * 0.5;
            pParams->sphereCenter2.z = glm::cos(phase) * 0.5 + 2;

            pParams->sphereCenter.x = glm::sin(phase + glm::pi<float>()) * 0.5;
            pParams->sphereCenter.z = glm::cos(phase + glm::pi<float>()) * 0.5 + 2;

            for (float y = pParams->end; y > pParams->start; y -= pParams->incY)
            {
                for (float x = -1.0; x < 1.0; x += pParams->incX)
                {
                    float distance = glm::min(255.0, rayMarch(pParams->cameraPosition, glm::vec3(x, y, 1.0), pParams) * 100.0);
                    pParams->canvas->setPixel((x * SCREEN_WIDTH + SCREEN_WIDTH) / 2, (y * -SCREEN_HEIGHT + SCREEN_HEIGHT) / 2, sf::Color(distance, distance, distance, 255));
                }
            }
            pthread_barrier_wait(pParams->barrierEnd);
        }
        return NULL;
    }

    void renderScene(float deltaTime)
    {
        for (int i = 0; i < PTHREAD_COUNT; i++)
        {
            pthreadParams[i].dt = deltaTime;
        }

        pthread_barrier_wait(&barrierStart);
        pthread_barrier_wait(&barrierEnd);
    }

    static float rayMarch(glm::vec3 rayOrigin, glm::vec3 screenPosition, PthreadParams *params)
    {
        // beyond this distance, we asume the ray didn't hit anything
        static float maxDistance = 100.0;
        static float minDistance = 0.01;
        static int maxSteps = 100;
        auto rayDirection = glm::normalize(screenPosition - rayOrigin);
        float distanceToScene = 0;
        float distanceToOrigin = 0;
        for (int i = 0; i < maxSteps; i++)
        {
            rayOrigin = rayOrigin + rayDirection * distanceToScene;
            distanceToScene = distance(rayOrigin, params);
            distanceToOrigin += distanceToScene;
            if (distanceToScene < minDistance || distanceToOrigin > maxDistance)
                break;
        }
        return distanceToOrigin;
    }

    static float distance(glm::vec3 rayOrigin, PthreadParams *params)
    {

        float sphereRadius = .50f;

        auto distanceToSphere = glm::length(rayOrigin - params->sphereCenter) - sphereRadius;
        auto distanceToSphere2 = glm::length(rayOrigin - params->sphereCenter2) - sphereRadius;

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
