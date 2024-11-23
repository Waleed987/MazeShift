#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <random>
#include <ctime>
#include <sstream>

using namespace std;


enum Difficulty { EASY = 15, MEDIUM = 20, HARD = 25 };
enum GameState { MENU, DIFFICULTY_SELECT, PLAYING };

class Cell {
public:
    bool visited = false;
    bool walls[4] = { true, true, true, true }; // Top, Right, Bottom, Left
    bool isPath = false; // For marking cells in the hint path
};

// ... [Previous Button class code remains the same] ...
class Button {
    sf::RectangleShape shape;
    sf::Text text;

public:
    Button(const std::string& str, const sf::Font& font, float x, float y) {
        shape.setSize(sf::Vector2f(350, 100)); // Default size
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(100, 100, 100));

        text.setString(str);
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);

        text.setPosition(
            x + (shape.getSize().x - text.getLocalBounds().width) / 2,
            y + (shape.getSize().y - text.getLocalBounds().height) / 2
        );
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        window.draw(text);
    }

    bool contains(sf::Vector2i point) const {
        return shape.getGlobalBounds().contains(point.x, point.y);
    }

    void setSize(const sf::Vector2f& size) {
        shape.setSize(size);
        text.setCharacterSize(static_cast<unsigned int>(size.y / 2)); // Adjust text size proportionally

        // Recalculate the text position to center it within the new button size
        text.setPosition(
            shape.getPosition().x + (size.x - text.getLocalBounds().width) / 2,
            shape.getPosition().y + (size.y - text.getLocalBounds().height) / 2
        );
    }
};

class Game {
public:
    int score = 1000; // Starting score
    // Declare lastTime as a member variable or global
    int lastTime = 0; // Ensure it's initialized to 0

private:
    // Existing variables
    sf::RenderWindow window;
    sf::Font font;
    GameState state = MENU;
    int mazeSize = EASY;
    int CELL_SIZE = 30;
    std::vector<std::vector<Cell>> maze;
    sf::Vector2i playerPos{ 0, 0 };
    sf::Clock gameTimer;
    bool showHint = false;
    sf::Clock hintTimer;

    Button hintButton;
    std::vector<sf::Vector2i> hintPath;

    std::vector<Button> menuButtons;
    std::vector<Button> difficultyButtons;
    bool isGameWon = false;
    sf::Time completionTime;


    void resetGame() {
        // Reset game variables
        score = 1000;
        hintCount = 3;
        playerPos = sf::Vector2i(0, 0);
        gameTimer.restart();  // Reset the timer
        hintPath.clear();
        state = MENU;

        // Reset lastTime to match the new timer
        lastTime = 0;

        // Regenerate the maze
        generateMaze();
    }

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
        window(sf::VideoMode(1000, 800), "Maze Game"),
        hintButton("Hint", font, 10, 120) // Adjusted position for the hint button
    {
        if (!font.loadFromFile("C:\\Users\\Waleed\\Desktop\\arial.ttf")) {
            throw std::runtime_error("Could not load font");
        }

        menuButtons.emplace_back("Single Player", font, 300, 200);
        menuButtons.emplace_back("Multiplayer", font, 300, 350);

        difficultyButtons.emplace_back("Easy", font, 300, 150);
        difficultyButtons.emplace_back("Medium", font, 300, 300);
        difficultyButtons.emplace_back("Hard", font, 300, 450);

        hintButton.setSize(sf::Vector2f(100, 40)); // Reduced size
    }

    // Adjust cell size based on difficulty
    void setDifficulty(int difficulty) {
        mazeSize = difficulty;
        if (difficulty == EASY) {
            CELL_SIZE = 50; // Larger cells for easy difficulty
        }
        else if (difficulty == MEDIUM) {
            CELL_SIZE = 40; // Medium cells for medium difficulty
        }
        else if (difficulty == HARD) {
            CELL_SIZE = 30; // Smaller cells for hard difficulty
        }
        generateMaze(); // Regenerate the maze for the new difficulty
    }



    void run() {
        while (window.isOpen()) {
            handleEvents();
            draw();
        }
    }
    void updateScore(bool isMove = false, bool isHintUsed = false) {
        int currentTime = static_cast<int>(gameTimer.getElapsedTime().asSeconds());

        // Deduct time-based points only if the game is running
        if (currentTime > lastTime) {
            score = std::max(0, score - (currentTime - lastTime));
            lastTime = currentTime; // Update lastTime
        }

        // Deduct points for movement
        if (isMove) {
            score = std::max(0, score - 5);
        }

        // Deduct points for using a hint
        if (isHintUsed) {
            score = std::max(0, score - 50);
        }
    }



private:
    int hintCount = 3;  // Player starts with 3 hints
    void handleEvents() {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Handle hint button click and check if hints are available
                if (state == PLAYING && hintButton.contains(mousePos) && hintCount > 0) {
                    showHint = true;
                    hintTimer.restart();
                    hintPath = findPath(); // Generate hint path
                    updateScore(false, true); // Indicate a hint was used
                    --hintCount; // Decrease the hint count by 1
                }
                // Provide feedback if there are no hints left
                else if (state == PLAYING && hintButton.contains(mousePos) && hintCount == 0) {
                    std::cout << "No hints left!" << std::endl;
                }

                // Handle menu and difficulty selection buttons
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
                            int selectedDifficulty = (i == 0) ? EASY : (i == 1) ? MEDIUM : HARD;
                            setDifficulty(selectedDifficulty);
                            state = PLAYING;
                            playerPos = sf::Vector2i(0, 0);
                            gameTimer.restart();
                            hintPath.clear();
                        }
                    }
                }

            }

            // Handle player movement
            if (state == PLAYING && event.type == sf::Event::KeyPressed) {
                sf::Vector2i newPos = playerPos;
                bool isMoveValid = false;  // Flag to check if the move is valid
                bool isValidMove = false;  // To check if player is making a valid move

                switch (event.key.code) {
                case sf::Keyboard::Up:
                    if (!maze[playerPos.y][playerPos.x].walls[0]) {
                        newPos.y--;
                        isMoveValid = true;
                    }
                    break;
                case sf::Keyboard::Right:
                    if (!maze[playerPos.y][playerPos.x].walls[1]) {
                        newPos.x++;
                        isMoveValid = true;
                    }
                    break;
                case sf::Keyboard::Down:
                    if (!maze[playerPos.y][playerPos.x].walls[2]) {
                        newPos.y++;
                        isMoveValid = true;
                    }
                    break;
                case sf::Keyboard::Left:
                    if (!maze[playerPos.y][playerPos.x].walls[3]) {
                        newPos.x--;
                        isMoveValid = true;
                    }
                    break;
                case sf::Keyboard::Escape:
                    state = MENU;
                    break;
                default:
                    break;
                }

                // Only handle valid moves
                if (isMoveValid) {
                    // Check if the new position is the same as a visited one
                    if (maze[newPos.y][newPos.x].visited) {
                        // Allow the move without point deduction
                        isValidMove = true;
                        updateScore(true, false);
                    }
                    else {
                        // New unvisited cell, mark it as visited
                        maze[newPos.y][newPos.x].visited = true;
                        isValidMove = true;
                        updateScore(false, false); // No penalty for valid new moves
                    }

                    // If it's a valid move (whether revisiting or a new visit), update the player position
                    if (isValidMove) {
                        playerPos = newPos;
                    }
                    else {
                        // If the move is invalid (due to wall), do nothing
                    }
                }
                if (state == PLAYING) {
                    // Check if the game is won
                    if (isGameWon) {
                        // Handle Enter key to return to menu after winning
                        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                            resetGame();
                            state = MENU;
                            isGameWon = false;
                        }
                        return;
                    }

                    // Existing movement and other logic...
                    if (playerPos.x == mazeSize - 1 && playerPos.y == mazeSize - 1) {
                        isGameWon = true;
                        completionTime = gameTimer.getElapsedTime(); // Store the time when the player wins
                    }
                }

            }
        }

        // Update hint timer
        if (showHint && hintTimer.getElapsedTime().asSeconds() > 2.0f) {
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
            if (isGameWon) {
                // Center position calculations
                float centerX = window.getSize().x / 2.0f;
                float centerY = window.getSize().y / 2.0f;

                // Main congratulations text
                sf::Text winText("Congratulations! You Won!", font, 40);
                winText.setFillColor(sf::Color::Yellow);
                winText.setPosition(
                    centerX - winText.getLocalBounds().width / 2,
                    centerY - 100
                );

                // Score text
                sf::Text scoreText("Final Score: " + std::to_string(score), font, 30);
                scoreText.setFillColor(sf::Color::White);
                scoreText.setPosition(
                    centerX - scoreText.getLocalBounds().width / 2,
                    centerY
                );

                // Time text
                sf::Text timeText("Time: " + std::to_string(static_cast<int>(completionTime.asSeconds())) + "s", font, 30);
                timeText.setFillColor(sf::Color::White);
                timeText.setPosition(
                    centerX - timeText.getLocalBounds().width / 2,
                    centerY + 50
                );

                // Instruction text
                sf::Text instructionText("Press Enter to return to menu", font, 25);
                instructionText.setFillColor(sf::Color(150, 150, 150));
                instructionText.setPosition(
                    centerX - instructionText.getLocalBounds().width / 2,
                    centerY + 120
                );

                // Draw all win screen elements
                window.draw(winText);
                window.draw(scoreText);
                window.draw(timeText);
                window.draw(instructionText);
            }
            else {
                // Regular game rendering
                float mazeAreaWidth = 1000;
                float offsetX = (mazeAreaWidth - mazeSize * CELL_SIZE) / 2;
                float offsetY = (800 - mazeSize * CELL_SIZE) / 2;

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

                // Draw UI elements
                sf::Text timerText("Time: " + std::to_string(static_cast<int>(gameTimer.getElapsedTime().asSeconds())) + "s", font, 20);
                timerText.setPosition(10, 10);
                timerText.setFillColor(sf::Color::White);
                window.draw(timerText);

                sf::Text scoreText("Score: " + std::to_string(score), font, 20);
                scoreText.setPosition(10, 50);
                scoreText.setFillColor(sf::Color::White);
                window.draw(scoreText);

                sf::Text hintCountText("Hints: " + std::to_string(hintCount), font, 20);
                hintCountText.setPosition(10, 90);
                hintCountText.setFillColor(sf::Color::White);
                window.draw(hintCountText);

                hintButton.draw(window);

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
            }
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
