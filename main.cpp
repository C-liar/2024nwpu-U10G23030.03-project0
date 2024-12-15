#include <graphics.h>
#include <bits/stdc++.h>

/*
  (0,0)
     +--x--------------------+
     |                       |  A
     y                       |  |
     |                       | High
     |                       |  |
     |                       |  V
     +-----------------------+
           <--Width-->
*/

const int Width = 640;
const int High = 480;
const int Wmid = Width / 2;
const int Hmid = High / 2;
const int BufLen = 0x10000;
const int fps = 10;
const int timegap = 1000 / fps;

enum _toward { u = 0, r = 1, d = 2, l = 3 };
const int dy[] = {-1, 0, 1, 0};
const int dx[] = {0, 1, 0, -1};

namespace my_output {

void _outtextxy(int x, int y, const char* str) {
    outtextxy(x - textwidth(str) / 2, y - textheight(str) / 2, str);
}

} // namespace my_output

namespace my_random {

void _init() {
    srand(time(NULL));
}

int _rand() {
    if (RAND_MAX == 32767) return (rand() << 15) | rand();
    else return rand();
}

int rand(int x, int y) {
    return _rand() % (y - x + 1) + x;
}

}

namespace _waste {

class bot {
  public:
    int x, y;
    enum _toward toward;
    bot () : x(0), y(0), toward(r) {}
    bot (int x, int y, _toward twd) :
        x(x), y(y), toward(twd) {}
    
    void display () const {
        setcolor(YELLOW);
        for (int x0 = 0; x0 < Width; x0++) {
            for (int y0 = 0; y0 < High; y0++) {
                if (std::hypot(x0 - x, y0 - y) < 10) {
                    /**
                     * position of mouth :
                     * u : y0 - y > 0 && abs(x0 - x) < y0 - y
                     * r : x0 - x > 0 && abs(y0 - y) < x0 - x
                     * d : y0 - y < 0 && abs(x0 - x) < y - y0
                     * l : x0 - x < 0 && abs(y0 - y) < x - x0
                     **/
                    if (toward == u && (y0 - y > 0 && abs(x0 - x) < y0 - y)) continue;
                    if (toward == r && (x0 - x > 0 && abs(y0 - y) < x0 - x)) continue;
                    if (toward == d && (y0 - y < 0 && abs(x0 - x) < y - y0)) continue;
                    if (toward == l && (x0 - x < 0 && abs(y0 - y) < x - x0)) continue;
                    putpixel(x0, y0, WHITE);
                }
            }
        }
    }
};

} // namespace _waste

struct Pos {
    int x, y;
};

void init_pos(struct Pos *p) {
    p->x = p->y = 0;
}

namespace my_list {

struct List {
    struct List *prev, *next;
    struct Pos val;
};

void init(struct List *p) {
    p = (List*)malloc(sizeof(List));
    p->next = NULL;
    p->prev = NULL;
    init_pos(&(p->val));
}

void del_next(struct List *p) {
    if (p->next == NULL) return;
    free(p->next);
    p->next = NULL;
}

void del_prev(struct List *p) {
    if (p->prev == NULL) return;
    free(p->prev);
    p->prev = NULL;
}

void add_next(struct List* Now, struct List* Next) {
    Next->next = Now->next;
    Next->prev = Now;
    Now->next->prev = Next;
    Now->next = Next;
}

void add_prev(struct List* Now, struct List* Prev) {
    Prev->prev = Now->prev;
    Prev->next = Now;
    Now->prev->next = Prev;
    Now->prev = Prev;
}

}

namespace Map {

/**
 * done:
 *  1. volatile Map 
 * 
 * TODO:
 *  1. 2 snakes fight mode.
 *  2. variable candy.
 * 
 **/

const int Edge = 40;
const int aPixel = 20;
const int W = (Width - Edge * 2) / aPixel;
const int H = (High - Edge * 2) / aPixel;

#define GROUND_VAL 0
#define SNAKE_VAL 1
#define APPLE_VAL 2

/**
 * Ground = 0
 * Snake = 1
 * Apple = 2
 * 
 * amap[y][x] -> (x, y)
 * 
 **/
int **amap;

void initMap() {
    amap = (int**)malloc(sizeof(int*) * H);
    *amap = (int*)malloc(W * H * sizeof(int));
    for (int y = 1; y < H; y++) {
        amap[y] = *amap + W * y;
    }
    memset(*amap, 0, W * H * sizeof(int));
}

void delMap() {
    free(*amap);
    *amap = NULL;
    free(amap);
    amap = NULL;
}

} // namespace Map

namespace doSnake {

const int rawLen = 3;

/**
 * done:
 *  1. volatile rawLen
 *
 * TODO:
 *  1. 2 Snakes fight mode.
 *  2. variable candy.
 **/

struct Snake {
    struct my_list::List* Head;
    struct my_list::List* Tail;
    int add;
    enum _toward toward;
};

/**
 *  (0,0)
 *    +--x--------------------+
 *    | ---->----->----->----V|  A
 *    y V---<-----<-----<----<|  |
 *    | >--->----->...        | High
 *    |                       |  |
 *    |                       |  V
 *    +-----------------------+
 *          <--Width-->
 **/
struct ::Pos initNextPos() {
    static ::Pos now = {-1, 0};
    if (now.y & 1) { // odd
        if (now.x > 0) {
            now.x--;
        }
        else if (now.x == 0) {
            assert(now.y != Map::H - 1);
            now.y++;
        }
        else {
            assert(0);
        }
    }
    else { // even
        if (now.x < Map::W - 1) {
            now.x++;
        }
        else if (now.x == Map::W - 1) {
            assert(now.y != Map::H - 1);
            now.y++;
        }
        else {
            assert(0);
        }
    }
    return now;
};

// volatile rawLen but 1 snake
void init(struct Snake *s) {
    s = (struct Snake*)malloc(sizeof(struct Snake));
    my_list::init(s->Head);
    my_list::init(s->Tail);
    s->add = 0;
    
    int rem = rawLen - 2;
    struct my_list::List *p = s->Head;
    struct my_list::List *Now = NULL;
    
    /**
     * 0<-[Tail]-> ... <-[Now]-> <-[p]-> ... <-[Head]->0
     * link p & now to build body
     **/
    while (rem--) {
        my_list::init(Now);
        p->next = Now;
        Now->prev = p;
        p = Now;
    }
    
    // link tail & last body
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Tail;
    
    // put the snake into Map
    while (p->next != NULL) {
        p->val = initNextPos();
        p = p->next;
        
    }
    
    // calc Head toward
    if (p->val.y & 1) { // odd
        if (p->val.x > 0) {
            s->toward = l;
        }
        else {
            s->toward = d;
        }
    }
    else { // even
        if (p->val.x < Map::W - 1) {
            s->toward = r;
        }
        else {
            s->toward = d;
        }
    }
}

void del(struct Snake *s) {
    my_list::List *rem = s->Head->next, *tmp = NULL;
    while (rem != s->Tail && rem != NULL) {
        tmp = rem->next;
        free(rem);
        rem = tmp;
    }
    free(s->Head), free(s->Tail);
    free(s);
    s = NULL;
}

}

namespace Draw {

const color_t EdgeBk = BLUE;
const color_t GroundBk = GRAY;
const color_t SnakeColor = BLACK;
const color_t AppleColor = RED;

int isEdge(int,int);

struct ::Pos PosOfPixel(int x, int y) {
    assert(!isEdge(x, y));
    struct Pos res;
    res.x = (x - Map::Edge) / Map::aPixel;
    res.y = (y - Map::Edge) / Map::aPixel;
    return res;
};

int isEdge(int x, int y) {
    return x < Map::Edge
        || y < Map::Edge
        || x >= Map::W - Map::Edge
        || y >= Map::H - Map::Edge;
}

int isGround(int x, int y) {
    return !isEdge(x, y);
}

int isSnake(int x, int y) {
    struct Pos p = PosOfPixel(x, y);
    return Map::amap[p.y][p.x] == SNAKE_VAL;
}

int isApple(int x, int y) {
    struct Pos p = PosOfPixel(x, y);
    return Map::amap[p.y][p.x] == APPLE_VAL;
}

void drawMap() {
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < High; y++) {
            if (isEdge(x, y)) putpixel(x, y, EdgeBk);
            if (isGround(x, y)) putpixel(x, y, GroundBk);
            if (isSnake(x, y)) putpixel(x, y, SnakeColor);
            if (isApple(x, y)) putpixel(x, y, AppleColor);
        }
    }
}

}

namespace SetApple {

const int MaxAppleCount = 3;

struct ::Pos *PosPool;
int nHead;

void init() {
    PosPool = (struct Pos*)malloc(sizeof(Pos) * Map::H * Map::W);
    memset(PosPool, 0, sizeof(Pos) * Map::H * Map::W);
    nHead = 0;
}

void Set() {
    int cnt = 0;
    nHead = 0;
    for (int y = 0; y < Map::H; y++) {
        for (int x = 0; x < Map::W; x++) {
            if (Map::amap[y][x] == APPLE_VAL) {
                cnt++;
            }
            if (Map::amap[y][x] == GROUND_VAL) {
                PosPool[nHead++] = {x, y};
            }
        }
    }
    if (cnt == MaxAppleCount) return;
    while (cnt < MaxAppleCount) {
        int x = my_random::rand(0, nHead - 1);
        Map::amap[PosPool[x].y][PosPool[x].x] = APPLE_VAL;
        for (int j = x + 1; j < nHead; j++) {
            PosPool[j - 1] = PosPool[j];
        }
        nHead--;
    }
}

}

int MoveSnake(struct doSnake::Snake *s) {
    struct ::Pos NewHead;
    NewHead = {
        (s->Head->val.x + dx[s->toward] + Map::W) % Map::W,
        (s->Head->val.y + dy[s->toward] + Map::H) % Map::H
    };
    int val = Map::amap[NewHead.y][NewHead.x];
    if (val == SNAKE_VAL) return 0;
    if (val == APPLE_VAL) s->add++;
    
    
}

void Start() {
    setcolor(WHITE);
    setbkcolor(BLACK);
    my_output::_outtextxy(Wmid, Hmid - textheight("a") * 2, "Welcome to Gluttonous Snake!");
    my_output::_outtextxy(Wmid, Hmid, "press any key to start...");
    
    my_random::_init();
    Map::initMap();
    SetApple::init();
    
    getch();
}

void Main() {
    
    doSnake::Snake *s;
    
    while (1) {
        
        doSnake::init(s);
        Map::initMap();
        
        while (1) {
            delay_fps(fps);
            SetApple::Set();
            int res = MoveSnake(s);
            
        }
        
        Map::delMap();
        doSnake::del(s);
        
    }
}

int main()
{
	initgraph(Width, High);				//初始化图形界面
	
	Start();
	
	cleardevice();
	
	Main();
	
	getch();                            //暂停，等待键盘按键

	closegraph();						//关闭图形界面
	
	return 0;
}
