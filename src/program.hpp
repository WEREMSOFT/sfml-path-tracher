#pragma once

#include <SFML/Graphics.hpp>

#include "constants.hpp"

class Program
{
    sf::RenderWindow *window;

public:
    Program()
    {
        window = new sf::RenderWindow(sf::VideoMode(
                                          SCREEN_WIDTH * WINDOW_RATIO, SCREEN_HEIGHT * WINDOW_RATIO),
                                      "Path Tracer!!");

        // window->setMouseCursorGrabbed(true);
        // window->setMouseCursorVisible(false);
    }

    void runMainLoop(void)
    {
        while (window->isOpen())
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

            window->clear(sf::Color::Red);
            window->display();
        }
    }
};