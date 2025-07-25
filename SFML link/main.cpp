#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <stack>
#include <map>
#include <string>
#include <deque>
#include <iostream>
#include <SFML/Audio.hpp>

const int CELL_SIZE = 40;
int CELL_NUM_X = 20;
int CELL_NUM_Y = 20;

bool retry = false;

struct directions
{
    sf::Vector2f UP{0, -1};
    sf::Vector2f DOWN{0, 1};
    sf::Vector2f RIGHT{1, 0};
    sf::Vector2f LEFT{-1, 0};
};

struct directions2
{
    sf::Vector2f UP{0, -40};
    sf::Vector2f DOWN{0, 40};
    sf::Vector2f RIGHT{40, 0};
    sf::Vector2f LEFT{-40, 0};
};

enum assetsID
{
    MAIN_FONT = 0,
    GRASS,
    FOOD,
    EAT_SOUND,

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

struct State
{
    State(){};

    virtual ~State(){};

    virtual void Init() = 0;

    virtual void ProcessInput() = 0;

    virtual void Update(const sf::Time &deltaTime) = 0;

    virtual void Draw() = 0;

    virtual void Pause(){};

    virtual void Start(){};
};

struct StateMan
{
    std::stack<State *> m_stateStack;
    State *m_newState;

    bool m_add;
    bool m_replace;
    bool m_remove;

    StateMan()
    {
        // m_newState = new State;
        m_add = false;
        m_replace = false;
        m_remove = false;
    }

    ~StateMan()
    {
    }

    void Add(State *toAdd, bool replace = false)
    {
        m_add = true;
        m_newState = toAdd;
        m_replace = replace;
    }

    void PopCurrent()
    {
        m_remove = true;
    }

    void ProcessStateChange()
    {
        if (m_remove && !(m_stateStack.empty()))
        {
            m_stateStack.pop();

            if (!m_stateStack.empty())
            {
                m_stateStack.top()->Start();
            }

            m_remove = false;
        }

        if (m_add)
        {
            if (m_replace && (!m_stateStack.empty()))
            {
                m_stateStack.pop();
                m_replace = false;
            }

            if (!m_stateStack.empty())
            {
                m_stateStack.top()->Pause();
            }

            m_stateStack.push(m_newState);
            m_stateStack.top()->Init();
            m_stateStack.top()->Start();
            m_add = false;
        }
    }

    State *GetCurrent()
    {
        return m_stateStack.top();
    }
};

struct AssetMan
{

    std::map<int, sf::Texture *> m_textures;
    std::map<int, sf::Font *> m_fonts;
    std::map<int, sf::SoundBuffer *> m_soundBuffers;

    void AddTexture(int id, const std::string &filePath, bool wantRepeated = false)
    {
        sf::Texture *texture = new sf::Texture;

        if (texture->loadFromFile(filePath))
        {
            texture->setRepeated(wantRepeated);
            m_textures[id] = texture;
        }
        else
        {
            std::cout << "Error loading texture" << id << std::endl;
        }
    }

    void AddFont(int id, const std::string filePath)
    {
        sf::Font *font = new sf::Font;

        if (font->loadFromFile(filePath))
        {
            m_fonts[id] = font;
        }
    }

    void AddSoundBuffer(int id, const std::string filePath)
    {
        sf::SoundBuffer *soundBuffer = new sf::SoundBuffer;

        if (soundBuffer->loadFromFile(filePath))
        {
            m_soundBuffers[id] = soundBuffer;
        }
    }

    sf::Texture &GetTexture(int id)
    {
        return *m_textures[id];
    }

    sf::Font &GetFont(int id)
    {
        return *m_fonts[id];
    }

    sf::SoundBuffer &GetSoundBuffer(int id)
    {
        return *m_soundBuffers[id];
    }
};

struct CONT
{
    AssetMan *m_assets;
    StateMan *m_states;
    sf::RenderWindow *m_window;

    CONT()
    {
        m_assets = new AssetMan;
        m_states = new StateMan;
        m_window = new sf::RenderWindow;
    }

    ~CONT()
    {
        delete m_assets;
        delete m_states;
        delete m_window;
    }
};

struct Snake
{
    std::deque<sf::Sprite> body;
    directions d;
    CONT *m_context;

    Snake(CONT *context)
    {
        m_context = context;
    }

    ~Snake()
    {
    }

    void Init()
    {
        body.resize(3);
        float x = 5 * CELL_SIZE;
        for (auto &piece : body)
        {
            piece.setTexture(m_context->m_assets->GetTexture(BODY_VERTICAL));
            piece.setPosition({x, 10 * CELL_SIZE});
            x -= CELL_SIZE;
        }
    }

    void Move(const sf::Vector2f &direction)
    {
        if (body[0].getPosition().x < 0)
        {
            body[0].setPosition(m_context->m_window->getSize().x, body[0].getPosition().y);
        }
        else if (body[0].getPosition().x > m_context->m_window->getSize().x)
        {
            body[0].setPosition(-CELL_SIZE, body[0].getPosition().y);
        }
        else if (body[0].getPosition().y < 0)
        {
            body[0].setPosition(body[0].getPosition().x, m_context->m_window->getSize().y);
        }
        else if (body[0].getPosition().y > m_context->m_window->getSize().y)
        {
            body[0].setPosition(body[0].getPosition().x, 0);
        }

        body.pop_back();
        auto new_head = body.front();
        new_head.setPosition(new_head.getPosition() + direction);
        body.push_front(new_head);
    }

    bool IsOn(const sf::Sprite &other)
    {
        return other.getGlobalBounds().intersects(body.front().getGlobalBounds());
    }

    void Grow(const sf::Vector2f &direction)
    {
        sf::Sprite newPiece;
        newPiece.setTexture(*(body.begin()->getTexture()));
        newPiece.setPosition(body.begin()->getPosition() + direction);
        body.push_front(newPiece);
    }

    bool IsSelfIntersecting()
    {
        bool flag = false;

        for (auto piece = body.begin() + 1; piece != body.end(); ++piece)
        {

            flag = IsOn(*piece);

            if (flag)
            {
                break;
            }
        }

        return flag;
    }

    void update_tail()
    {
        auto it = body.rbegin();
        auto relation = it->getPosition() - (it + 1)->getPosition();

        relation.x /= CELL_SIZE;
        relation.y /= CELL_SIZE;

        if (relation == d.UP)
        {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_UP));
        }
        else if (relation == d.DOWN)
        {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_DOWN));
        }
        else if (relation == d.RIGHT)
        {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_RIGHT));
        }
        else if (relation == d.LEFT)
        {
            body.rbegin()->setTexture(m_context->m_assets->GetTexture(TAIL_LEFT));
        }
    }

    void update_head()
    {
        auto it = body.begin();
        sf::Vector2f relation = it->getPosition() - (it + 1)->getPosition();

        relation.x /= CELL_SIZE;
        relation.y /= CELL_SIZE;

        if (relation == d.UP)
        {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_UP));
        }
        else if (relation == d.DOWN)
        {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_DOWN));
        }
        else if (relation == d.RIGHT)
        {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_RIGHT));
        }
        else if (relation == d.LEFT)
        {
            body.begin()->setTexture(m_context->m_assets->GetTexture(HEAD_LEFT));
        }
    }

    void update_body(std::deque<sf::Sprite>::iterator &peice)
    {
        auto pPeice = peice - 1;
        auto nPeice = peice + 1;

        auto pRelation = peice->getPosition() - pPeice->getPosition();
        auto nRelation = peice->getPosition() - nPeice->getPosition();

        pRelation.x /= CELL_SIZE;
        pRelation.y /= CELL_SIZE;
        nRelation.x /= CELL_SIZE;
        nRelation.y /= CELL_SIZE;

        if (pRelation.x == 0 && nRelation.x == 0)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_VERTICAL));
        }
        else if (pRelation.y == 0 && nRelation.y == 0)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_HORIZONTAL));
        }
        else if (pRelation.x == 1 && nRelation.y == 1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_LEFT));
        }
        else if (pRelation.x == 1 && nRelation.y == -1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_LEFT));
        }
        else if (pRelation.x == -1 && nRelation.y == 1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_RIGHT));
        }
        else if (pRelation.x == -1 && nRelation.y == -1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_RIGHT));
        }
        else if (pRelation.y == 1 && nRelation.x == -1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_RIGHT));
        }
        else if (pRelation.y == 1 && nRelation.x == 1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_TOP_LEFT));
        }
        else if (pRelation.y == -1 && nRelation.x == -1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_RIGHT));
        }
        else if (pRelation.y == -1 && nRelation.x == 1)
        {
            peice->setTexture(m_context->m_assets->GetTexture(BODY_BOTTOM_LEFT));
        }
    }

    void draw()
    {

        for (auto it = body.begin(); it != body.end(); ++it)
        {
            if (it == body.begin())
            {
                update_head();
            }
            else if (it == body.end() - 1)
            {
                update_tail();
            }
            else
            {
                update_body(it);
            }
            m_context->m_window->draw(*it);
        }
    }
};

struct GameOver : public State
{
    CONT *m_context;
    sf::Text m_gameOverTitle;
    sf::Text m_retryButton;
    sf::Text m_exitButton;

    bool m_isRetryButtonSelected;
    bool m_isRetryButtonPressed;

    bool m_isExitButtonSelected;
    bool m_isExitButtonPressed;

    GameOver(CONT *context)
    {
        m_context = context;
        m_isRetryButtonSelected = true;
        m_isRetryButtonPressed = false;
        m_isExitButtonSelected = false;
        m_isExitButtonPressed = false;
    }

    ~GameOver()
    {
    }

    void Init()
    {
        // Title
        m_gameOverTitle.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_gameOverTitle.setString("Game Over");
        m_gameOverTitle.setOrigin(m_gameOverTitle.getLocalBounds().width / 2,
                                  m_gameOverTitle.getLocalBounds().height / 2);
        m_gameOverTitle.setPosition(m_context->m_window->getSize().x / 2,
                                    m_context->m_window->getSize().y / 2 - 150.f);

        // Play Button
        m_retryButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_retryButton.setString("Retry");
        m_retryButton.setOrigin(m_retryButton.getLocalBounds().width / 2,
                                m_retryButton.getLocalBounds().height / 2);
        m_retryButton.setPosition(m_context->m_window->getSize().x / 2,
                                  m_context->m_window->getSize().y / 2 - 25.f);
        m_retryButton.setCharacterSize(20);

        // Exit Button
        m_exitButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_exitButton.setString("Exit");
        m_exitButton.setOrigin(m_exitButton.getLocalBounds().width / 2,
                               m_exitButton.getLocalBounds().height / 2);
        m_exitButton.setPosition(m_context->m_window->getSize().x / 2,
                                 m_context->m_window->getSize().y / 2 + 25.f);
        m_exitButton.setCharacterSize(20);
    }

    void ProcessInput()
    {
        sf::Event event;
        while (m_context->m_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_context->m_window->close();
            }
        }

        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
                  sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < -50;

        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
                    sf::Joystick::getAxisPosition(0, sf::Joystick::Y) > 50;

        bool enter = sf::Keyboard::isKeyPressed(sf::Keyboard::Return) ||
                     sf::Joystick::isButtonPressed(0, 2);

        if (up && !m_isRetryButtonSelected)
        {
            m_isRetryButtonSelected = true;
            m_isExitButtonSelected = false;
        }
        else if (down && !m_isExitButtonSelected)
        {
            m_isRetryButtonSelected = false;
            m_isExitButtonSelected = true;
        }
        else if (enter)
        {
            m_isRetryButtonPressed = false;
            m_isExitButtonPressed = false;

            if (m_isRetryButtonSelected)
            {
                m_isRetryButtonPressed = true;
            }
            else
            {
                m_isExitButtonPressed = true;
            }
        }
    }

    void Update(const sf::Time &deltaTime)
    {

        if (m_isRetryButtonSelected)
        {
            m_retryButton.setFillColor(sf::Color::Red);
            m_exitButton.setFillColor(sf::Color::White);
        }
        else
        {
            m_exitButton.setFillColor(sf::Color::Red);
            m_retryButton.setFillColor(sf::Color::White);
        }

        if (m_isRetryButtonPressed)
        {
            // m_context->m_states->Add(, true);
            retry = true;
        }
        else if (m_isExitButtonPressed)
        {
            m_context->m_window->close();
        }
    }

    void Draw()
    {
        m_context->m_window->clear();
        m_context->m_window->draw(m_gameOverTitle);
        m_context->m_window->draw(m_retryButton);
        m_context->m_window->draw(m_exitButton);
        m_context->m_window->display();
    }
};

struct PauseGame : public State
{

    CONT *m_context;
    sf::Text m_pauseTitle;

    PauseGame(CONT *context)
    {
        m_context = context;
    }

    ~PauseGame()
    {
    }

    void Init()
    {
        // Title
        m_pauseTitle.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_pauseTitle.setString("Paused");
        m_pauseTitle.setOrigin(m_pauseTitle.getLocalBounds().width / 2,
                               m_pauseTitle.getLocalBounds().height / 2);
        m_pauseTitle.setPosition(m_context->m_window->getSize().x / 2,
                                 m_context->m_window->getSize().y / 2);
    }

    void ProcessInput()
    {
        sf::Event event;
        while (m_context->m_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_context->m_window->close();
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) ||
            sf::Joystick::isButtonPressed(0, 8))
        {
            m_context->m_states->PopCurrent();
        }
    }

    void Update(const sf::Time &deltaTime){

    };

    void Draw()
    {
        m_context->m_window->draw(m_pauseTitle);
        m_context->m_window->display();
    };
};

struct GamePlay : State
{
    CONT *m_context;
    sf::Sprite m_grass;
    sf::Sprite m_food;
    Snake *snake;
    directions2 d;
    sf::Sound m_eatSound;
    GameOver *gameOver;

    PauseGame *pauseGame;

    sf::Text m_scoreText;
    int m_score;

    sf::Vector2f m_snakeDirection;
    sf::Time m_eleapsedTime;

    bool is_paused;

    GamePlay(CONT *context)
    {
        m_context = context;
        snake = new Snake(context);
        pauseGame = new PauseGame(context);
        gameOver = new GameOver(context);
        m_score = 0;
        m_snakeDirection = {40.f, 0.f};
        m_eleapsedTime = sf::Time::Zero;
        is_paused = false;

        srand(time(nullptr));
    }

    ~GamePlay()
    {
        delete snake;
        delete pauseGame;
    }

    void Init()
    {
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
        m_scoreText.setString("Score : " + std::to_string(m_score));
        m_scoreText.setCharacterSize(15);
    }

    void ProcessInput()
    {
        sf::Event event;
        while (m_context->m_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
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

        bool pause = sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) ||
                     sf::Joystick::isButtonPressed(0, 8);

        if (pause)
        {
            m_context->m_states->Add(pauseGame, false);
        }
        else if (up)
        {
            newDirection = d.UP;
        }
        else if (down)
        {
            newDirection = d.DOWN;
        }
        else if (left)
        {
            newDirection = d.LEFT;
        }
        else if (right)
        {
            newDirection = d.RIGHT;
        }

        bool flag = 0 <= head.x &&
                    head.x <= m_context->m_window->getSize().x - CELL_SIZE &&
                    0 <= head.y &&
                    head.y <= m_context->m_window->getSize().y - CELL_SIZE;

        if ((std::abs(m_snakeDirection.x) != std::abs(newDirection.x) ||
             std::abs(m_snakeDirection.y) != std::abs(newDirection.y)) &&
            flag)
        {
            m_snakeDirection = newDirection;
        }
    }

    void Update(const sf::Time &deltaTime)
    {
        if (!is_paused)
        {
            m_eleapsedTime += deltaTime;

            if (m_eleapsedTime.asSeconds() > 0.1)
            {
                if (snake->IsOn(m_food))
                {
                    m_eatSound.play();
                    snake->Grow(m_snakeDirection);

                    int x = 0, y = 0;
                    x = std::clamp<int>(rand() % CELL_NUM_X, 0,
                                        CELL_NUM_X - 1);
                    y = std::clamp<int>(rand() % CELL_NUM_Y, 0,
                                        CELL_NUM_Y - 1);

                    m_food.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                    m_score += 1;
                    m_scoreText.setString("Score : " + std::to_string(m_score));
                }
                else
                {
                    snake->Move(m_snakeDirection);
                }

                if (snake->IsSelfIntersecting())
                {
                    m_context->m_states->Add(gameOver, true);
                }

                m_eleapsedTime = sf::Time::Zero;
            }
        }
    }

    void Draw()
    {
        m_context->m_window->clear();
        m_context->m_window->draw(m_grass);
        m_context->m_window->draw(m_food);
        snake->draw();
        m_context->m_window->draw(m_scoreText);
        m_context->m_window->display();
    }

    void Pause()
    {
        is_paused = true;
    }

    void Start()
    {
        is_paused = false;
    }
};

struct MainMenu : public State
{
    CONT *m_context;
    GamePlay *gamePlay;
    sf::Text m_gameTitle;
    sf::Text m_playButton;
    sf::Text m_exitButton;

    bool m_isPlayButtonSelected;
    bool m_isPlayButtonPressed;

    bool m_isExitButtonSelected;
    bool m_isExitButtonPressed;

    MainMenu(CONT *context)
    {
        m_context = context;
        m_isPlayButtonSelected = true;
        m_isExitButtonSelected = false;
        m_isPlayButtonPressed = false;
        m_isExitButtonPressed = false;

        gamePlay = new GamePlay(m_context);
    }

    ~MainMenu()
    {
        delete gamePlay;
    }

    void Init()
    {
        m_context->m_assets->AddFont(MAIN_FONT, "assets/fonts/Courgette-Regular.ttf");

        // Title
        m_gameTitle.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_gameTitle.setString("Snake Game");
        m_gameTitle.setOrigin(m_gameTitle.getLocalBounds().width / 2,
                              m_gameTitle.getLocalBounds().height / 2);
        m_gameTitle.setPosition(m_context->m_window->getSize().x / 2,
                                m_context->m_window->getSize().y / 2 - 150.f);

        // Play Button
        m_playButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_playButton.setString("Play");
        m_playButton.setOrigin(m_playButton.getLocalBounds().width / 2,
                               m_playButton.getLocalBounds().height / 2);
        m_playButton.setPosition(m_context->m_window->getSize().x / 2,
                                 m_context->m_window->getSize().y / 2 - 25.f);
        m_playButton.setCharacterSize(20);

        // Exit Button
        m_exitButton.setFont(m_context->m_assets->GetFont(MAIN_FONT));
        m_exitButton.setString("Exit");
        m_exitButton.setOrigin(m_exitButton.getLocalBounds().width / 2,
                               m_exitButton.getLocalBounds().height / 2);
        m_exitButton.setPosition(m_context->m_window->getSize().x / 2,
                                 m_context->m_window->getSize().y / 2 + 25.f);
        m_exitButton.setCharacterSize(20);
    }

    void ProcessInput()
    {
        sf::Event event;
        while (m_context->m_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_context->m_window->close();
            }
        }
        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
                  sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < -50 ||
                  sf::Joystick::isButtonPressed(0, 10);
        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
                    sf::Joystick::getAxisPosition(0, sf::Joystick::Y) > 50;
        bool enter = sf::Keyboard::isKeyPressed(sf::Keyboard::Return) ||
                     sf::Joystick::isButtonPressed(0, 2);

        if (up && !m_isPlayButtonSelected)
        {
            m_isPlayButtonSelected = true;
            m_isExitButtonSelected = false;
        }
        else if (down && !m_isExitButtonSelected)
        {
            m_isPlayButtonSelected = false;
            m_isExitButtonSelected = true;
        }
        else if (enter)
        {
            if (m_isPlayButtonSelected)
            {
                m_isPlayButtonPressed = true;
            }
            else
            {
                m_isExitButtonPressed = true;
            }
        }
    }

    void Update(const sf::Time &deltaTime)
    {
        if (m_isPlayButtonSelected)
        {
            m_playButton.setFillColor(sf::Color::Red);
            m_exitButton.setFillColor(sf::Color::White);
        }
        else
        {
            m_exitButton.setFillColor(sf::Color::Red);
            m_playButton.setFillColor(sf::Color::White);
        }

        if (m_isPlayButtonPressed)
        {
            m_context->m_states->Add(gamePlay, true);
        }
        else if (m_isExitButtonPressed)
        {
            m_context->m_window->close();
        }
    }

    void Draw()
    {
        m_context->m_window->clear();
        m_context->m_window->draw(m_gameTitle);
        m_context->m_window->draw(m_playButton);
        m_context->m_window->draw(m_exitButton);
        m_context->m_window->display();
    }
};

struct GAME
{
    CONT *m_context;
    const sf::Time timePF = sf::seconds(1.f / 60.f);

    GAME()
    {
        m_context = new CONT;
        m_context->m_window->create(sf::VideoMode(CELL_NUM_X * CELL_SIZE, CELL_NUM_Y * CELL_SIZE), "RA2S MAL GAME HUB");

        MainMenu *mainMenu = new MainMenu(m_context);
        m_context->m_states->Add(mainMenu);
    }

    ~GAME()
    {
        delete m_context;
    }

    void RUN()
    {
        sf::CircleShape shape(100.f);
        shape.setFillColor(sf::Color::Green);

        sf::Clock clock;
        sf::Time timeSinceLastFrame = sf::Time::Zero;

        while (m_context->m_window->isOpen())
        {
            timeSinceLastFrame += clock.restart();

            while (timeSinceLastFrame > timePF)
            {
                timeSinceLastFrame -= timePF;

                if (retry)
                {
                    GamePlay *gamePlay = new GamePlay(m_context);
                    m_context->m_states->Add(gamePlay, true);
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

int main()
{
    GAME game;
    game.RUN();

    return 0;
}
