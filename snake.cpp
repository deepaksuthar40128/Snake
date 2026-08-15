#include <exception>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <queue>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fstream>
#define PIXEL_RATIO = "2:1" //WIDTH/HEIGHT
constexpr auto GET_SCREEN_RATIO()noexcept{
    struct {
        uint8_t height = 1;
        uint8_t width = 2;
    } s;
    return s;
}
std::random_device rd;
std::mt19937 gen(rd()); 
void enableRawMode()
{
    termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}
std::atomic<char> entered_dir;
std::atomic<bool> gameOver{false};
namespace Game
{
    enum class Direction
    {
        UP,
        LEFT,
        RIGHT,
        DOWN
    };

    class Snake
    {
        Direction dir = Direction::RIGHT;
        std::deque<std::pair<uint32_t, uint32_t>> position;
        uint32_t bheight, bwidth;

    public:
        std::ofstream debug;
        Snake(uint32_t height, uint32_t width) : bheight(height), bwidth(width)
        {
            for(int i=0;i<width/3;i++)
                this->position.push_front({i, 0});
            this->debug = std::ofstream("debug.log");
        };
        const std::deque<std::pair<uint32_t, uint32_t>> &getPosition() const noexcept
        {
            return position;
        }
        bool move(Direction d,std::vector<std::pair<uint32_t,uint32_t>>&foods)
        {
            this->dir = d;
            auto current_head = this->position.front();
            auto new_head = this->_getHeadPosition(current_head, d);
            auto old_head_ptr = this->position.begin();
            old_head_ptr++;
            this->debug<<"[NEW_HEAD] "<<new_head.first<<' '<<new_head.second<<" [OLD_HEAD] "<<current_head.first<<' '<<current_head.second<<" [OLD_HEAD2] "<<old_head_ptr->first<<' '<<old_head_ptr->second<<std::endl;
            if(old_head_ptr!=this->position.end()){
                if(new_head==*old_head_ptr)return false;
            }
            bool removeTail = true;
            for(auto it = foods.begin();it!=foods.end();it++){
                if(it->first==new_head.first && it->second == new_head.second){
                    foods.erase(it);
                    removeTail=false;
                    break;
                }
            }
            if(removeTail)
                this->position.pop_back();
            this->_validatePosition(new_head);
            this->position.push_front(new_head);
            return true;
        }

    private:
        std::pair<uint32_t, uint32_t> _getHeadPosition(std::pair<uint32_t, uint32_t> &curr, Direction d) noexcept
        {
            std::pair<uint32_t, uint32_t> new_head = curr;
            switch (d)
            {
            case Direction::UP:
                if (new_head.second == 0)
                    new_head.second = this->bheight - 1;
                else
                    new_head.second -= 1;
                break;
            case Direction::DOWN:
                new_head.second += 1;
                break;
            case Direction::LEFT:
                if (new_head.first == 0)
                    new_head.first = this->bwidth - 1;
                else
                    new_head.first -= 1;
                break;
            case Direction::RIGHT:
                new_head.first += 1;
                break;
            default:
                break;
            }
            this->debug << "bwidth: " << this->bwidth << " " << "bheight " << this->bheight << " Head First: " << new_head.first << " Head second: " << new_head.second << std::endl;
            new_head.first = (new_head.first + this->bwidth) % this->bwidth;
            new_head.second = (new_head.second + this->bheight) % this->bheight;
            return new_head;
        }
        void _validatePosition(std::pair<uint32_t, uint32_t> &pos) const
        {
            for (auto &p : this->position)
            {
                if (p.first == pos.first && p.second == pos.second)
                {
                    throw std::runtime_error("Game Over!");
                }
            }
        }
    };

    class Board
    {
        uint32_t height, width;
        std::string symbol = " ";
        std::string snakeSymbol = "█";
        std::string boundarySymbol = "█";
        std::string foodSymbol = "●";
        std::vector<std::pair<uint32_t,uint32_t>>foodPoints;
    public:
        Board(uint32_t height, uint32_t width) : height(height), width(width) {};
        void draw(const std::unique_ptr<Snake> &snake,bool drawSnake=true, bool drawFood=true, std::string msg="")
        {
            std::vector<std::pair<uint32_t, uint32_t>> sp;
            for (auto &p : snake->getPosition())
            {
                sp.push_back(p);
                snake->debug << "[POSITION] " << p.first << ' ' << p.second << std::endl;
            }
            std::sort(sp.begin(), sp.end(), [](const std::pair<uint32_t, uint32_t> &a, const std::pair<uint32_t, uint32_t> &b) -> bool
                      {
                if(a.second!=b.second)return a.second<b.second;
                return a.first<b.first; });
            std::sort(this->foodPoints.begin(), this->foodPoints.end(), [](const std::pair<uint32_t, uint32_t> &a, const std::pair<uint32_t, uint32_t> &b) -> bool
                      {
                if(a.second!=b.second)return a.second<b.second;
                return a.first<b.first; });
            int spindex = 0,foodIndex=0;
            for (int i = 0; i < this->width; i++)
                std::cout << boundarySymbol;
            std::cout << std::endl;
            for (int i = 0; i < this->height; i++)
            {
                std::cout << boundarySymbol;
                for (int j = 0; j < this->width; j++)
                {
                    if (drawSnake && spindex >= 0 && i == sp[spindex].second && j == sp[spindex].first)
                    {
                        if(spindex%2)
                            std::cout << "\033[1;96m" << snakeSymbol << "\033[0m";
                        else
                            std::cout << "\033[1;95m" << snakeSymbol << "\033[0m";
                        spindex++;
                        if (spindex == sp.size())
                            spindex = -1;
                        continue;
                    }
                    if (drawFood && foodIndex >= 0 && foodPoints.size()>0 && i == foodPoints[foodIndex].second && j == foodPoints[foodIndex].first)
                    {
                        std::cout << "\033[1;91m" << foodSymbol << "\033[0m";
                        foodIndex++;
                        if (foodIndex == foodPoints.size())
                            foodIndex = -1;
                        continue;
                    }
                    if(msg.size()>0){
                        if(i==this->height/2){
                            int msgOffset = msg.size()/2;
                            int li = this->width/2-msgOffset;
                            int ri = this->width/2+msgOffset;
                            int msgInd = j-li;
                            if(msgInd>=0&&j<=ri)
                                {std::cout<<msg[msgInd];continue;}
                        }
                    }
                    std::cout << symbol;
                }
                std::cout << boundarySymbol;
                std::cout << std::endl;
            }
            for (int i = 0; i < this->width; i++)
                std::cout << boundarySymbol;
            std::cout << std::endl;
            for (int i = 0; i < this->height + 2; i++)
                std::cout << "\033[A";
            for (int i = 0; i < this->width + 2; i++)
                std::cout << "\033[D";
        }

        void addFood(const std::pair<int,int>&point) noexcept{
            for(auto&it:this->foodPoints){
                if(it.first==point.first && it.second == point.second)return;
            }
            this->foodPoints.push_back(point);
        }
        std::vector<std::pair<uint32_t,uint32_t>>& getFood() noexcept{
            return this->foodPoints;
        }
    };

    class GameController
    {
        std::unique_ptr<Snake> snake;
        std::unique_ptr<Board> board;
        char lastValidDir = 'd';
    public:
        GameController(uint32_t height, uint32_t width)
        {
            this->snake = std::make_unique<Snake>(height, width);
            this->board = std::make_unique<Board>(height, width);
        }
        Direction getMoveDirection()
        {
            char lastInput = entered_dir.load();
            switch (lastInput)
            {
            case 'w':
                return Direction::UP;
                break;
            case 'a':
                return Direction::LEFT;
                break;
            case 'd':
                return Direction::RIGHT;
                break;
            case 's':
                return Direction::DOWN;
                break;

            default:
                return Direction::RIGHT;
            }
        }
        void start()
        {
            this->board->draw(this->snake);
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                try
                {
                    bool valid = this->snake->move(this->getMoveDirection(),this->board->getFood());
                    if(!valid)
                        entered_dir.store(this->lastValidDir);
                    else
                        this->lastValidDir = entered_dir.load();
                    if(this->board->getFood().size()<2){
                        std::uniform_int_distribution<uint32_t> dist(1,59);
                        std::uniform_int_distribution<uint32_t> dist2(1,29);
                        std::pair<uint32_t,uint32_t>p={dist(gen),dist2(gen)};
                        this->board->addFood(p);
                    }
                }
                catch (const std::exception e)
                {
                    this->board->draw(this->snake,false,false,"Game Over");
                    gameOver.store(true);
                    close(STDIN_FILENO); 
                    break;
                }
                this->board->draw(this->snake);
            }
        }
    };

    void GameRunner()
    {
        Game::GameController *b = new Game::GameController(30, 60);
        b->start();
    }
};

int main()
{
    enableRawMode();
    std::thread t(Game::GameRunner);
    while (!gameOver.load())
    {
        char c;
        std::cin.get(c);
        if (c != '\n')
            entered_dir.store(c);
    }
    t.join();
}
