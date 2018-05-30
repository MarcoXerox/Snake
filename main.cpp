#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Clock.hpp>
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>
#include <vector>
#include <cmath>

const sf::Vector2f UP(0, -1);
const sf::Vector2f LEFT(-1, 0);
const sf::Vector2f DOWN(0, 1);
const sf::Vector2f RIGHT(1, 0);

const unsigned int FRAMERATE_LIMIT = 10;
const unsigned int TIMER_WAIT_SECS = 3;

inline bool bounded(unsigned int lo, float t, unsigned int hi)
{
	return lo <= t and t <= hi;
}

sf::Vector2f direction(sf::Keyboard::Key code)
{
	switch (code)
	{
		case sf::Keyboard::W:
			return UP;
		case sf::Keyboard::A:
			return LEFT;
		case sf::Keyboard::S:
			return DOWN;
		case sf::Keyboard::D:
			return RIGHT;
		default:
			return sf::Vector2f();
	}
}

class Snake
{
	public:
		Snake
		(
			sf::Vector2u scr,
			float size,
			unsigned int length
		) :	head(size / 2.0f),
			dirs(length + 1, UP),
			body(length)
		{
			sf::Vector2f pos(scr.x / 2.0f, scr.y / 4.0f - head.getRadius());
			head.setPosition(pos);
			head.setFillColor(sf::Color::Yellow);
			for (auto& part : body)
			{
				pos += sf::Vector2f(0, size);
				part.setSize(sf::Vector2f(size, size));
				part.setFillColor(sf::Color::Green);
				part.setPosition(pos);
			}
		}
		bool isCollided(const sf::Shape& x) const
		{
			return head.getGlobalBounds().intersects(x.getGlobalBounds());
		}
		bool isAlive(sf::Vector2u scr) const
		{
			for (auto it = body.begin() + 1; it != body.end(); ++it)
			{
				if (isCollided(*it))
				{
					return false;
				}
			}
			sf::Vector2f pos = head.getPosition();
			return bounded(0, pos.x, scr.x) and bounded(0, pos.y, scr.y);
		}
		void draw(sf::RenderWindow* pwindow)
		{
			for (const auto& part : body)
			{
				pwindow->draw(part);
			}
			pwindow->draw(head);
		}
		void turn(const sf::Vector2f& dir)
		{
			dirs.back() = dir;
		}
		void move(void)
		{
			auto it = dirs.end();
			head.move(*(--it) * size());
			for (auto& part : body)
			{
				part.move(*(--it) * size());
			}
			dirs.push_back(dirs.back());
		}
		void extend(void)
		{
			sf::Vector2f pos = body.back().getPosition() - size() * dirs.end()[- 2 - body.size()];
			body.emplace_back(sf::Vector2f(size(), size()));
			body.back().setFillColor(sf::Color::Green);
			body.back().setPosition(pos);
		}
		float size(void) const
		{
			return head.getRadius() * 2.0f;
		}
		unsigned int length(void) const
		{
			return body.size();
		}
		
	private:
		sf::CircleShape head;
		std::vector<sf::Vector2f> dirs;
		std::vector<sf::RectangleShape> body;
};

class FoodCollection
{
	public:
		FoodCollection
		(
			sf::Vector2u scr,
			float size,
			unsigned int amount
		) :	foods(amount),
			gen(std::chrono::system_clock::now().time_since_epoch().count()),
			range(scr / static_cast<unsigned int>(size))
		{
			for (auto it = foods.begin(); it != foods.end(); ++it)
			{
				it->setSize(sf::Vector2f(size, size));
				it->setFillColor(sf::Color::Red);
				migrate(it, size);
			}
		}
		void migrate(std::vector<sf::RectangleShape>::iterator it, float size)
		{
			do
			{
				float x = size * static_cast<unsigned int>(dist(gen) * range.x);
				float y = size * static_cast<unsigned int>(dist(gen) * range.y);
				it->setPosition(x, y);
			} while (too_close(it, size * 4.0f));
		}
		bool too_close(std::vector<sf::RectangleShape>::iterator cur, float threshold)
		{
			for (auto it = foods.begin(); it != foods.end(); ++it)
			{
				if (it == cur) continue;
				sf::Vector2f diff = it->getPosition() - cur->getPosition();
				float norm = sqrt(diff.x * diff.x + diff.y * diff.y);
				if (norm <= threshold) return true;
			}
			return false;
		}
		bool isEaten(const Snake& snake)
		{
			for (auto it = foods.begin(); it != foods.end(); ++it)
			{
				if (snake.isCollided(*it))
				{
					migrate(it, snake.size());
					return true;
				}
			}
			return false;
		}
		void draw(sf::RenderWindow* pwindow)
		{
			for (const auto& food : foods)
			{
				pwindow->draw(food);
			}
		}
	private:
		std::vector<sf::RectangleShape> foods;
		std::default_random_engine gen;
		std::uniform_real_distribution<float> dist;
		sf::Vector2u range;
};

class Game
{
	public:
		Game
		(
			unsigned int width,
			unsigned int height,
			unsigned int snake_length,
			unsigned int food_qty,
			float size
		) :	window(sf::VideoMode(width, height), "Snake"),
			snake(window.getSize(), size, snake_length),
			foods(window.getSize(), size, food_qty),
			isPaused(true)
		{
			window.setFramerateLimit(FRAMERATE_LIMIT);
			if (!font.loadFromFile("Ubuntu-R.ttf"))
			{
				throw "FONT NOT FOUND";
			}
			text.setFont(font);
			text.setCharacterSize(16);
			text.setFillColor(sf::Color::White);
			text.setPosition(width * 0.60f, height * 0.01f);
		}
		int run(void)
		{
			while (window.isOpen() and snake.isAlive(window.getSize()))
			{
				sf::Event event;
				while (window.pollEvent(event))
				{
					switch (event.type)
					{
						case sf::Event::Closed:
							window.close();
							return 0;
						case sf::Event::KeyPressed:
							switch (event.key.code)
							{
								case sf::Keyboard::Space:
									isPaused ^= true;
									break;
								default:
									snake.turn(direction(event.key.code));
							}
							break;
						default:
							break;
					}
				}
				if (isPaused)
				{
					text.setString("Press [Space] to unpause Snake.");
				}
				else
				{
					std::stringstream ss;
					unsigned int secs = clock.getElapsedTime().asSeconds();
					ss << "Length: " << snake.length() << '\t' << "Time (sec): " << secs;
					text.setString(ss.str());
					snake.move();
				}
				if (foods.isEaten(snake))
				{
					snake.extend();
				}
				window.clear();
				window.draw(text);
				snake.draw(&window);
				foods.draw(&window);
				window.display();
			}
			clock.restart();
			while (clock.getElapsedTime().asSeconds() <= TIMER_WAIT_SECS);
			return 0;
		}
	private:
		sf::RenderWindow window;
		Snake snake;
		FoodCollection foods;
		bool isPaused;
		sf::Font font;
		sf::Text text;
		sf::Clock clock;
};

int main()
{
	try
	{
		Game game(800, 600, 15, 5, 20.0f);
		return game.run();
	}
	catch (const char* s)
	{
		std::cerr << "An error has occurred: " << s << std::endl;
		return -1;
	}
}
