
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <random>
#include <ctime>
#include <sstream>

using namespace std;

const int CELL_SIZE = 30;
enum Difficulty { EASY = 15, MEDIUM = 20, HARD = 25 };
enum GameState { MENU, DIFFICULTY_SELECT, PLAYING };

class Cell {
public:
    bool visited = false;
    bool walls[4] = { true, true, true, true }; // Top, Right, Bottom, Left
    bool isPath = false; // For marking cells in the hint path
};

class Button {
    sf::RectangleShape shape;
    sf::Text text;
public:
    Button(const std::string& str, const sf::Font& font, float x, float y) {
        shape.setSize(sf::Vector2f(200, 50));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(100, 100, 100));

        text.setString(str);
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);

        text.setPosition(
            x + (200 - text.getLocalBounds().width) / 2,
            y + (50 - text.getLocalBounds().height) / 2
        );
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        window.draw(text);
    }

    bool contains(sf::Vector2i point) const {
        return shape.getGlobalBounds().contains(point.x, point.y);
    }
};

class Game {
private:
    sf::RenderWindow window;
    sf::Font font;
    GameState state = MENU;
    int mazeSize = EASY;

    std::vector<std::vector<Cell>> maze;
    sf::Vector2i playerPos{ 0, 0 };
    sf::Clock gameTimer;
    bool showHint = false;
    sf::Clock hintTimer;

    Button hintButton;
    std::vector<sf::Vector2i> hintPath;

    std::vector<Button> menuButtons;
    std::vector<Button> difficultyButtons;

    void generateMaze() {
        // Initialize maze with all walls
        maze.clear();
        maze.resize(mazeSize, std::vector<Cell>(mazeSize));

        // Initialize RNG
        std::random_device rd;
        std::mt19937 gen(rd());

        std::stack<sf::Vector2i> stack;
        sf::Vector2i current(0, 0);
        maze[current.y][current.x].visited = true;
        stack.push(current);

        while (!stack.empty()) {
            current = stack.top();
            stack.pop();

            std::vector<int> directions;
            // Check all four directions (Top, Right, Bottom, Left)
            if (current.y > 0 && !maze[current.y - 1][current.x].visited)           // Top
                directions.push_back(0);
            if (current.x < mazeSize - 1 && !maze[current.y][current.x + 1].visited) // Right
                directions.push_back(1);
            if (current.y < mazeSize - 1 && !maze[current.y + 1][current.x].visited) // Bottom
                directions.push_back(2);
            if (current.x > 0 && !maze[current.y][current.x - 1].visited)           // Left
                directions.push_back(3);

            if (!directions.empty()) {
                stack.push(current);

                // Choose random direction
                std::uniform_int_distribution<> dis(0, directions.size() - 1);
                int nextDir = directions[dis(gen)];

                // Remove walls between current cell and chosen neighbor
                switch (nextDir) {
                case 0: // Top
                    maze[current.y][current.x].walls[0] = false;
                    maze[current.y - 1][current.x].walls[2] = false;
                    maze[current.y - 1][current.x].visited = true;
                    stack.push(sf::Vector2i(current.x, current.y - 1));
                    break;
                case 1: // Right
                    maze[current.y][current.x].walls[1] = false;
                    maze[current.y][current.x + 1].walls[3] = false;
                    maze[current.y][current.x + 1].visited = true;
                    stack.push(sf::Vector2i(current.x + 1, current.y));
                    break;
                case 2: // Bottom
                    maze[current.y][current.x].walls[2] = false;
                    maze[current.y + 1][current.x].walls[0] = false;
                    maze[current.y + 1][current.x].visited = true;
                    stack.push(sf::Vector2i(current.x, current.y + 1));
                    break;
                case 3: // Left
                    maze[current.y][current.x].walls[3] = false;
                    maze[current.y][current.x - 1].walls[1] = false;
                    maze[current.y][current.x - 1].visited = true;
                    stack.push(sf::Vector2i(current.x - 1, current.y));
                    break;
                }
            }
        }

        // Reset visited flags for gameplay
        for (auto& row : maze) {
            for (auto& cell : row) {
                cell.visited = false;
            }
        }
    }

    std::vector<sf::Vector2i> findPath() {
        std::vector<std::vector<sf::Vector2i>> parent(mazeSize, std::vector<sf::Vector2i>(mazeSize, sf::Vector2i(-1, -1)));
        std::vector<std::vector<bool>> visited(mazeSize, std::vector<bool>(mazeSize, false));
        std::queue<sf::Vector2i> q;

        q.push(playerPos);
        visited[playerPos.y][playerPos.x] = true;

        sf::Vector2i goal(mazeSize - 1, mazeSize - 1);
        bool foundPath = false;

        while (!q.empty() && !foundPath) {
            sf::Vector2i current = q.front();
            q.pop();

            if (current == goal) {
                foundPath = true;
                break;
            }

            // Check all four directions
            for (int dir = 0; dir < 4; ++dir) {
                if (!maze[current.y][current.x].walls[dir]) {
                    sf::Vector2i next = current;
                    switch (dir) {
                    case 0: next.y--; break; // Top
                    case 1: next.x++; break; // Right
                    case 2: next.y++; break; // Bottom
                    case 3: next.x--; break; // Left
                    }

                    if (next.x >= 0 && next.x < mazeSize &&
                        next.y >= 0 && next.y < mazeSize &&
                        !visited[next.y][next.x]) {
                        q.push(next);
                        visited[next.y][next.x] = true;
                        parent[next.y][next.x] = current;
                    }
                }
            }
        }

        std::vector<sf::Vector2i> path;
        if (foundPath) {
            sf::Vector2i current = goal;
            while (current != playerPos) {
                path.push_back(current);
                current = parent[current.y][current.x];
            }
            std::reverse(path.begin(), path.end());
        }
        return path;
    }

public:
    Game() :
        window(sf::VideoMode(800, 600), "Maze Game"),
        hintButton("Hint", font, 10, 70)
    {
        if (!font.loadFromFile("C:\\Users\\Hp\\Desktop\\arial\\arial.ttf")) {
            throw std::runtime_error("Could not load font");
        }

        menuButtons.emplace_back("Single Player", font, 300, 200);
        menuButtons.emplace_back("Multiplayer", font, 300, 300);

        difficultyButtons.emplace_back("Easy", font, 300, 150);
        difficultyButtons.emplace_back("Medium", font, 300, 250);
        difficultyButtons.emplace_back("Hard", font, 300, 350);
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            draw();
        }
    }

private:
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                if (state == PLAYING && hintButton.contains(mousePos)) {
                    showHint = true;
                    hintTimer.restart();
                    hintPath = findPath();
                }

                if (state == MENU) {
                    for (const auto& button : menuButtons) {
                        if (button.contains(mousePos)) {
                            state = DIFFICULTY_SELECT;
                        }
                    }
                }
                else if (state == DIFFICULTY_SELECT) {
                    for (size_t i = 0; i < difficultyButtons.size(); i++) {
                        if (difficultyButtons[i].contains(mousePos)) {
                            mazeSize = (i == 0) ? EASY : (i == 1) ? MEDIUM : HARD;
                            state = PLAYING;
                            generateMaze();
                            playerPos = sf::Vector2i(0, 0);
                            gameTimer.restart();
                            hintPath.clear();
                        }
                    }
                }
            }

            if (state == PLAYING && event.type == sf::Event::KeyPressed) {
                sf::Vector2i newPos = playerPos;
                switch (event.key.code) {
                case sf::Keyboard::Up:
                    if (!maze[playerPos.y][playerPos.x].walls[0]) newPos.y--;
                    break;
                case sf::Keyboard::Right:
                    if (!maze[playerPos.y][playerPos.x].walls[1]) newPos.x++;
                    break;
                case sf::Keyboard::Down:
                    if (!maze[playerPos.y][playerPos.x].walls[2]) newPos.y++;
                    break;
                case sf::Keyboard::Left:
                    if (!maze[playerPos.y][playerPos.x].walls[3]) newPos.x--;
                    break;
                case sf::Keyboard::Escape:
                    state = MENU;
                    break;
                default:
                    break;
                }
                playerPos = newPos;

                // Check win condition
                if (playerPos.x == mazeSize - 1 && playerPos.y == mazeSize - 1) {
                    state = MENU;
                }
            }
        }

        // Update hint timer
        if (showHint && hintTimer.getElapsedTime().asSeconds() > 3.0f) {
            showHint = false;
            hintPath.clear();
        }
    }

    void draw() {
        window.clear(sf::Color::Black);

        switch (state) {
        case MENU:
            for (const auto& button : menuButtons) {
                button.draw(window);
            }
            break;

        case DIFFICULTY_SELECT:
            for (const auto& button : difficultyButtons) {
                button.draw(window);
            }
            break;

        case PLAYING:
            // Calculate offset to center maze
            float offsetX = (800 - mazeSize * CELL_SIZE) / 2;
            float offsetY = (600 - mazeSize * CELL_SIZE) / 2;

            // Draw maze
            for (int y = 0; y < mazeSize; y++) {
                for (int x = 0; x < mazeSize; x++) {
                    float px = x * CELL_SIZE + offsetX;
                    float py = y * CELL_SIZE + offsetY;

                    // Draw walls
                    if (maze[y][x].walls[0]) { // Top
                        sf::RectangleShape wall(sf::Vector2f(CELL_SIZE, 2));
                        wall.setPosition(px, py);
                        wall.setFillColor(sf::Color::White);
                        window.draw(wall);
                    }
                    if (maze[y][x].walls[1]) { // Right
                        sf::RectangleShape wall(sf::Vector2f(2, CELL_SIZE));
                        wall.setPosition(px + CELL_SIZE - 2, py);
                        wall.setFillColor(sf::Color::White);
                        window.draw(wall);
                    }
                    if (maze[y][x].walls[2]) { // Bottom
                        sf::RectangleShape wall(sf::Vector2f(CELL_SIZE, 2));
                        wall.setPosition(px, py + CELL_SIZE - 2);
                        wall.setFillColor(sf::Color::White);
                        window.draw(wall);
                    }
                    if (maze[y][x].walls[3]) { // Left
                        sf::RectangleShape wall(sf::Vector2f(2, CELL_SIZE));
                        wall.setPosition(px, py);
                        wall.setFillColor(sf::Color::White);
                        window.draw(wall);
                    }
                }
            }

            // Draw hint path
            if (showHint && !hintPath.empty()) {
                for (const auto& pos : hintPath) {
                    sf::RectangleShape pathCell(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
                    pathCell.setPosition(
                        pos.x * CELL_SIZE + offsetX + 2,
                        pos.y * CELL_SIZE + offsetY + 2
                    );
                    pathCell.setFillColor(sf::Color(255, 255, 0, 128));
                    window.draw(pathCell);
                }
            }

            // Draw goal
            sf::RectangleShape goal(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
            goal.setPosition(
                (mazeSize - 1) * CELL_SIZE + offsetX + 2,
                (mazeSize - 1) * CELL_SIZE + offsetY + 2
            );
            goal.setFillColor(sf::Color::Green);
            window.draw(goal);

            // Draw player
            sf::RectangleShape player(sf::Vector2f(CELL_SIZE - 8, CELL_SIZE - 8));
            player.setPosition(
                playerPos.x * CELL_SIZE + offsetX + 4,
                playerPos.y * CELL_SIZE + offsetY + 4
            );
            player.setFillColor(sf::Color::Red);
            window.draw(player);

            // Draw timer
            std::stringstream ss;
            ss << "Time: " << static_cast<int>(gameTimer.getElapsedTime().asSeconds()) << "s";
            sf::Text timerText(ss.str(), font, 20);
            timerText.setPosition(10, 10);
            timerText.setFillColor(sf::Color::White);
            window.draw(timerText);

            // Draw hint button
            hintButton.draw(window);
            break;
        }

        window.display();
    }
};

int main() {
    srand(time(0));
    try {
        Game game;
        game.run();
    }
    catch (const std::exception& e) {
        cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
