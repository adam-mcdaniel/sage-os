/*
Programmer: Gaddiel Morales
Program: Snake
Purpose: Showcase a user app making use of input device and GPU
*/

/*TODO list
    * draw board on rectangle
    * send rectangle to the OS
    * timer for movement (optional: scale with difficulty)
*/

#include <list>
#include <random>

enum collisionType {none, 
        food, 
        self
    };

enum Direction{
    up, 
    right,
    down,
    left,
    stop
};

class World{
    private:
        int height = 50, width = 50, apple_lim = 3;

        //used to find items
        SnakeHead* snake = nullptr;
        std::list<Apple> apples;

    public:
        World(){

        }

        int getHeight(){
            return this->height;
        }
        int getWidth(){
            return this->width;
        }
        void setAppleLimit(int new_lim){
            this->apple_lim = new_lim;
        }

        int checkCollisions(){
           

            //check apples
            for(Apple& apple : apples){
                if(snake->getX() == apple.x && snake->getY() == apple.y){
                    return collisionType(food);
                }
            }

            //check self
            SnakeSegment* curr_segment = snake->getNext();
            while(curr_segment){
                if(snake->getX() == curr_segment->getX() && snake->getY() == curr_segment->getY())
                return collisionType(self);
            }

            return collisionType(none);

        }

        void cleanBoard(){
            apples.clear();
            this->snake->kill();
        }

        void spawnSnake(){
            int x = std::rand() % width;
            int y = std::rand() % height;

            this->snake = new SnakeHead(x,y,width,height);

        }

        void spawnApples(){
            while(apples.size() < apple_lim){
                int x = std::rand() % width;
                int y = std::rand() % height;
                bool valid = true;
                for(Apple& apple : apples){
                    if(apple.x == x && apple.y == y) valid = false;
                }
                if(valid){
                    apples.push_back(Apple(x,y));
                }
            }

        }

        SnakeHead* getSnake(){
            return this->snake;
        }
};

class Apple{
    public:
        int x, y;
        Apple(int x,int y){
            this->x = x;
            this->y = y;
        }
};

class SnakeSegment{
    protected:
        int x, y;
        bool spawned = true;   //used to tell if this segment was made on the current timestep
        //next segment is a linked segment closer to the tail, previous is a linked segment closer to the head
        SnakeSegment* next = nullptr;
        SnakeSegment* prev = nullptr;

    public:
        SnakeSegment(){
            this->x = 0;
            this->y = 0;
        }

        SnakeSegment(int x, int y){
            this->x = x;
            this->y = y;
        }

        SnakeSegment* getNext(){
            return this->next;
        }

        SnakeSegment* getPrev(){
            return this->prev;
        }

        int getX(){
            return this->x;
        }

        int getY(){
            return this->y;
        }

        //inch the snake forward, call from SnakeHead
        void moveSegment(){
            //move trailing segment to this position
            if(this->next) this->next->moveSegment();
            //move this segment to up towards head
            this->y = prev->y;
            this->x = prev->x;
        }

        //use when eating food
        void addSegment(){
            if(this->getNext()){
                this->getNext()->addSegment();
            }
            else{
                this->next = new SnakeSegment(this->x,this->y);
            }
        }

        //deletes this and trailing segments
        void kill(){
            if(this->getNext()) this->getNext()->kill();

            delete this;
            
        }

};

class SnakeHead : public SnakeSegment{
    private:
        int x_lim = 50, y_lim = 50;
    public: 

        

        Direction direction = Direction::stop;

        SnakeHead(int x, int y, int x_lim, int y_lim){
            this->x = x;
            this->y = y;
            this->next = nullptr;
            this->prev = nullptr;
            this->x_lim = x_lim;
            this->y_lim = y_lim;
        }
        
        void moveSnake(){
            //don't move freshly spawned segments
            if(spawned){
                spawned = false;
                return;
            }

            //move trailing segment to this position
            if(this->next) this->next->moveSegment();

            //move this segment
            switch(this->direction){
                case Direction::up:
                    this->y = ((this->y + 1) % y_lim);
                    break;
                case Direction::down:
                    this->y = ((this->y - 1 + y_lim) % y_lim);
                    break;
                case Direction::left:
                    this->x = ((this->x - 1 + x_lim) % x_lim);
                    break;
                case Direction::right:
                    this->x = ((this->x + 1) % x_lim);
                    break;
            }

        }
};



int main(){
    //create board
    World snake_game = World();

    //spawn snake
    snake_game.spawnSnake();
            
    //spawn food
    snake_game.spawnApples();

    bool alive = true;
    while(alive){

        //movesnake
        snake_game.getSnake()->moveSnake();

        //check if eating food or eating self

        int collision = snake_game.checkCollisions();

        if(collision == collisionType(self)){
        
            //wait until keypress

            //clean board

            //spawn snake
            
            //spawn food
        }

        //wait according to timer

        //(optional) update timer

    }

}

