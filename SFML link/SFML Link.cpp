#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <stack>
#include <map>
#include <string>
#include <deque>
#include <iostream>
#include <cstring>
#include <SFML/Audio.hpp>

const int CELL_SIZE = 40;
int CELL_NUM_X = 20;
int CELL_NUM_Y = 20;

bool retry = false;
bool IS_SNAKE = true;

struct directions {
    sf::Vector2f UP{ 0, -1 };
    sf::Vector2f DOWN{ 0, 1 };
    sf::Vector2f RIGHT{ 1, 0 };
    sf::Vector2f LEFT{ -1, 0 };
};

struct directions2 {
    sf::Vector2f UP{ 0, -40 };
    sf::Vector2f DOWN{ 0, 40 };
    sf::Vector2f RIGHT{ 40, 0 };
    sf::Vector2f LEFT{ -40, 0 };
};

enum assetsID {
    MAIN_FONT = 0,
    GRASS,
    FOOD,
    EAT_SOUND,

    ICON,

    HEAD_UP,
    HEAD_DOWN,
    HEAD_RIGHT,
    HEAD_LEFT,

    TAIL_UP,
    TAIL_DOWN,
    TAIL_RIGHT,
    TAIL_LEFT,

    BODY_VERTICAL,
    BODY_HORIZONTAL,

    BODY_BOTTOM_LEFT,
    BODY_BOTTOM_RIGHT,
    BODY_TOP_LEFT,
    BODY_TOP_RIGHT,

};

struct State {
    State() {};

    virtual ~State() {};

    virtual void Init() = 0;

    virtual void ProcessInput() = 0;

    virtual void Update(const sf::Time& deltaTime) = 0;

    virtual void Draw() = 0;

    virtual void Pause() {};

    virtual void Start() {};
};

struct StateMan {
    std::stack<State*> m_stateStack;
    State* m_newState;

    bool m_add;
    bool m_replace;
    bool m_remove;

    StateMan() {
        // m_newState = new State;
        m_add = false;
        m_replace = false;
        m_remove = false;
    }

    ~StateMan() {
    }

    void Add(State* toAdd, bool replace = false) {
        m_add = true;
        m_newState = toAdd;
        m_replace = replace;
    }

    void PopCurrent() {
        m_remove = true;
    }

    void ProcessStateChange() {
        if (m_remove && !(m_stateStack.empty())) {
            m_stateStack.pop();

            if (!m_stateStack.empty()) {
                m_stateStack.top()->Start();
            }

            m_remove = false;
        }

        if (m_add) {
            if (m_replace && (!m_stateStack.empty())) {
                m_stateStack.pop();
                m_replace = false;
            }

            if (!m_stateStack.empty()) {
                m_stateStack.top()->Pause();
            }

            m_stateStack.push(m_newState);
            m_stateStack.top()->Init();
            m_stateStack.top()->Start();
            m_add = false;
        }
    }

    State* GetCurrent() {
        return m_stateStack.top();
    }
};

struct AssetMan {

    std::map<int, sf::Texture*> m_textures;
    std::map<int, sf::Font*> m_fonts;
    std::map<int, sf::SoundBuffer*> m_soundBuffers;

    void AddTexture(int id, const std::string& filePath, bool wantRepeated = false) {
        sf::Texture* texture = new sf::Texture;

        if (texture->loadFromFile(filePath)) {
            texture->setRepeated(wantRepeated);
            m_textures[id] = texture;
        }
        else {
            std::cout << "Error loading texture" << id << std::endl;
        }
    }

    void AddFont(int id, const std::string filePath) {
        sf::Font* font = new sf::Font;

        if (font->loadFromFile(filePath)) {
            m_fonts[id] = font;
        }
    }

    void AddSoundBuffer(int id, const std::string filePath) {
        sf::SoundBuffer* soundBuffer = new sf::SoundBuffer;

        if (soundBuffer->loadFromFile(filePath)) {
            m_soundBuffers[id] = soundBuffer;
        }
    }

    sf::Texture& GetTexture(int id) {
        return *m_textures[id];
    }

    sf::Font& GetFont(int id) {
        return *m_fonts[id];
    }

    sf::SoundBuffer& GetSoundBuffer(int id) {
        return *m_soundBuffers[id];
    }
};

struct CONT {
    AssetMan* m_assets;
    StateMan* m_states;
    sf::RenderWindow* m_window;

    CONT() {
        m_assets = new AssetMan;
        m_states = new StateMan;
        m_window = new sf::RenderWindow;
    }

    ~CONT() {
        delete m_assets;
        delete m_states;
        delete m_window;
    }
};

struct Snake {
    std::deque<sf::Sprite> body;
    directions d;
    CONT* m_context;

    Snake(CONT* context) {
        m_context = context;
    }

    ~Snake() {
    }

    void Init() {
        body.resize(3);
        float x = 5 * CELL_SIZE;
        for (auto& piece : body) {
            piece.setPosition({ x, 10 * CELL_SIZE });
            x -= CELL_SIZE;
        }
    }

    void Move(const sf::Vector2f& direction) {
        if (body[0].getPosition().x < 0) {
            body[0].setPosition(m_context->m_window->getSize().x, body[0].getPosition().y);
        }
        else if (body[0].getPosition().x > m_context->m_window->getSize().x) {
            body[0].setPosition(-CELL_SIZE, body[0].getPosition().y);
        }
        else if (body[0].getPosition().y < 0) {
            body[0].setPosition(body[0].getPosition().x, m_context->m_window->getSize().y);
        }
        else if (body[0].getPosition().y > m_context->m_window->getSize().y) {
            body[0].setPosition(body[0].getPosition().x, 0);
        }

        body.pop_back();
        auto new_head = body.front();
        new_head.setPosition(new_head.getPosition() + direction);
        body.push_front(new_head);
    }

    bool IsOn(const sf::Sprite& other) {
        return other.getGlobalBounds().intersects(body.front().getGlobalBounds());
    }

    void Grow(const sf::Vector2f& direction) {
        sf::Sprite newPiece;
        newPiece.setTexture(*(body.begin()->getTexture()));
        newPiece.setPosition(body.begin()->getPosition() + direction);
        body.push_front(newPiece);
    }

    bool IsSelfIntersecting() {
        bool flag = false;

        for (auto piece = body.begin() + 1; piece != body.end(); ++piece) {

            flag = IsOn(*piece);

            if (flag) {
                break;
            }
        }

        return flag;
    }

    void update_tail() {
        auto it = body.rbegin();
        auto relation = it->getPosition() - (it + 1)->getPosition();

        relation.x /= CELL_SIZE;
        relation.y /= CELL_SIZE;

        if (relation == d.UP) {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_UP));
        }
        else if (relation == d.DOWN) {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_DOWN));
        }
        else if (relation == d.RIGHT) {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_RIGHT));
        }
        else if (relation == d.LEFT) {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_LEFT));
        }
    }

    void update_head() {
        auto it = body.begin();
        sf::Vector2f relation = it->getPosition() - (it + 1)->getPosition();

        relation.x /= CELL_SIZE;
        relation.y /= CELL_SIZE;

        if (relation == d.UP) {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_UP));
        }
        else if (relation == d.DOWN) {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_DOWN));
        }
        else if (relation == d.RIGHT) {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_RIGHT));
        }
        else if (relation == d.LEFT) {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_LEFT));
        }
    }

    void update_body(std::deque<sf::Sprite>::iterator& peice) {
        auto pPeice = peice - 1;
        auto nPeice = peice + 1;

        auto pRelation = peice->getPosition() - pPeice->getPosition();
        auto nRelation = nPeice->getPosition() - peice->getPosition();

        pRelation.x /= CELL_SIZE;
        pRelation.y /= CELL_SIZE;
        nRelation.x /= CELL_SIZE;
        nRelation.y /= CELL_SIZE;

        if (nRelation.x == 0 && pRelation.x == 0) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_VERTICAL));
        }
        else if (nRelation.y == 0 && pRelation.y == 0) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_HORIZONTAL));
        }
        else if ((pRelation.y == 1 && nRelation.x == 1) || (pRelation.x == -1 && nRelation.y == -1)) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_RIGHT));
        }
        else if ((pRelation.y == 1 && nRelation.x == -1) || (pRelation.x == 1 && nRelation.y == -1)) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_LEFT));
        }
        else if ((pRelation.x == 1 && nRelation.y == 1) || (pRelation.y == -1 && nRelation.x == -1)) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_LEFT));
        }
        else if ((pRelation.y == -1 && nRelation.x == 1) || (pRelation.x == -1 && nRelation.y == 1)) {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_RIGHT));
        }
    }

    void draw() {

        for (auto it = body.begin(); it != body.end(); ++it) {
            if (it == body.begin()) {
                update_head();
            }
            else if (it == body.end() - 1) {
                update_tail();
            }
            else {
                update_body(it);
            }
            m_context->m_window->draw(*it);
        }
    }
};

struct GameOver : public State {
    CONT* m_context;
    sf::Text m_gameOverTitle;
    sf::Text m_MainMenuButton;

    sf::Text m_exitButton;

    bool m_isMainMenuButtonSelected;
    bool m_isMainMenuButtonPressed;


    bool m_isExitButtonSelected;
    bool m_isExitButtonPressed;

    bool m_isMouseOverMainMenuButton;
    bool m_isMouseOverExitButton;

    bool m_isMousePressed;

    GameOver(CONT* context) {
        m_context = context;
        m_isMainMenuButtonSelected = true;
        m_isMainMenuButtonPressed = false;

        m_isExitButtonSelected = false;
        m_isExitButtonPressed = false;

    }

    ~GameOver() {
    }

    void Init() {
        m_context->m_assets->AddFont(MAIN_FONT, "assets/fonts/Top Show.otf");

        // Title
        m_gameOverTitle.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_gameOverTitle.setString("Game Over");
        m_gameOverTitle.setOrigin(m_gameOverTitle.getLocalBounds().width / 2,
            m_gameOverTitle.getLocalBounds().height / 2);
        m_gameOverTitle.setPosition(m_context->m_window->getSize().x / 2.7, 2.5 * CELL_SIZE);
        m_gameOverTitle.setCharacterSize(70);


        // Main Menu Button
        m_MainMenuButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_MainMenuButton.setString("Main Menu");
        m_MainMenuButton.setOrigin(m_MainMenuButton.getLocalBounds().width / 2,
            m_MainMenuButton.getLocalBounds().height / 2);
        m_MainMenuButton.setPosition(m_context->m_window->getSize().x / 2.15,
            8 * CELL_SIZE);
        m_MainMenuButton.setCharacterSize(50);

        // Exit Button
        m_exitButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_exitButton.setString("Exit");
        m_exitButton.setOrigin(m_exitButton.getLocalBounds().width / 2,
            m_exitButton.getLocalBounds().height / 2);
        m_exitButton.setPosition(m_context->m_window->getSize().x / 2.15,
            m_context->m_window->getSize().y / 2 + 25.f);
        m_exitButton.setCharacterSize(50);
    }

    void ProcessInput() {
        sf::Event event;
        while (m_context->m_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_context->m_window->close();
            }
        }


        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < -50;

        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) > 50;

        m_isMouseOverMainMenuButton = m_MainMenuButton.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y);

        m_isMouseOverExitButton = m_exitButton.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y);

        m_isMousePressed =
            sf::Mouse::isButtonPressed(sf::Mouse::Right) && (m_isMouseOverMainMenuButton || m_isMouseOverExitButton);

        bool enter = sf::Keyboard::isKeyPressed(sf::Keyboard::Space) ||
            sf::Joystick::isButtonPressed(0, 2);

        if ((up || m_isMouseOverMainMenuButton) && !m_isMainMenuButtonSelected) {
            m_isMainMenuButtonSelected = true;
            m_isExitButtonSelected = false;
        }
        else if ((down || m_isMouseOverExitButton) && !m_isExitButtonSelected) {
            m_isMainMenuButtonSelected = false;
            m_isExitButtonSelected = true;
        }
        else if (enter || m_isMousePressed) {
            m_isMainMenuButtonPressed = false;
            m_isExitButtonPressed = false;

            if (m_isMainMenuButtonSelected) {
                m_isMainMenuButtonPressed = true;
            }
            else {
                m_isExitButtonPressed = true;
            }
        }
    }

    void Update(const sf::Time& deltaTime) {
        auto size = m_context->m_window->getSize();

        if (m_isMainMenuButtonSelected) {
            m_MainMenuButton.setFillColor(sf::Color::Red);
            m_MainMenuButton.setCharacterSize(60);
            m_MainMenuButton.setPosition(
                size.x / 2.5,
                m_MainMenuButton.getPosition().y
            );
            m_exitButton.setFillColor(sf::Color::White);
            m_exitButton.setCharacterSize(50);
        }
        else {
            m_exitButton.setFillColor(sf::Color::Red);
            m_exitButton.setCharacterSize(60);
            m_MainMenuButton.setPosition(size.x / 2.2, m_MainMenuButton.getPosition().y);
            m_MainMenuButton.setFillColor(sf::Color::White);
            m_MainMenuButton.setCharacterSize(50);
        }

        if (m_isMainMenuButtonPressed) {
            retry = true;
        }
        else if (m_isExitButtonPressed) {
            m_context->m_window->close();
        }
    }

    void Draw() {
        m_context->m_window->clear();

        m_context->m_window->draw(m_gameOverTitle);
        m_context->m_window->draw(m_MainMenuButton);
        m_context->m_window->draw(m_exitButton);

        m_context->m_window->display();
    }
};

struct PauseGame : public State {

    CONT* m_context;
    sf::Text m_pauseTitle;

    PauseGame(CONT* context) {
        m_context = context;
    }

    ~PauseGame() {
    }

    void Init() {
        // Title
        m_pauseTitle.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_pauseTitle.setString("Paused");
        m_pauseTitle.setOrigin(m_pauseTitle.getLocalBounds().width / 2,
            m_pauseTitle.getLocalBounds().height / 2);
        m_pauseTitle.setPosition(m_context->m_window->getSize().x / 2,
            m_context->m_window->getSize().y / 2);
    }

    void ProcessInput() {
        sf::Event event;
        while (m_context->m_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_context->m_window->close();
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) ||
            sf::Joystick::isButtonPressed(0, 9)) {
            m_context->m_states->PopCurrent();
        }
    }

    void Update(const sf::Time& deltaTime) {

    };

    void Draw() {
        m_context->m_window->draw(m_pauseTitle);
        m_context->m_window->display();
    };
};

struct GamePlay : public State {
    CONT* m_context;
    sf::Sprite m_grass;
    sf::Sprite m_food;
    Snake* snake;
    directions2 d;
    sf::Sound m_eatSound;
    GameOver* gameOver;

    PauseGame* pauseGame;

    sf::Text m_scoreText;
    int m_score;

    sf::Vector2f m_snakeDirection;
    sf::Time m_eleapsedTime;

    bool is_paused;

    GamePlay(CONT* context) {
        m_context = context;
        snake = new Snake(context);
        pauseGame = new PauseGame(context);
        gameOver = new GameOver(context);
        m_score = 0;
        m_snakeDirection = { 40.f, 0.f };
        m_eleapsedTime = sf::Time::Zero;
        is_paused = false;

        srand(time(nullptr));
    }

    ~GamePlay() {
        delete snake;
        delete pauseGame;
    }

    void Init() {
        m_context->m_assets->AddTexture(GRASS, "assets/img/grass.png", true);
        m_context->m_assets->AddTexture(FOOD, "assets/img/apple.png");

        // snake graphics
        m_context->m_assets->AddTexture(HEAD_UP, "assets/img/head_up.png");
        m_context->m_assets->AddTexture(HEAD_DOWN, "assets/img/head_down.png");
        m_context->m_assets->AddTexture(HEAD_RIGHT, "assets/img/head_right.png");
        m_context->m_assets->AddTexture(HEAD_LEFT, "assets/img/head_left.png");

        m_context->m_assets->AddTexture(TAIL_UP, "assets/img/tail_up.png");
        m_context->m_assets->AddTexture(TAIL_DOWN, "assets/img/tail_down.png");
        m_context->m_assets->AddTexture(TAIL_RIGHT, "assets/img/tail_right.png");
        m_context->m_assets->AddTexture(TAIL_LEFT, "assets/img/tail_left.png");

        m_context->m_assets->AddTexture(BODY_VERTICAL, "assets/img/body_vertical.png");
        m_context->m_assets->AddTexture(BODY_HORIZONTAL, "assets/img/body_horizontal.png");

        m_context->m_assets->AddTexture(BODY_TOP_RIGHT, "assets/img/body_topright.png");
        m_context->m_assets->AddTexture(BODY_TOP_LEFT, "assets/img/body_topleft.png");
        m_context->m_assets->AddTexture(BODY_BOTTOM_RIGHT, "assets/img/body_bottomright.png");
        m_context->m_assets->AddTexture(BODY_BOTTOM_LEFT, "assets/img/body_bottomleft.png");

        m_context->m_assets->AddSoundBuffer(EAT_SOUND, "assets/sounds/eat.wav");
        m_eatSound.setBuffer(m_context->m_assets->GetSoundBuffer(EAT_SOUND));

        m_grass.setTexture(m_context->m_assets->GetTexture(GRASS));
        m_grass.setTextureRect(m_context->m_window->getViewport(m_context->m_window->getDefaultView()));

        m_food.setTexture(m_context->m_assets->GetTexture(FOOD));
        m_food.setPosition(m_context->m_window->getSize().x / 2, m_context->m_window->getSize().y / 2);

        snake->Init();

        m_scoreText.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_scoreText.setString(std::to_string(m_score));
        m_scoreText.setPosition((CELL_NUM_X - 1) * CELL_SIZE, (CELL_NUM_Y - 1) * CELL_SIZE);
        m_scoreText.setStyle(sf::Text::Bold);
        m_scoreText.setCharacterSize(30);
    }

    void ProcessInput() {
        sf::Event event;
        while (m_context->m_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_context->m_window->close();
            }
        }

        sf::Vector2f newDirection = m_snakeDirection;
        auto head = snake->body[0].getPosition();

        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < -50;

        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) > 50;

        bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::X) < -50;

        bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::X) > 50;

        bool pause = sf::Keyboard::isKeyPressed(sf::Keyboard::P) ||
            sf::Joystick::isButtonPressed(0, 8);

        if (pause) {
            m_context->m_states->Add(pauseGame, false);
        }
        else if (up) {
            newDirection = d.UP;
        }
        else if (down) {
            newDirection = d.DOWN;
        }
        else if (left) {
            newDirection = d.LEFT;
        }
        else if (right) {
            newDirection = d.RIGHT;
        }

        bool flag = 0 <= head.x &&
            head.x <= m_context->m_window->getSize().x - CELL_SIZE &&
            0 <= head.y &&
            head.y <= m_context->m_window->getSize().y - CELL_SIZE;

        if ((std::abs(m_snakeDirection.x) != std::abs(newDirection.x) ||
            std::abs(m_snakeDirection.y) != std::abs(newDirection.y)) &&
            flag) {
            m_snakeDirection = newDirection;
        }
    }

    void Update(const sf::Time& deltaTime) {
        if (!is_paused) {
            m_eleapsedTime += deltaTime;

            if (m_eleapsedTime.asSeconds() > 0.1) {
                if (snake->IsOn(m_food)) {
                    m_eatSound.play();
                    snake->Grow(m_snakeDirection);

                    int x = 0, y = 0;
                    x = std::clamp<int>(rand() % CELL_NUM_X, 0,
                        CELL_NUM_X - 1);
                    y = std::clamp<int>(rand() % CELL_NUM_Y, 0,
                        CELL_NUM_Y - 1);

                    m_food.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                    m_score += 1;
                    if (m_score > 99) {
                        m_scoreText.setCharacterSize(20);
                    }
                    m_scoreText.setString(std::to_string(m_score));
                }
                else {
                    snake->Move(m_snakeDirection);
                }

                if (snake->IsSelfIntersecting()) {
                    m_context->m_states->Add(gameOver, true);
                }

                m_eleapsedTime = sf::Time::Zero;
            }
        }
    }

    void Draw() {
        m_context->m_window->clear();
        m_context->m_window->draw(m_grass);
        m_context->m_window->draw(m_food);
        snake->draw();
        m_context->m_window->draw(m_scoreText);
        m_context->m_window->display();
    }

    void Pause() {
        is_paused = true;
    }

    void Start() {
        is_paused = false;
    }
};

struct CONNECT4 : public State {
    CONT* m_context;

    int ROWS = 6;
    int COLS = 7;
    int square_size = 100;
    int width = COLS * square_size;
    int height = (ROWS + 1) * square_size;
    int turn = 1; // Player 1 starts first
    float radius = 45;
    int col;
    bool isMousePressed = false;
    bool m_isPaused = false;

    bool game_over = false;

    int board_data[6][7];

    // 0 = empty
    // 1 = red
    // 2 = yellow

    sf::CircleShape redCircle;
    sf::CircleShape yellowCircle;
    sf::CircleShape blackCircle;
    sf::CircleShape cursorCircle;

    sf::RectangleShape board;

    CONNECT4(CONT* context) {
        m_context = context;
        memset(board_data, 0, sizeof(board_data));

        m_context->m_window->create(sf::VideoMode(width, height), "Connect 4");
    }

    void Init() {
        col = -1;
        redCircle.setRadius(radius);
        redCircle.setFillColor(sf::Color::Red);
        redCircle.setOrigin(radius, radius);

        yellowCircle.setRadius(radius);
        yellowCircle.setFillColor(sf::Color::Yellow);
        yellowCircle.setOrigin(radius, radius);

        blackCircle.setRadius(radius);
        blackCircle.setFillColor(sf::Color::Black);
        blackCircle.setOrigin(radius, radius);

        cursorCircle.setRadius(radius);
        cursorCircle.setFillColor(sf::Color(128, 128, 128, 128));
        cursorCircle.setOrigin(radius, radius);
    }

    void ProcessInput() {
        if (!m_isPaused) {
            sf::Event event{};
            while (m_context->m_window->pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    m_context->m_window->close();
                }

                if (event.type == sf::Event::MouseButtonPressed) {
                    if (game_over) {
                        continue;
                    }

                    int x = event.mouseButton.x;
                    col = x / square_size;
                    isMousePressed = true;
                }
            }
        }
    }

    void Update(const sf::Time& deltaTime) {
        if (isMousePressed)
            if (is_valid_move()) {
                make_move();
                draw_board();

                if (check_game_over()) {
                    game_over = true;
                }
                else {
                    switch_turns();
                }
            }

        isMousePressed = false;
        col = -1;


    }

    void Draw() {
        m_context->m_window->clear();
        draw_board();
        if (game_over) {
            display_result();
        }
        m_context->m_window->display();

    }

    void Pause() {
        m_isPaused = true;
    }

    void Start() {
        m_isPaused = false;
    }

    // Function to draw the game board and pieces
    void draw_board() {
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                sf::RectangleShape square;
                square.setSize(sf::Vector2f(100, 100));
                square.setFillColor(sf::Color::Blue);
                square.setPosition(c * square_size, r * square_size + square_size);
                m_context->m_window->draw(square);

                sf::CircleShape target;
                if (board_data[r][c] == 0) {
                    target = blackCircle;
                }
                else if (board_data[r][c] == 1) {
                    target = redCircle;
                }
                else if (board_data[r][c] == 2) {
                    target = yellowCircle;
                }

                target.setPosition(c * square_size + square_size / 2, r * square_size + 1.5 * square_size);
                m_context->m_window->draw(target);

                sf::Vector2i mousePos = sf::Mouse::getPosition(*m_context->m_window);
                col = mousePos.x / square_size;

                if (turn == 1) {
                    cursorCircle.setFillColor(sf::Color::Red);
                }
                else {
                    cursorCircle.setFillColor(sf::Color::Yellow);
                }

                cursorCircle.setPosition((col + 0.5) * square_size, square_size / 2);
                m_context->m_window->draw(cursorCircle);
            }
        }
    }

    // Function to check if a move is valid
    bool is_valid_move() {
        return 0 <= col && col < COLS;

    }

    // Function to make a move and update the game board
    void make_move() {
        for (int r = ROWS - 1; r >= 0; r--) {
            if (board_data[r][col] == 0) {
                board_data[r][col] = turn;
                break;
            }
        }
    }

    // Function to check if the game has ended
    bool check_game_over() {
        // Check for horizontal win
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                if (board_data[r][c] == turn &&
                    board_data[r][c + 1] == turn &&
                    board_data[r][c + 2] == turn &&
                    board_data[r][c + 3] == turn) {
                    return true;
                }
            }
        }

        // Check for vertical win
        for (int r = 0; r < ROWS - 3; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board_data[r][c] == turn &&
                    board_data[r + 1][c] == turn &&
                    board_data[r + 2][c] == turn &&
                    board_data[r + 3][c] == turn) {
                    return true;
                }
            }
        }

        // Check for diagonal win (top-left to bottom-right)
        for (int r = 0; r < ROWS - 3; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                if (board_data[r][c] == turn &&
                    board_data[r + 1][c + 1] == turn &&
                    board_data[r + 2][c + 2] == turn &&
                    board_data[r + 3][c + 3] == turn) {
                    return true;
                }
            }
        }

        // Check for diagonal win (bottom-left to top-right)
        for (int r = 3; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                if (board_data[r][c] == turn &&
                    board_data[r - 1][c + 1] == turn &&
                    board_data[r - 2][c + 2] == turn &&
                    board_data[r - 3][c + 3] == turn) {
                    return true;
                }
            }
        }

        // Check for a tie
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board_data[r][c] == 0) {
                    return false;
                }
            }
        }

        // If none of the above conditions are met, the game is still in progress
        return false;
    }

    // Function to switch turns
    void switch_turns() {
        if (turn == 1) {
            turn = 2;
        }
        else {
            turn = 1;
        }
    }

    // Function to display the winner or a tie message
    void display_result() {
        sf::Font font;
        font = m_context->m_assets->GetFont(MAIN_FONT);

        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(60);
        text.setFillColor(sf::Color::White);
        text.setStyle(sf::Text::Bold);

        if (check_game_over()) {
            text.setString("Player " + std::to_string(turn) + " wins");
        }
        else {
            text.setString("It's a tie");
        }

        text.setPosition(5, 1);
        m_context->m_window->draw(text);
        m_context->m_window->display();

        sf::sleep(sf::seconds(3));
        auto* GAME_OVER = new GameOver(m_context);
        m_context->m_states->Add(GAME_OVER);
    }
};

struct HugeMenu : public State {

    CONT* m_context;
    GamePlay* gamePlay;
    sf::Text choose_game, snake_game, xo_game, connect4_game, created;
    sf::Texture snakeTex, xoTex, connect4Tex;
    sf::Sprite snake, xo, connect4;
    sf::Font font;

    bool m_isSnakeButtonSelected;
    bool m_isSnakeButtonPressed;


    bool m_isC4ButtonSelected;
    bool m_isC4ButtonPressed;

    bool m_isMouseOverSnakeButton;
    bool m_isMouseOverC4Button;
    bool m_isMousePressed;

    HugeMenu(CONT* context) {
        m_context = context;
        m_isSnakeButtonSelected = true;
        m_isC4ButtonSelected = false;
        m_isSnakeButtonPressed = false;
        m_isC4ButtonPressed = false;

        gamePlay = new GamePlay(m_context);
    }

    ~HugeMenu() {
        delete gamePlay;
    }

    void Init() {
        m_context->m_assets->AddFont(MAIN_FONT, "assets/fonts/Top Show.otf");

        font.loadFromFile("assets/fonts/Top Show.otf");

        choose_game.setFont(font);
        choose_game.setString("Choose Game");
        choose_game.setCharacterSize(60);
        choose_game.setFillColor(sf::Color::White);
        choose_game.setOrigin(choose_game.getLocalBounds().width / 2, choose_game.getLocalBounds().height / 2);
        choose_game.setPosition(m_context->m_window->getSize().x / 2.f, 40.f);
        choose_game.setStyle(sf::Text::Bold);

        created.setFont(font);
        created.setString("Created By Ra2s-Mal Team");
        created.setCharacterSize(25);
        created.setFillColor(sf::Color::White);
        created.setOrigin(created.getLocalBounds().width / 2, created.getLocalBounds().height / 2);
        created.setPosition(m_context->m_window->getSize().x / 2.f, 100.f);
        created.setStyle(sf::Text::Bold);

        snake_game.setFont(font);
        snake_game.setString("Snake Game");
        snake_game.setCharacterSize(35);
        snake_game.setFillColor(sf::Color::White);
        snake_game.setOrigin(snake_game.getLocalBounds().width / 2, snake_game.getLocalBounds().height / 2);
        snake_game.setPosition(
            m_context->m_window->getSize().x / 2.f,
            m_context->m_window->getSize().y / 2.f
        );
        snake_game.setStyle(sf::Text::Bold);

        connect4_game.setFont(font);
        connect4_game.setString("Connect4 Game");
        connect4_game.setCharacterSize(35);
        connect4_game.setFillColor(sf::Color::White);
        connect4_game.setOrigin(connect4_game.getLocalBounds().width / 2, connect4_game.getLocalBounds().height / 2);
        connect4_game.setPosition(
            m_context->m_window->getSize().x / 2.f,
            m_context->m_window->getSize().y / 1.25f);
        connect4_game.setStyle(sf::Text::Bold);

        snakeTex.loadFromFile("assets/img/snake.png");
        connect4Tex.loadFromFile("assets/img/connect4.png");

        snake.setTexture(snakeTex);
        snake.setScale(sf::Vector2f(0.2f, 0.2f));
        snake.setOrigin(snake.getLocalBounds().width / 2, snake.getLocalBounds().height / 2);
        snake.setPosition(
            sf::Vector2f(
                m_context->m_window->getSize().x / 2.f,
                m_context->m_window->getSize().y / 2.5f));

        connect4.setTexture(connect4Tex);
        connect4.setScale(sf::Vector2f(0.2f, 0.2f));
        connect4.setOrigin(connect4.getLocalBounds().width / 2, connect4.getLocalBounds().height / 2);
        connect4.setPosition(
            sf::Vector2f(
                m_context->m_window->getSize().x / 2.f,
                m_context->m_window->getSize().y / 1.4f));
    }


    void ProcessInput() {

        sf::Event event;
        while (m_context->m_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_context->m_window->close();
            }
        }

        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < -50;

        m_isMouseOverSnakeButton = snake_game.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y
        ) || snake.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y
        );


        m_isMouseOverC4Button = connect4_game.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y
        ) || connect4.getGlobalBounds().contains(
            sf::Mouse::getPosition(*m_context->m_window).x,
            sf::Mouse::getPosition(*m_context->m_window).y
        );

        m_isMousePressed =
            sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
            (m_isMouseOverSnakeButton || m_isMouseOverC4Button);

        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
            sf::Joystick::getAxisPosition(0, sf::Joystick::Y) > 50;
        bool enter = sf::Keyboard::isKeyPressed(sf::Keyboard::Return) ||
            sf::Joystick::isButtonPressed(0, 2);

        if ((up || m_isMouseOverSnakeButton) && !m_isSnakeButtonSelected) {
            m_isSnakeButtonSelected = true;
            m_isC4ButtonSelected = false;
        }
        else if ((down || m_isMouseOverC4Button) && !m_isC4ButtonSelected) {
            m_isC4ButtonSelected = true;
            m_isSnakeButtonSelected = false;

        }
        else if (enter || (m_isMousePressed)) {
            if (m_isSnakeButtonSelected) {
                m_isSnakeButtonPressed = true;
            }
            else if (m_isC4ButtonSelected) {
                m_isC4ButtonPressed = true;
            }
        }
    }


    void Update(const sf::Time& deltaTime) {
        if (m_isSnakeButtonSelected) {
            snake_game.setFillColor(sf::Color::Red);
            snake_game.setCharacterSize(40);
            snake.setScale(0.25f, 0.25f);
            xo_game.setFillColor(sf::Color::White);
            xo_game.setCharacterSize(35);
            xo.setScale(0.2f, 0.2f);
            connect4_game.setFillColor(sf::Color::White);
            connect4_game.setCharacterSize(35);
            connect4.setScale(0.2f, 0.2f);

        }
        else if (m_isC4ButtonSelected) {
            connect4_game.setFillColor(sf::Color::Red);
            connect4_game.setCharacterSize(40);
            connect4.setScale(0.25f, 0.25f);
            xo_game.setFillColor(sf::Color::White);
            xo_game.setCharacterSize(35);
            xo.setScale(0.2f, 0.2f);
            snake_game.setFillColor(sf::Color::White);
            snake_game.setCharacterSize(35);
            snake.setScale(0.2f, 0.2f);
        }
        if (m_isC4ButtonPressed) {
            IS_SNAKE = false;
            CONNECT4* game2 = new CONNECT4(m_context);
            m_context->m_states->Add(game2, true);
        }
        else if (m_isSnakeButtonPressed) {
            IS_SNAKE = true;
            GamePlay* game1 = new GamePlay(m_context);
            m_context->m_states->Add(game1, true);
        }
    }

    void Draw() {
        m_context->m_window->clear();
        m_context->m_window->draw(choose_game);

        m_context->m_window->draw(created);
        m_context->m_window->draw(snake_game);
        m_context->m_window->draw(connect4_game);
        m_context->m_window->draw(snake);
        m_context->m_window->draw(connect4);

        m_context->m_window->display();
    }

};

struct GAME {
    CONT* m_context;
    const sf::Time timePF = sf::seconds(1.f / 60.f);

    GAME() {
        m_context = new CONT;

        m_context->m_window->create(sf::VideoMode(CELL_NUM_X * CELL_SIZE, CELL_NUM_Y * CELL_SIZE), "RA2S MAL GAME HUB");
        m_context->m_window->setFramerateLimit(60);
        auto* mainMenu = new HugeMenu(m_context);
        m_context->m_states->Add(mainMenu);
    }

    ~GAME() {
        delete m_context;
    }

    void RUN() {
        sf::CircleShape shape(100.f);
        shape.setFillColor(sf::Color::Green);

        sf::Clock clock;
        sf::Time timeSinceLastFrame = sf::Time::Zero;

        while (m_context->m_window->isOpen()) {
            timeSinceLastFrame += clock.restart();

            while (timeSinceLastFrame > timePF) {
                timeSinceLastFrame -= timePF;

                if (retry) {
                    auto* mainMenu = new HugeMenu(m_context);
                    m_context->m_states->Add(mainMenu);
                    retry = false;
                }

                m_context->m_states->ProcessStateChange();
                m_context->m_states->GetCurrent()->ProcessInput();
                m_context->m_states->GetCurrent()->Update(timePF);
                m_context->m_states->GetCurrent()->Draw();
            }
        }
    }
};

int main() {

    GAME game;
    game.RUN();

    return 0;
}
