#include <graphics.h>
#include <bits/stdc++.h>

#define DEBUG 0
#if DEBUG == 1
#define Dbg printf("%d\n", __LINE__);
#elif DEBUG == 0
#define Dbg
#endif

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
    for (int i = 0; i < H; i++) {
        amap[i] = (int*)malloc(sizeof(int) * W);
        memset(amap[i], 0, sizeof(int) * W);
    }
}

void delMap() {
    free(*amap);
    *amap = NULL;
    free(amap);
    amap = NULL;
}

void OutMap() {
    puts("-------Map-------");
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            printf("%d%c", amap[y][x], " \n"[x == W - 1]);
        }
    }
    puts("-------Map-------");
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
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == Map::H) now = {0, 0};
        }
    }
    else { // even
        if (now.x < Map::W - 1) {
            now.x++;
        }
        else if (now.x == Map::W - 1) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == Map::H) now = {0, 0};
        }
    }
    return now;
};

// volatile rawLen but 1 snake
void init(struct Snake *s) {
    s->Head = (my_list::List*)malloc(sizeof(struct my_list::List));
    s->Tail = (my_list::List*)malloc(sizeof(struct my_list::List));
    my_list::init(s->Head);
    my_list::init(s->Tail);
    s->add = 0;
    
    int rem = rawLen - 2;
    struct my_list::List *p = s->Head;
    struct my_list::List *Now = NULL;
    
    Dbg
    
    /**
     * 0<-[Tail]-> ... <-[Now]-> <-[p]-> ... <-[Head]->0
     * link p & now to build body
     **/
    while (rem--) {
        Now = (my_list::List*)malloc(sizeof(struct my_list::List));
        my_list::init(Now);
        p->next = Now;
        Now->prev = p;
        p = Now;
        
        Dbg
    }
    
    Dbg
    
    // link tail & last body
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Head;
    
    // put the snake into Map
    while (p->next != NULL) {
        p->val = initNextPos();
        Map::amap[p->val.y][p->val.x] = SNAKE_VAL;
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
    // assert(!isEdge(x, y));
    struct Pos res;
    res.x = (x - Map::Edge) / Map::aPixel;
    res.y = (y - Map::Edge) / Map::aPixel;
    return res;
};

int isEdge(int x, int y) {
    return x < Map::Edge
        || y < Map::Edge
        || x >= Width - Map::Edge
        || y >= High - Map::Edge;
}

int isNotEdge(int x, int y) {
    return !isEdge(x, y);
}

int isSnake(int x, int y) {
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return Map::amap[p.y][p.x] == SNAKE_VAL;
}

int isApple(int x, int y) {
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return Map::amap[p.y][p.x] == APPLE_VAL;
}

int isGround(int x, int y) {
    return isNotEdge(x, y) && !isApple(x, y) && !isSnake(x, y);
}

void drawMap() {
    if (DEBUG) Map::OutMap();
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < High; y++) {
            if (isEdge(x, y)) {
                putpixel(x, y, EdgeBk);
                continue;
            }
            if (isGround(x, y)) {
                putpixel(x, y, GroundBk);
                continue;
            }
            if (isSnake(x, y)) {
                putpixel(x, y, SnakeColor);
                continue;
            }
            if (isApple(x, y)) {
                putpixel(x, y, AppleColor);
                continue;
            }
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
        cnt++;
    }
}

}

int MoveSnake(struct doSnake::Snake *s) {
    struct my_list::List* NewHead;
    NewHead = (struct my_list::List*)malloc(sizeof(struct my_list::List));
    my_list::init(NewHead);
    
//    printf("$ %d ", s->toward);
    
    NewHead->val = {
        (s->Head->val.x + dx[s->toward] + Map::W) % Map::W,
        (s->Head->val.y + dy[s->toward] + Map::H) % Map::H
    };
    
    int val = Map::amap[NewHead->val.y][NewHead->val.x];
//    printf("%d %d\n", NewHead->val.y, NewHead->val.x);
    if (val == SNAKE_VAL) return 0;
    if (val == APPLE_VAL) {
        s->add += 1;
    }
    Map::amap[NewHead->val.y][NewHead->val.x] = SNAKE_VAL;
    
    // Add a New Head.
    s->Head->prev = NewHead;
    NewHead->next = s->Head;
    s->Head = NewHead;
    
    // Delete Tail
    if (s->add) s->add--;
    else {
        struct my_list::List* NewTail = s->Tail->prev;
        Map::amap[s->Tail->val.y][s->Tail->val.x] = GROUND_VAL;
        my_list::del_next(NewTail);
        s->Tail = NewTail;
    }
    
    return 1;
}

void Start() {
    setcolor(WHITE);
    setbkcolor(BLACK);
    my_output::_outtextxy(Wmid, Hmid - textheight("a") * 2, "Welcome to Gluttonous Snake!");
    my_output::_outtextxy(Wmid, Hmid, "press any key to start...");
    
    Dbg
    
    my_random::_init();
    Map::initMap();
    SetApple::init();
    
    getch();
    Dbg
}

void Main() {
    
    doSnake::Snake *s;
    
    while (1) {
        
        Map::initMap();
        
        Dbg
        
        s = (struct doSnake::Snake*)malloc(sizeof(struct doSnake::Snake));
        doSnake::init(s);
        
        Dbg
        
        Draw::drawMap();
        
        Dbg
        
        SetApple::init();
        
        Dbg
        
        while (1) {
            delay_fps(fps);
            SetApple::Set();
            
            Dbg
            
            if (kbhit()) {
                
                enum _toward twd = s->toward;
                
                
                key_msg S = getkey();
                if (S.key == key_up || S.key == key_W) {
                    if (twd == l || twd == r) twd = u;
                }
                if (S.key == key_left || S.key == key_A) {
                    if (twd == u || twd == d) twd = l;
                }
                if (S.key == key_down || S.key == key_S) {
                    if (twd == l || twd == r) twd = d;
                }
                if (S.key == key_right || S.key == key_D) {
                    if (twd == u || twd == d) twd = r;
                }
                
                s->toward = twd;
                flushkey();
            }
            
            int res = MoveSnake(s);
            if (!res) goto gamefail;
            
            Draw::drawMap();
        }
        
      gamefail:
        
        Map::delMap();
        doSnake::del(s);
        
    }
}

int main()
{
	initgraph(Width, High);				//初始化图形界面
	
	Start();
	
	Main();
	
	getch();                            //暂停，等待键盘按键

	closegraph();						//关闭图形界面
	
	return 0;
}
