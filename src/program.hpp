#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>

#include <stdio.h>
#include <stdlib.h>

#include "constants.hpp"
#include "canvas.hpp"
#include <glm/glm.hpp>

class Program
{
    sf::RenderWindow *window;
    glm::vec3 cameraPosition;
    Canvas canvas;
    // beyond this distance, we asume the ray didn't hit anything
    float maxDistance = 100.0;
    float minDistance = 0.01;
    int maxSteps = 100;
    glm::vec3 sphereCenter;
    glm::vec3 sphereCenter2;
    float phase = 0;

public:
    Program() : cameraPosition(0, 0.5, 0), sphereCenter(0.5, 0.25, 2), sphereCenter2(-0.5, 0.25, 2)
    {
        window = new sf::RenderWindow(sf::VideoMode(
                                          SCREEN_WIDTH * WINDOW_RATIO, SCREEN_HEIGHT * WINDOW_RATIO),
                                      "Path Tracer!!");

        window->setFramerateLimit(60);
    }

    void runMainLoop(void)
    {
        sf::Clock clock;
        while (window->isOpen())
        {
            float dt = clock.getElapsedTime().asSeconds();
            clock.restart();
            checkExitConditions();
            renderScene(dt);

            canvas.draw(window);
            window->display();
        }
    }

private:
    void renderScene(float deltaTime)
    {
        float incX = 2.f / SCREEN_WIDTH;
        float incY = 2.f / SCREEN_HEIGHT;

        phase += deltaTime * 5.0;
        sphereCenter2.x = glm::sin(phase) * 0.5;
        sphereCenter2.z = glm::cos(phase) * 0.5 + 2;

        sphereCenter.x = glm::sin(phase + glm::pi<float>()) * 0.5;
        sphereCenter.z = glm::cos(phase + glm::pi<float>()) * 0.5 + 2;
        for (float y = 1.0; y > -1.0; y -= incY)
        {
            for (float x = -1.0; x < 1.0; x += incX)
            {
                float distance = glm::min(255.0, rayMarch(cameraPosition, glm::vec3(x, y, 1.0)) * 100.0);
                canvas.setPixel((x * SCREEN_WIDTH + SCREEN_WIDTH) / 2, (y * -SCREEN_HEIGHT + SCREEN_HEIGHT) / 2, sf::Color(distance, distance, distance, 255));
            }
        }
    }

    float rayMarch(glm::vec3 rayOrigin, glm::vec3 screenPosition)
    {
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

        float sphereRadius = .50f;

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
