#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include "constants.hpp"
#include <stdio.h>

class Canvas
{
    sf::Sprite canvasS;
    sf::Image canvasI;
    sf::Texture canvasT;

public:
    Canvas()
    {
        canvasI.create(SCREEN_WIDTH, SCREEN_HEIGHT, sf::Color(0, 0, 0));
        canvasT.loadFromImage(canvasI);
        canvasS.setTexture(canvasT);
        canvasS.setScale(WINDOW_RATIO, WINDOW_RATIO);
    }

    ~Canvas()
    {
        printf("destroying canvas\n");
    }

    void setPixel(int x, int y, sf::Color color)
    {
        x = glm::min(glm::max(0, x), SCREEN_WIDTH - 1);
        y = glm::min(glm::max(0, y), SCREEN_HEIGHT - 1);
        canvasI.setPixel(x, y, color);
    }

    void draw(sf::RenderWindow *window)
    {
        canvasT.loadFromImage(canvasI);
        window->draw(canvasS);
    }
};