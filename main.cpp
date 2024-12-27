#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <random>
#include <ctime>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
using namespace std;

enum Difficulty { EASY = 15, MEDIUM = 20, HARD = 25 };
enum GameState { MENU, DIFFICULTY_SELECT, PLAYING, MULTIPLAYER_PLAYING, SCORES };

sf::Texture backgroundTexture;
sf::Sprite backgroundSprite;
sf::Texture playerTextureRight, playerTextureLeft;
sf::Sprite playerSprite;
sf::Texture cellTexture;
sf::Texture wallTextureHorizontal, wallTextureVertical;
sf::Texture goalTexture;
sf::Sprite goalSprite;
sf::Music backgroundMusic;


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
        shape.setSize(sf::Vector2f(350, 100)); // Default size
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color::Black);

        text.setString(str);
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color(192, 192, 192));

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
    int score = 300; // Starting score
    // Declare lastTime as a member variable or global
    int lastTime = 0; // Ensure it's initialized to 0

private:
    // Existing variables
    bool isFacingRight = true; // True if facing right, false if facing left

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
    bool isGameWonM = false;
    bool isGameLost = false;
    Button hintButton1{ "Hint Player 1", font, 50, 50 };  // Player 1 hint button
    Button hintButton2{ "Hint Player 2", font, 650, 50 };// Player 2 hint button
    std::vector<sf::Vector2i> hintPath1;                // Hint path for Player 1
    std::vector<sf::Vector2i> hintPath2;                // Hint path for Player 2

    sf::Time completionTime;
    std::vector<std::vector<Cell>> maze2;  // Second maze
    sf::Vector2i player1Pos{ 0, 0 };         // Player 1 position
    sf::Vector2i player2Pos{ 0, 0 };         // Player 2 position
    int winningPlayer = 0;                 // Tracks the winning player (1 or 2)
    bool isMultiplayerMazeGenerated = false;


    bool showHint1 = false;                             // Whether Player 1's hint is active
    bool showHint2 = false;                             // Whether Player 2's hint is active
    sf::Clock hintTimer1;                               // Timer for Player 1 hint
    sf::Clock hintTimer2;                               // Timer for Player 2 hint
    int hintCount1 = 3;                                 // Number of hints for Player 1
    int hintCount2 = 3;                                 // Number of hints for Player 2
    // Ensure mazes are generated once

    void resetGame() {
        score = 300;
        hintCount = 3;
        playerPos = sf::Vector2i(0, 0);
        maze.clear();
        gameTimer.restart(); // Restart the game timer
        hintPath.clear();
        state = MENU;

        lastTime = 0;
        isGameLost = false;

        generateMaze(); // Regenerate the maze
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
    std::vector<sf::Vector2i> findPathForPlayer(const std::vector<std::vector<Cell>>& maze, sf::Vector2i start) {
        std::vector<std::vector<sf::Vector2i>> parent(mazeSize, std::vector<sf::Vector2i>(mazeSize, sf::Vector2i(-1, -1)));
        std::vector<std::vector<bool>> visited(mazeSize, std::vector<bool>(mazeSize, false));
        std::queue<sf::Vector2i> q;

        q.push(start);
        visited[start.y][start.x] = true;

        sf::Vector2i goal(mazeSize - 1, mazeSize - 1); // Target is the bottom-right corner
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

        // Build the path from the goal back to the start
        std::vector<sf::Vector2i> path;
        if (foundPath) {
            sf::Vector2i current = goal;
            while (current != start) {
                path.push_back(current);
                current = parent[current.y][current.x];
            }
            std::reverse(path.begin(), path.end());
        }
        return path;
    }

    void saveScoreToFile(const std::string& difficulty, int completionTime) {
        std::ofstream outFile("scores.csv", std::ios::app);

        if (outFile.is_open()) {
            // Get the current time and format it as hh:mm and date
            auto now = std::chrono::system_clock::now();
            std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm localTime;

            // Use localtime_s for safer conversion
            if (localtime_s(&localTime, &nowTime) != 0) {
                std::cerr << "Error: Unable to get local time." << std::endl;
                return;
            }

            char timeBuffer[10];
            char dateBuffer[20];
            std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &localTime);
            std::strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &localTime);

            // Write the data to the file
            outFile << difficulty << ","
                << score << ","
                << completionTime << "s,"
                << dateBuffer << " " << timeBuffer << "\n";

            outFile.close();
        }
        else {
            std::cerr << "Error: Unable to open file for saving scores.\n";
        }
    }
    void drawWall(sf::Vector2f position, sf::Vector2f size, sf::Texture& texture) {
        sf::Sprite wallSprite;
        wallSprite.setTexture(texture);
        wallSprite.setScale(
            (size.x / texture.getSize().x) * 2.0,
            (size.y / texture.getSize().y) * 2.0
        );
        wallSprite.setPosition(position);

        window.draw(wallSprite);
    }
    void drawMultiplayerMaze(const std::vector<std::vector<Cell>>& maze, float offsetX, float offsetY, sf::Color playerColor, sf::Vector2i playerPos) {
        for (int y = 0; y < mazeSize; ++y) {
            for (int x = 0; x < mazeSize; ++x) {
                float px = x * CELL_SIZE + offsetX;
                float py = y * CELL_SIZE + offsetY;

                // Draw walls with appropriate textures
                if (maze[y][x].walls[0]) // Top wall
                    drawWall({ px, py }, { static_cast<float>(CELL_SIZE - 10), 12.0f }, wallTextureHorizontal);

                if (maze[y][x].walls[1]) // Right wall
                    drawWall({ px + CELL_SIZE , py }, { 12.0f, static_cast<float>(CELL_SIZE - 10) }, wallTextureVertical);

                if (maze[y][x].walls[2]) // Bottom wall
                    drawWall({ px, py + CELL_SIZE }, { static_cast<float>(CELL_SIZE - 10), 12.0f }, wallTextureHorizontal);

                if (maze[y][x].walls[3]) // Left wall
                    drawWall({ px, py }, { 12.0f, static_cast<float>(CELL_SIZE - 10) }, wallTextureVertical);
            }
        }



        if (showHint1 && &maze == &this->maze) { // Show Player 1's hint
            for (const auto& pos : hintPath1) {
                sf::RectangleShape hintCell(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
                hintCell.setPosition(pos.x * CELL_SIZE + offsetX + 10, pos.y * CELL_SIZE + offsetY + 18);
                hintCell.setFillColor(sf::Color(255, 255, 0, 128)); // Yellow, semi-transparent
                window.draw(hintCell);
            }
        }

        if (showHint2 && &maze == &this->maze2) { // Show Player 2's hint
            for (const auto& pos : hintPath2) {
                sf::RectangleShape hintCell(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
                hintCell.setPosition(pos.x * CELL_SIZE + offsetX + 10, pos.y * CELL_SIZE + offsetY + 18);
                hintCell.setFillColor(sf::Color(255, 255, 0, 128)); // Yellow, semi-transparent
                window.draw(hintCell);
            }
        }

        // Draw goal node (bottom-right corner) in green
        goalSprite.setPosition(
            (mazeSize - 1) * CELL_SIZE + offsetX + 2,
            (mazeSize - 1) * CELL_SIZE + offsetY + 2
        );
        goalSprite.setScale(
            CELL_SIZE / static_cast<float>(playerTextureRight.getSize().x) * 1.3f,
            CELL_SIZE / static_cast<float>(playerTextureRight.getSize().y) * 1.3f
        );
        window.draw(goalSprite);

        // Draw player
        playerSprite.setPosition(
            playerPos.x * CELL_SIZE + offsetX + 2,
            playerPos.y * CELL_SIZE + offsetY + 3
        );

        // Adjust the sprite's scale dynamically to fit the CELL_SIZE
        playerSprite.setScale(
            CELL_SIZE / static_cast<float>(playerTextureRight.getSize().x) * 1.3f,
            CELL_SIZE / static_cast<float>(playerTextureRight.getSize().y) * 1.3f
        );


        // Render the player sprite
        window.draw(playerSprite);

    }


    void generateMultiplayerMaze() {
        maze.clear();
        maze2.clear();
        maze.resize(mazeSize, std::vector<Cell>(mazeSize));
        generateMaze();
        maze2.clear();
        maze2.resize(mazeSize, std::vector<Cell>(mazeSize));
        generateMaze();
        maze2 = maze;   // Copy the maze for player 2
        isMultiplayerMazeGenerated = true;
    }



public:
    Game() : //gameconstruct
        window(sf::VideoMode(1000, 850), "Trapped"),
        hintButton("Hint", font, 10, 120) // Adjusted position for the hint button
    {

        if (!backgroundTexture.loadFromFile("background.jpg")) {
            throw std::runtime_error("Could not load background image");
        }
        backgroundSprite.setTexture(backgroundTexture);
        if (!playerTextureRight.loadFromFile("player_right.png")) {
            throw std::runtime_error("Could not load right-facing texture");
        }
        if (!playerTextureLeft.loadFromFile("player_left.png")) {
            throw std::runtime_error("Could not load left-facing texture");
        }
        playerSprite.setTexture(playerTextureRight); // Assuming the player starts facing right

        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Could not load font");
        }
        if (!cellTexture.loadFromFile("cell.png")) { // Replace with your image path
            throw std::runtime_error("Could not load cell texture");
        }
        if (!wallTextureHorizontal.loadFromFile("wall_horizontal.png")) {
            throw std::runtime_error("Could not load horizontal wall texture");
        }
        if (!wallTextureVertical.loadFromFile("wall_vertical.png")) {
            throw std::runtime_error("Could not load vertical wall texture");
        }
        if (!goalTexture.loadFromFile("goal.png")) { // Replace with your image file path
            throw std::runtime_error("Could not load goal image");
        }
        goalSprite.setTexture(goalTexture);

        if (!backgroundMusic.openFromFile("background.wav")) { // Use your audio file here
            throw std::runtime_error("Could not load background music");
        }
        backgroundMusic.setLoop(true); // Loop the music
        backgroundMusic.setVolume(50); // Adjust volume (0-100)
        backgroundMusic.play();

        menuButtons.emplace_back("Single Player", font, 300, 200);
        menuButtons.emplace_back("Multiplayer", font, 300, 350);
        menuButtons.emplace_back("Scores", font, 300, 500); // Position the Scores button below others

        difficultyButtons.emplace_back("Easy", font, 300, 150);
        difficultyButtons.emplace_back("Medium", font, 300, 300);
        difficultyButtons.emplace_back("Hard", font, 300, 450);

        hintButton.setSize(sf::Vector2f(100, 40)); // Reduced size
        hintButton1.setSize(sf::Vector2f(150, 40));
        hintButton2.setSize(sf::Vector2f(150, 40));
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

    void resetGameForMultiplayer() {
        score = 3;
        hintCount = 3;
        hintCount1 = 3;
        hintCount2 = 3;
        player1Pos = { 0, 0 };
        player2Pos = { 0, 0 };
        isGameWonM = false;
        winningPlayer = 0;
        generateMultiplayerMaze();

    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            draw();
        }
    }
    void updateScore(bool isMove = false, bool isHintUsed = false) {
        int currentTime = static_cast<int>(gameTimer.getElapsedTime().asSeconds());
        if (currentTime >= 180) {
            score = 0;
        }
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

                if (state == MENU) {
                    for (size_t i = 0; i < menuButtons.size(); ++i) {
                        if (menuButtons[i].contains(sf::Mouse::getPosition(window))) {
                            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                                if (i == 0) { // Single Player button
                                    resetGame();
                                    state = DIFFICULTY_SELECT;

                                }
                                else if (i == 1) { // Multiplayer button
                                    resetGameForMultiplayer();
                                    state = MULTIPLAYER_PLAYING;
                                }
                                else if (i == 2) { // Scores button
                                    state = SCORES;

                                }
                            }
                        }
                    }
                }
                else if (state == DIFFICULTY_SELECT) {

                    for (size_t i = 0; i < difficultyButtons.size(); i++) {
                        if (difficultyButtons[i].contains(sf::Mouse::getPosition(window))) {
                            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
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
                else if (state == SCORES) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                        state = MENU; // Return to the main menu
                    }
                }

            }
            if (state == MULTIPLAYER_PLAYING) {
                if (event.type == sf::Event::KeyPressed) {
                    // Player 1 (Arrow keys)
                    sf::Vector2i newPos1 = player1Pos;
                    switch (event.key.code) {
                    case sf::Keyboard::Up:    if (!maze[player1Pos.y][player1Pos.x].walls[0]) newPos1.y--; break;
                    case sf::Keyboard::Right: if (!maze[player1Pos.y][player1Pos.x].walls[1]) newPos1.x++; break;
                    case sf::Keyboard::Down:  if (!maze[player1Pos.y][player1Pos.x].walls[2]) newPos1.y++; break;
                    case sf::Keyboard::Left:  if (!maze[player1Pos.y][player1Pos.x].walls[3]) newPos1.x--; break;
                    }
                    player1Pos = newPos1;

                    // Player 2 (WASD)
                    sf::Vector2i newPos2 = player2Pos;
                    switch (event.key.code) {
                    case sf::Keyboard::W: if (!maze2[player2Pos.y][player2Pos.x].walls[0]) newPos2.y--; break;
                    case sf::Keyboard::D: if (!maze2[player2Pos.y][player2Pos.x].walls[1]) newPos2.x++; break;
                    case sf::Keyboard::S: if (!maze2[player2Pos.y][player2Pos.x].walls[2]) newPos2.y++; break;
                    case sf::Keyboard::A: if (!maze2[player2Pos.y][player2Pos.x].walls[3]) newPos2.x--; break;
                    }
                    player2Pos = newPos2;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                        state = MENU; // Return to the main menu
                    }
                    // Add winning condition here
                    if (player1Pos.x == mazeSize - 1 && player1Pos.y == mazeSize - 1) {
                        winningPlayer = 1;
                        isGameWonM = true;
                    }
                    else if (player2Pos.x == mazeSize - 1 && player2Pos.y == mazeSize - 1) {
                        winningPlayer = 2;
                        isGameWonM = true;
                    }
                }
            }
            if (state == MULTIPLAYER_PLAYING && event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Player 1 hint button
                if (hintButton1.contains(mousePos) && hintCount1 > 0) {
                    showHint1 = true;
                    hintTimer1.restart();
                    hintPath1 = findPathForPlayer(maze, player1Pos); // Generate hint path for Player 1
                    hintCount1--;
                }

                // Player 2 hint button
                if (hintButton2.contains(mousePos) && hintCount2 > 0) {
                    showHint2 = true;
                    hintTimer2.restart();
                    hintPath2 = findPathForPlayer(maze2, player2Pos); // Generate hint path for Player 2
                    hintCount2--;
                }
            }

            // Update hint timers to hide hints after a few seconds
            if (showHint1 && hintTimer1.getElapsedTime().asSeconds() > 2.0f) {
                showHint1 = false;
                hintPath1.clear();
            }

            if (showHint2 && hintTimer2.getElapsedTime().asSeconds() > 2.0f) {
                showHint2 = false;
                hintPath2.clear();
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
                        isFacingRight = true; // Update direction
                        playerSprite.setTexture(playerTextureRight); // Switch to right-facing texture

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
                        isFacingRight = false; // Update direction
                        playerSprite.setTexture(playerTextureLeft); // Switch to left-facing texture
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
                            int timeTaken = static_cast<int>(gameTimer.getElapsedTime().asSeconds());
                            saveScoreToFile(mazeSize == EASY ? "Easy" : (mazeSize == MEDIUM ? "Medium" : "Hard"), timeTaken);
                            resetGame();
                            resetGameForMultiplayer();
                            state = MENU;
                            isGameWon = false;
                        }
                        return;
                    }
                    if (state == PLAYING) {
                        // Check if the game is lost
                        if (score <= 0) {
                            isGameLost = true;

                            // Display a game-over message and handle resetting
                            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                                resetGame();
                                state = MENU;
                            }
                            return; // Exit early to avoid further updates
                        }
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
        // Update hint timers to hide hints after 2 seconds
        if (showHint1 && hintTimer1.getElapsedTime().asSeconds() > 2.0f) {
            showHint1 = false;
            hintPath1.clear(); // Clear Player 1's hint path
        }

        if (showHint2 && hintTimer2.getElapsedTime().asSeconds() > 2.0f) {
            showHint2 = false;
            hintPath2.clear(); // Clear Player 2's hint path
        }


    }

    void draw() { //drawconstruct
        window.clear(sf::Color::Black);
        window.draw(backgroundSprite);

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
            else if (isGameLost) {
                // Center position calculations
                float centerX = window.getSize().x / 2.0f;
                float centerY = window.getSize().y / 2.0f;

                // Main game over text
                sf::Text loseText("Game Over!", font, 40);
                loseText.setFillColor(sf::Color::Red);
                loseText.setPosition(
                    centerX - loseText.getLocalBounds().width / 2,
                    centerY - 100
                );

                // Score text
                sf::Text scoreText("Final Score: " + std::to_string(score), font, 30);
                scoreText.setFillColor(sf::Color::White);
                scoreText.setPosition(
                    centerX - scoreText.getLocalBounds().width / 2,
                    centerY
                );

                // Instruction text
                sf::Text instructionText("Press Enter to retry or return to menu", font, 25);
                instructionText.setFillColor(sf::Color(150, 150, 150));
                instructionText.setPosition(
                    centerX - instructionText.getLocalBounds().width / 2,
                    centerY + 120
                );

                // Draw all game over screen elements
                window.draw(loseText);
                window.draw(scoreText);
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
                        if (maze[y][x].walls[0]) // Top wall
                            drawWall({ px, py }, { static_cast<float>(CELL_SIZE-10), 12.0f }, wallTextureHorizontal);

                        if (maze[y][x].walls[1]) // Right wall
                            drawWall({ px + CELL_SIZE , py }, { 12.0f, static_cast<float>(CELL_SIZE-10) }, wallTextureVertical);

                        if (maze[y][x].walls[2]) // Bottom wall
                            drawWall({ px, py + CELL_SIZE }, { static_cast<float>(CELL_SIZE-10), 12.0f }, wallTextureHorizontal);

                        if (maze[y][x].walls[3]) // Left wall
                            drawWall({ px, py }, { 12.0f, static_cast<float>(CELL_SIZE-10) }, wallTextureVertical);
                    }
                }



                // Draw goal
                //sf::RectangleShape goal(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
                //goal.setPosition(
                  //  (mazeSize - 1) * CELL_SIZE + offsetX + 2,
                    //(mazeSize - 1) * CELL_SIZE + offsetY + 2
                //);
                //goal.setFillColor(sf::Color::Green);
               // window.draw(goal);
                goalSprite.setPosition(
                    (mazeSize - 1)* CELL_SIZE + offsetX ,
                    (mazeSize - 1)* CELL_SIZE + offsetY
                );
                goalSprite.setScale(
                    CELL_SIZE / static_cast<float>(playerTextureRight.getSize().x) ,
                    CELL_SIZE / static_cast<float>(playerTextureRight.getSize().y) 
                );
                window.draw(goalSprite);

                // Draw player
                playerSprite.setPosition(
                    playerPos.x * CELL_SIZE + offsetX + 2,
                    playerPos.y * CELL_SIZE + offsetY + 3
                );

                // Adjust the sprite's scale dynamically to fit the CELL_SIZE
                playerSprite.setScale(
                    CELL_SIZE / static_cast<float>(playerTextureRight.getSize().x) * 1.3f,
                    CELL_SIZE / static_cast<float>(playerTextureRight.getSize().y) * 1.3f
                );


                // Render the player sprite
                window.draw(playerSprite);

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
                            pos.x * CELL_SIZE + offsetX + 10,
                            pos.y * CELL_SIZE + offsetY + 18
                        );
                        pathCell.setFillColor(sf::Color(255, 255, 0, 128));
                        window.draw(pathCell);
                    }
                }
            }
            break;
        case SCORES: {
            // Static variables for sorting and data loading
            static bool isSorted = false;
            static std::vector<std::vector<std::string>> rows;

            // Only load data if not already loaded
            static bool dataLoaded = false;
            if (!dataLoaded) {
                std::ifstream inFile("scores.csv");
                rows.clear(); // Clear any previous data
                if (!inFile.is_open()) {
                    sf::Text errorText("Error: Unable to load scores.", font, 30);
                    errorText.setPosition(300, 300);
                    errorText.setFillColor(sf::Color::Red);
                    window.draw(errorText);
                    break;
                }

                std::string line;
                std::getline(inFile, line); // Skip the header

                // Read each row from the file
                while (std::getline(inFile, line)) {
                    std::istringstream ss(line);
                    std::string difficulty, score, timeTaken, dateTime;

                    std::getline(ss, difficulty, ',');
                    std::getline(ss, score, ',');
                    std::getline(ss, timeTaken, ',');
                    std::getline(ss, dateTime, ',');

                    rows.push_back({ difficulty, score, timeTaken, dateTime });
                }
                inFile.close();
                dataLoaded = true; // Data is loaded
            }

            // Draw the title
            sf::Text titleText("Scores", font, 40);
            titleText.setPosition(400, 50);
            titleText.setFillColor(sf::Color::Yellow);
            window.draw(titleText);

            // Draw the headings
            sf::Text headings("Difficulty | Score | Time Taken | Date & Time", font, 20);
            headings.setPosition(150, 100);
            headings.setFillColor(sf::Color::White);
            window.draw(headings);

            // Add Sort button
            Button sortButton("Sort", font, 600, 50);
            sortButton.draw(window);

            // Check for Sort button click
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (sortButton.contains(mousePos)) {
                    if (!isSorted) {
                        // Perform insertion sort
                        for (size_t i = 1; i < rows.size(); ++i) {
                            auto key = rows[i];
                            int j = i - 1;

                            while (j >= 0 && std::stoi(rows[j][1]) < std::stoi(key[1])) {
                                rows[j + 1] = rows[j];
                                --j;
                            }
                            rows[j + 1] = key;
                        }
                        isSorted = true; // Mark as sorted
                    }
                }
            }

            // Display rows
            int y_offset = 150;
            for (const auto& row : rows) {
                std::string rowText = row[0] + " | " + row[1] + " | " + row[2] + " | " + row[3];

                sf::Text rowTextDisplay(rowText, font, 20);
                rowTextDisplay.setPosition(150, y_offset);
                rowTextDisplay.setFillColor(sf::Color::White);
                window.draw(rowTextDisplay);

                y_offset += 30;
            }

            // Instruction to return to menu
            sf::Text returnText("Press ESC to return to menu.", font, 20);
            returnText.setPosition(300, 700);
            returnText.setFillColor(sf::Color::White);
            window.draw(returnText);

            // Reset state when ESC is pressed
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                dataLoaded = false; // Reset data loading flag
                isSorted = false;   // Reset sorting flag
                state = MENU;       // Return to menu
            }
            break;

        }
        case MULTIPLAYER_PLAYING: {
            float maze1OffsetX = (window.getSize().x / 4) - (mazeSize * CELL_SIZE / 2);
            float maze2OffsetX = (3 * window.getSize().x / 4) - (mazeSize * CELL_SIZE / 2);
            float offsetY = (window.getSize().y - mazeSize * CELL_SIZE) / 2;

            drawMultiplayerMaze(maze, maze1OffsetX, offsetY, sf::Color::Red, player1Pos);
            drawMultiplayerMaze(maze2, maze2OffsetX, offsetY, sf::Color::Blue, player2Pos);

            hintButton1.draw(window); // Player 1 hint button
            hintButton2.draw(window);
            // Display win message if a player wins
            if (isGameWonM) {
                sf::Text winText("Player " + std::to_string(winningPlayer) + " Wins!", font, 40);
                winText.setFillColor(sf::Color::Yellow);
                winText.setPosition(window.getSize().x / 2 - winText.getLocalBounds().width / 2, window.getSize().y / 2);
                window.draw(winText);

                // Allow players to return to the menu
                sf::Text returnText("Press Enter to return to the menu", font, 20);
                returnText.setFillColor(sf::Color::White);
                returnText.setPosition(window.getSize().x / 2 - returnText.getLocalBounds().width / 2, window.getSize().y / 2 + 50);
                window.draw(returnText);

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                    state = MENU;
                }
            }

            break;
        }

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
