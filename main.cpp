/*******************************************
 * 
 * 西北工业大学程序设计基础III实验课大作业
 * 
 * 项目名称：贪吃蛇
 * 小组成员：刘逸熙、李霖、蔡思博
 * 
 * TODO: 难度系统，结算界面，游戏结束时再选单人/双人界面，暂停键
 * 
 *******************************************/
#include <graphics.h>
#include <bits/stdc++.h>

// 供开发者使用，是否进入 debug 模式以输出运行信息
#define DEBUG 0
#if DEBUG == 1
#   define Dbg printf("%d\n", __LINE__);
#elif DEBUG == 0
#   define Dbg
#endif

/* 图像坐标说明
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

int player_mode; // 1 单人 2 双人

const int Width = 1080;
const int High = 600;
const int Wmid = Width / 2;
const int Hmid = High / 2;
const int BufLen = 0x10000;
const int fps = 10;
const int timegap = 1000 / fps;

enum _toward { u = 0, r = 1, d = 2, l = 3 };
const int dy[] = {-1, 0, 1, 0};
const int dx[] = {0, 1, 0, -1};

namespace ns_Output {

void _outtextxy(int x, int y, const char* str) {
    outtextxy(x - textwidth(str) / 2, y - textheight(str) / 2, str);
}

void _outtextxy(int x, int y, const wchar_t* str) {
    outtextxy(x - textwidth(str) / 2, y - textheight(str) / 2, str);
}

} // namespace ns_Output

namespace ns_Random {

void init() {
    srand(time(NULL));
}

int _rand() {
    if (RAND_MAX == 32767) return (rand() << 15) | rand();
    else return rand();
}

int RandMax() {
    return 0x7fffffff;
}

int rand(int x, int y) {
    return _rand() % (y - x + 1) + x;
}

} // namespace ns_Random

struct Pos {
    int x, y;
    Pos() {x = y = 0;}
    Pos(int x, int y):
        x(x), y(y) {}
    
    bool operator ==(const Pos& ot) const {
        return x == ot.x && y == ot.y;
    }
};

namespace ns_PosHash {

class Hash {
  public:
    std::size_t operator()(const Pos& k) const {
        return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
    }
};

} // namespace ns_PosHash

namespace ns_List {

struct List {
    struct List *prev, *next;
    struct Pos val;
};

void init(struct List *p) {
    p->next = NULL;
    p->prev = NULL;
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

} // namespace ns_List

namespace ns_Map {

/**
 * 完成的：
 *  1. 可变大小的地图（直接修改 Width 和 High）
 *  2. 双人对战模式
 *  3. 更多苹果模组
 * 
 **/

const int Edge = 40;
const int aPixel = 20;   // 不要改这个了
const int W = (Width - Edge * 2) / aPixel;
const int H = (High - Edge * 2) / aPixel;

#define GROUND_VAL 0                // 背景

/**
 * Ground = 0
 * Snake = 1
 * Apple = 2
 * 
 * amap[y][x] -> (x, y)
 * 
 **/
int **amap;

void init() {
    amap = (int**)malloc(sizeof(int*) * H);
    for (int i = 0; i < H; i++) {
        amap[i] = (int*)malloc(sizeof(int) * W);
        memset(amap[i], 0, sizeof(int) * W);
    }
}

void delMap() {
    for (int i = 0; i < H; i++) {
        free(amap[i]);
        amap[i] = NULL;
    }
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

} // namespace ns_Map

namespace ns_Snake {

const int rawLen = 5;


#define SNAKE_VAL 1                 // 蛇
#define SNAKE2_VAL 2                // 第二条蛇（双人模式）

/**
 * done:
 *  1. 可变长度
 *  2. 双人对战
 *  3. 吃不同的苹果，变长不同
 * 
 * TODO:
 *  1. 人机蛇
 **/

struct Snake {
    struct ns_List::List* Head;
    struct ns_List::List* Tail;
    int add;
    enum _toward toward;
};

/** 要分配位置的蛇很长时，以这样的顺序安排蛇的位置
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
struct ::Pos initNextPos(int reset = 0, int nx = -1, int ny = 0) {
    static ::Pos now = Pos(-1, 0);
    
    // 重置一次位置
    if (reset) {
        now = Pos(nx, ny);
        return now;
    }
    
    if (now.y & 1) { // odd
        if (now.x > 0) {
            now.x--;
        }
        else if (now.x == 0) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = Pos(0, 0);
        }
    }
    else { // even
        if (now.x < ns_Map::W - 1) {
            now.x++;
        }
        else if (now.x == ns_Map::W - 1) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = Pos(0, 0);
        }
    }
    return now;
};

// 方向和上面的相反，中心对称
struct ::Pos initNextPos_2(int reset = 0, int nx = -1, int ny = 0) {
    static ::Pos now = Pos(-1, 0);
    
    // 重置一次位置
    if (reset) {
        now = Pos(nx, ny);
        return now;
    }
    
    if (now.y & 1) { // odd
        if (now.x > 0) {
            now.x--;
        }
        else if (now.x == 0) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = Pos(0, 0);
        }
    }
    else { // even
        if (now.x < ns_Map::W - 1) {
            now.x++;
        }
        else if (now.x == ns_Map::W - 1) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = Pos(0, 0);
        }
    }
    
    Pos the_EmpErroR = Pos(ns_Map::W - now.x - 1, ns_Map::H - now.y - 1);
    
    return the_EmpErroR;
};

void Reverse(struct Snake* s) {
    ns_List::List *p0 = s->Head, *p1 = s->Tail;
    while (p0 != NULL && p1 != NULL && p0 != p1 && p0->prev != p1) {
        std::swap(p0->val, p1->val);
        p0 = p0->next, p1 = p1->prev;
    }
}

// 单个蛇的可变长度
void init(struct Snake *s) {
    s->Head = (ns_List::List*)malloc(sizeof(struct ns_List::List));
    s->Tail = (ns_List::List*)malloc(sizeof(struct ns_List::List));
    ns_List::init(s->Head);
    ns_List::init(s->Tail);
    s->add = 0;
    
    int rem = rawLen - 2;
    struct ns_List::List *p = s->Head;
    struct ns_List::List *Now = NULL;
    
    Dbg
    
    /**
     * 0<-[Tail]-> ... <-[Now]-> <-[p]-> ... <-[Head]->0
     * 连接 p & now 来构建身体
     **/
    while (rem--) {
        Now = (ns_List::List*)malloc(sizeof(struct ns_List::List));
        ns_List::init(Now);
        p->next = Now;
        Now->prev = p;
        p = Now;
        
        Dbg
    }
    
    Dbg
    
    // 连接尾巴和身体
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Head;
    
    // 把蛇放进 Map 里
    do {
        p->val = initNextPos();
        ns_Map::amap[p->val.y][p->val.x] = SNAKE_VAL;
        p = p->next;
        
        Dbg
        if (DEBUG) printf("p = 0x%p\n", p);
        
    } while (p != NULL);
    
    // 翻转蛇（因为奇怪的初始化方式带来的特性）
    
    Reverse(s);
    
    p = s->Head;
    
    // 计算头朝向哪里
    if (p->val.y & 1) { // 奇数行
        if (p->val.x > 0) {
            s->toward = l;
        }
        else {
            s->toward = d;
        }
    }
    else { // 偶数行
        if (p->val.x < ns_Map::W - 1) {
            s->toward = r;
        }
        else {
            s->toward = d;
        }
    }
    
    // 重置位置
    initNextPos(1);
}

void init_2(struct Snake *s) {
    s->Head = (ns_List::List*)malloc(sizeof(struct ns_List::List));
    s->Tail = (ns_List::List*)malloc(sizeof(struct ns_List::List));
    ns_List::init(s->Head);
    ns_List::init(s->Tail);
    s->add = 0;
    
    int rem = rawLen - 2;
    struct ns_List::List *p = s->Head;
    struct ns_List::List *Now = NULL;
    
    Dbg
    
    /**
     * 0<-[Tail]-> ... <-[Now]-> <-[p]-> ... <-[Head]->0
     * 连接 p & now 来构建身体
     **/
    while (rem--) {
        Now = (ns_List::List*)malloc(sizeof(struct ns_List::List));
        ns_List::init(Now);
        p->next = Now;
        Now->prev = p;
        p = Now;
        
        Dbg
    }
    
    Dbg
    
    // 连接尾巴和身体
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Head;
    
    // 把蛇放进 Map 里
    do {
        p->val = initNextPos_2();
        ns_Map::amap[p->val.y][p->val.x] = SNAKE2_VAL;
        p = p->next;
        
        Dbg
        if (DEBUG) printf("p = 0x%p\n", p);
        
    } while (p != NULL);
    
    // 翻转蛇（因为奇怪的初始化方式带来的特性）
    Reverse(s);
    
    p = s->Head;
    
    Pos val2 = Pos(ns_Map::W - 1 - p->val.x, ns_Map::H - 1 - p->val.y);
    
    // 计算头朝向哪里
    if (val2.y & 1) { // 奇数行
        if (val2.x > 0) {
            s->toward = r;
        }
        else {
            s->toward = u;
        }
    }
    else { // 偶数行
        if (val2.x < ns_Map::W - 1) {
            s->toward = l;
        }
        else {
            s->toward = u;
        }
    }
    
    // 重置位置
    initNextPos_2(1);
}

void del(struct Snake *s) {
    if (s->Head == s->Tail) {
        free(s->Head);
        s->Head = NULL, s->Tail = nullptr;
        free(s);
        s = NULL;
        return;
    }
    
    ns_List::List *rem = s->Head->next, *tmp = NULL;
    while (rem != s->Tail && rem != NULL) {
        tmp = rem->next;
        free(rem);
        rem = tmp;
    }
    free(s->Head), free(s->Tail);
    s->Head = s->Tail = NULL;
    free(s);
    s = NULL;
}

void OutSnake(struct Snake* s) {
    ns_List::List *p = s->Head;
    int pit = 0;
    do {
        printf("pc = %d, pos = (%d, %d)\n", pit++, p->val.y, p->val.x);
        p = p->next;
    } while (p != NULL);
}

} // namespace ns_Snake

namespace ns_Color {

int whiteAbs(color_t x) {
    return (x & 0xff) + ((x >> 8) & 0xff) + ((x >> 16) & 0xff);
}

bool isWhite(color_t x) {
    if (whiteAbs(x) > 0xe0 * 3) return 1;
    else return 0;
}

color_t AtoB(color_t A, color_t B, int wa, int wb) {
    int _ra = (A >> 16) & 0xff, _rb = (B >> 16) & 0xff;
    int _ga = (A >> 8) & 0xff, _gb = (B >> 8) & 0xff;
    int _ba = (A >> 0) & 0xff, _bb = (B >> 0) & 0xff;
    
    _ra = (_ra * wa + _rb * wb) / (wa + wb);
    _ga = (_ga * wa + _gb * wb) / (wa + wb);
    _ba = (_ba * wa + _bb * wb) / (wa + wb);
    
    return RGB(_ra, _ga, _ba);
}

} // namespace ns_Image

namespace ns_Apple {

const int MaxAppleCount = 5;

struct ::Pos *PosPool;
int nHead;

const int AppleTimeLim = 15 * fps;

const int BigAppleP = 60;
const int SuperAppleP = 150;
const int UltraAppleP = 300;
const int PoisonAppleP = 60;
const int DeathAppleP = 150;

#define APPLE_VAL 3                 // 苹果（+1）
#define BIG_APPLE_VAL 4             // 大苹果（+3）
#define SUPER_APPLE_VAL 5           // 超级苹果（+5）
#define ULTRA_APPLE_VAL 6           // 至尊苹果（+10）
#define POISON_APPLE_VAL 7          // 毒苹果（-5）
#define DEATH_APPLE_VAL 8           // 墙（-inf）

const int AppleEff = 1;
const int BigAppleEff = 3;
const int SuperAppleEff = 5;
const int UltraAppleEff = 10;
const int PoisonAppleEff = -5;
const int DeathAppleEff = 0x80000000;

class SpApple {
  private:
    Pos pos;
    int kind;       // 吃完的效果
    int timer;
    
  public:
    SpApple() {}
    
    SpApple(Pos pos, int kind, int timer):
        pos(pos), kind(kind), timer(timer) {}
    
    void tick() {
        timer--;
    }
    bool death() {
        return timer == 0;
    }
    int effect() {
        return kind;
    }
    bool danger() {
        return timer < 20;
    }
    bool isDraw() {
        return timer & 1;
    }
};

std::unordered_map<Pos, SpApple, ns_PosHash::Hash> AppleSet;

void init() {
    PosPool = (struct Pos*)malloc(sizeof(Pos) * ns_Map::H * ns_Map::W);
    memset(PosPool, 0, sizeof(Pos) * ns_Map::H * ns_Map::W);
    nHead = 0;
}

Pos NewApplePos() {
    int x = ns_Random::rand(0, nHead - 1);
    Pos ix = PosPool[x];
    for (int j = x + 1; j < nHead; j++) {
        PosPool[j - 1] = PosPool[j];
    }
    PosPool[--nHead] = Pos();
    return ix;
}

void SetApple(int NowCnt) {
    if (NowCnt == MaxAppleCount) return;
    while (NowCnt < MaxAppleCount && nHead != 0) {
        Pos R = NewApplePos();
        ns_Map::amap[R.y][R.x] = APPLE_VAL;
        NowCnt++;
    }
}

bool NewSpApple(int AppleP, int AppleEff, int valType) {
    if (ns_Random::rand(1, AppleP) != 1) return 0;
    Pos N = NewApplePos();
    ns_Map::amap[N.y][N.x] = valType;
    SpApple New(N, AppleEff, AppleTimeLim);
    AppleSet[N] = New;
    return 1;
}

void SetSpApple() {
    if (nHead) nHead -= NewSpApple(BigAppleP, BigAppleEff, BIG_APPLE_VAL);
    if (nHead) nHead -= NewSpApple(SuperAppleP, SuperAppleEff, SUPER_APPLE_VAL);
    if (nHead) nHead -= NewSpApple(UltraAppleP, UltraAppleEff, ULTRA_APPLE_VAL);
    if (nHead) nHead -= NewSpApple(PoisonAppleP, PoisonAppleEff, POISON_APPLE_VAL);
    if (nHead) nHead -= NewSpApple(DeathAppleP, DeathAppleEff, DEATH_APPLE_VAL);
}

void UpdateSpApple() {
    std::vector<Pos> era;
    for (std::pair<const Pos, SpApple>& i: AppleSet) {
        if (i.second.death()) era.push_back(i.first);
        i.second.tick();
    }
    for (Pos i: era) {
        AppleSet.erase(i);
        ns_Map::amap[i.y][i.x] = GROUND_VAL;
    }
}

void Set() {
    int cnt = 0;
    nHead = 0;
    
    for (int y = 0; y < ns_Map::H; y++) {
        for (int x = 0; x < ns_Map::W; x++) {
            if (ns_Map::amap[y][x] == APPLE_VAL) {
                cnt++;
            }
            if (ns_Map::amap[y][x] == GROUND_VAL) {
                PosPool[nHead++] = Pos(x, y);
            }
        }
    }
    
    SetApple(cnt);
    SetSpApple();
    UpdateSpApple();
    
}

} // namespace ns_Apple

namespace ns_Draw {

const color_t EdgeBk = 0xff80090d;
const color_t GroundBk = 0xffffe7c5;

const color_t SnakeColor = 0xfff91a09;
const color_t Snake2Color = 0xffffcd02;

const color_t ToSet = 0x80000000;
const color_t SnakeEdge = 0xff000000;
const color_t SnakeEye1 = ~0 - (EdgeBk & 0xffffff);
const color_t SnakeEye2 = EdgeBk;

color_t SnakeHeadImage[ns_Map::aPixel][ns_Map::aPixel];           // 必须20*20
color_t SnakeBodyImage[ns_Map::aPixel][ns_Map::aPixel];           // 必须20*20
color_t SnakeTurnImage[ns_Map::aPixel][ns_Map::aPixel];           // 必须20*20
color_t SnakeTailImage[ns_Map::aPixel][ns_Map::aPixel];           // 必须20*20

color_t AppleImage[ns_Map::aPixel][ns_Map::aPixel];               // 必须20*20
color_t AppleBigImage[ns_Map::aPixel][ns_Map::aPixel];
color_t AppleSuperImage[ns_Map::aPixel][ns_Map::aPixel];
color_t AppleUltraImage[ns_Map::aPixel][ns_Map::aPixel];
color_t ApplePoisonImage[ns_Map::aPixel][ns_Map::aPixel];
color_t AppleDeathImage[ns_Map::aPixel][ns_Map::aPixel];

void init() {
    int n = ns_Map::aPixel;
    
    // 加载苹果的图像，只能是 20*20 的
    freopen("apple.txt", "r", stdin);
    
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &AppleImage[y][x]);
        }
    }
    
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &AppleBigImage[y][x]);
        }
    }
    
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &AppleSuperImage[y][x]);
        }
    }

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &AppleUltraImage[y][x]);
        }
    }

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &ApplePoisonImage[y][x]);
        }
    }
    
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            scanf("%x", &AppleDeathImage[y][x]);
        }
    }
    
    freopen("Snake.txt", "r", stdin);
    
    // 输入蛇头
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            char m = getchar();
            while (m != EOF && m <= 32) m = getchar();
            if (m == 'B') {
                SnakeHeadImage[y][x] = SnakeEdge;
            }
            if (m == 'D') {
                SnakeHeadImage[y][x] = SnakeEye2;
            }
        }
    }
    
    // 操作。
    for (int x = 0; x < n; x++) {
        int flag = 0;
        for (int y = 0; y < n; y++) {
            flag |= (SnakeHeadImage[y][x] == SnakeEdge);
            if (flag && SnakeHeadImage[y][x] == 0) {
                SnakeHeadImage[y][x] = ToSet;
            }
        }
    }
    
    // 直身
    for (int y = 0; y < n; y++) {
        int flag = 0;
        for (int x = 0; x < n; x++) {
            char m = getchar();
            while (m != EOF && m <= 32) m = getchar();
            flag ^= (m == 'B');
            if (m == 'B') {
                SnakeBodyImage[y][x] = SnakeEdge;
            }
            if (m == '0' && flag) {
                SnakeBodyImage[y][x] = ToSet;
            }
        }
    }
    
    // 转弯身
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            char m = getchar();
            while (m != EOF && m <= 32) m = getchar();
            if (m == 'B') {
                SnakeTurnImage[y][x] = SnakeEdge;
            }
            if (m == 'R') {
                SnakeTurnImage[y][x] = ToSet;
            }
        }
    }
    
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            char m = getchar();
            while (m != EOF && m <= 32) m = getchar();
            if (m == 'B') {
                SnakeTailImage[y][x] = SnakeEdge;
            }
        }
    }
    
    // 操作。
    for (int x = 0; x < n; x++) {
        int flag = 0;
        for (int y = 0; y < n; y++) {
            flag |= (SnakeTailImage[y][x] == SnakeEdge);
            if (flag && SnakeTailImage[y][x] == 0) {
                SnakeTailImage[y][x] = ToSet;
            }
        }
    }
    
    freopen(NULL, "r", stdin);
    
    if (DEBUG) {
        for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) {
            if (SnakeHeadImage[y][x] == 0) putchar('0');
            if (SnakeHeadImage[y][x] == ToSet) putchar('T');
            if (SnakeHeadImage[y][x] == EdgeBk) putchar('D');
            if (SnakeHeadImage[y][x] == SnakeEdge) putchar('B');
            putchar(" \n"[x == 19]);
        }
    }
}

int isEdge(int,int);

Pos PosOfPixel(int x, int y) {
    // assert(!isEdge(x, y));
    struct Pos res;
    res.x = (x - ns_Map::Edge) / ns_Map::aPixel;
    res.y = (y - ns_Map::Edge) / ns_Map::aPixel;
    return res;
}

int isEdge(int x, int y) {
    return x < ns_Map::Edge
        || y < ns_Map::Edge
        || x >= Width - ns_Map::Edge
        || y >= High - ns_Map::Edge;
}

int isNotEdge(int x, int y) {
    return !isEdge(x, y);
}

int isSnake(int x, int y) {
    if (isEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == SNAKE_VAL;
}

int isSnake_2(int x, int y) {
    if (isEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == SNAKE2_VAL;
}

int isApple(int x, int y) {
    if (isEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == APPLE_VAL;
}

int isGround(int x, int y) {
    if (isEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == GROUND_VAL;
}

int isSpApple(int x, int y) {
    if (isEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == BIG_APPLE_VAL ||
           ns_Map::amap[p.y][p.x] == SUPER_APPLE_VAL ||
           ns_Map::amap[p.y][p.x] == ULTRA_APPLE_VAL ||
           ns_Map::amap[p.y][p.x] == POISON_APPLE_VAL ||
           ns_Map::amap[p.y][p.x] == DEATH_APPLE_VAL;
}

void drawEdge(int x, int y) {
    putpixel(x, y, EdgeBk);
}

void drawGround(int x, int y) {
    putpixel(x, y, GroundBk);
}

void drawApple(int x, int y) {
    int nx = x - PosOfPixel(x, y).x * ns_Map::aPixel - ns_Map::Edge;
    int ny = y - PosOfPixel(x, y).y * ns_Map::aPixel - ns_Map::Edge;
    
    if (DEBUG) {
        printf("(%d, %d) : %d", ny, nx, AppleImage[ny][nx]);
    }
    
    if (AppleImage[ny][nx] == 0xffffff) {
        drawGround(x, y);
        return;
    }
    else {
        putpixel(x, y, AppleImage[ny][nx] | (0xff << 24));
    }
}

void drawSpApple(int x, int y) {
    int mx = PosOfPixel(x, y).x, my = PosOfPixel(x, y).y;
    int Type = ns_Map::amap[my][mx];
    int nx = x - mx * ns_Map::aPixel - ns_Map::Edge;
    int ny = y - my * ns_Map::aPixel - ns_Map::Edge;
    
    ns_Apple::SpApple apl = ns_Apple::AppleSet[Pos(mx, my)];
    
    if (apl.danger() && !apl.isDraw()) {
        drawGround(x, y);
        return;
    }
    
    switch (Type) {
        case BIG_APPLE_VAL:
            if (ns_Color::isWhite(AppleBigImage[ny][nx])) drawGround(x, y);
            else putpixel(x, y, AppleBigImage[ny][nx]);
            break;
        case SUPER_APPLE_VAL:
            if (ns_Color::isWhite(AppleSuperImage[ny][nx])) drawGround(x, y);
            else putpixel(x, y, AppleSuperImage[ny][nx]);
            break;
        case ULTRA_APPLE_VAL:
            if (ns_Color::isWhite(AppleUltraImage[ny][nx])) drawGround(x, y);
            else putpixel(x, y, AppleUltraImage[ny][nx]);
            break;
        case POISON_APPLE_VAL:
            if (ns_Color::isWhite(ApplePoisonImage[ny][nx])) drawGround(x, y);
            else putpixel(x, y, ApplePoisonImage[ny][nx]);
            break;
        case DEATH_APPLE_VAL:
            if (ns_Color::isWhite(AppleDeathImage[ny][nx])) drawGround(x, y);
            else putpixel(x, y, AppleDeathImage[ny][nx]);
            break;
        default:
            puts("drawSpApple error.");
    }
}

// u, r, d, l
void drawHead(int x, int y, int toward, const color_t Color) {
    int n = ns_Map::aPixel;
    int mx = x * ns_Map::aPixel + ns_Map::Edge;
    int my = y * ns_Map::aPixel + ns_Map::Edge;
    
    // 20 * 20 中的 x, y
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int nx = mx, ny = my; // 真实的 x, y
            if (toward == u) nx += x, ny += y;
            if (toward == d) nx += 19 - x, ny += 19 - y;
            if (toward == l) nx += y, ny += 19 - x;
            if (toward == r) nx += 19 - y, ny += x;
            
            if (SnakeHeadImage[y][x] == ToSet) putpixel(nx, ny, Color);
            else if (SnakeHeadImage[y][x] == 0) drawGround(nx, ny);
            else if (SnakeHeadImage[y][x] == SnakeEdge) putpixel(nx, ny, SnakeEdge);
            else if (SnakeHeadImage[y][x] == SnakeEye2) {
                if (Color == SnakeColor) putpixel(nx, ny, SnakeEye1);
                if (Color == Snake2Color) putpixel(nx, ny, SnakeEye2);
            }
            else puts("Undefined color.");
        }
    }
}

// ud, lr
void drawBody(int x, int y, int toward, const color_t Color) {
    int n = ns_Map::aPixel;
    int mx = x * ns_Map::aPixel + ns_Map::Edge;
    int my = y * ns_Map::aPixel + ns_Map::Edge;
    
    // 20 * 20 中的 x, y
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int nx = mx, ny = my; // 真实的 x, y
            if (toward == 0) nx += x, ny += y;
            if (toward == 1) nx += y, ny += x;
            
            if (SnakeBodyImage[y][x] == ToSet) putpixel(nx, ny, Color);
            else if (SnakeBodyImage[y][x] == 0) drawGround(nx, ny);
            else if (SnakeBodyImage[y][x] == SnakeEdge) putpixel(nx, ny, SnakeEdge);
            else puts("Undefined color.");
        }
    }
}

// ul, ur, dl, dr : l = 0, r = 1, u = 0, d = 2
void drawTurn(int x, int y, int toward, const color_t Color) {
    int n = ns_Map::aPixel;
    int mx = x * ns_Map::aPixel + ns_Map::Edge;
    int my = y * ns_Map::aPixel + ns_Map::Edge;
    
    // 20 * 20 中的 x, y
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int nx = mx, ny = my; // 真实的 x, y
            if (toward == 0) nx += x, ny += y;
            if (toward == 1) nx += 19 - x, ny += y;
            if (toward == 2) nx += x, ny += 19 - y;
            if (toward == 3) nx += 19 - x, ny += 19 - y;
            
            if (SnakeTurnImage[y][x] == ToSet) putpixel(nx, ny, Color);
            else if (SnakeTurnImage[y][x] == 0) drawGround(nx, ny);
            else if (SnakeTurnImage[y][x] == SnakeEdge) putpixel(nx, ny, SnakeEdge);
            else puts("Undefined color.");
        }
    }
}

// u, r, d, l
void drawTail(int x, int y, int toward, const color_t Color) {
    int n = ns_Map::aPixel;
    int mx = x * ns_Map::aPixel + ns_Map::Edge;
    int my = y * ns_Map::aPixel + ns_Map::Edge;
    
    // 20 * 20 中的 x, y
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int nx = mx, ny = my; // 真实的 x, y
            if (toward == u) nx += x, ny += y;
            if (toward == d) nx += 19 - x, ny += 19 - y;
            if (toward == l) nx += y, ny += 19 - x;
            if (toward == r) nx += 19 - y, ny += x;
            
            if (SnakeTailImage[y][x] == ToSet) putpixel(nx, ny, Color);
            else if (SnakeTailImage[y][x] == 0) drawGround(nx, ny);
            else if (SnakeTailImage[y][x] == SnakeEdge) putpixel(nx, ny, SnakeEdge);
            else puts("Undefined color.");
        }
    }
}

void drawSnake(struct ns_Snake::Snake *s, const color_t Color) {
    Pos HeadPos = s->Head->val;
    drawHead(HeadPos.x, HeadPos.y, s->toward, Color);
    
    ns_List::List *p = s->Head->next;
    
    while (p != NULL && p != s->Tail) {
        int dx0 = (p->prev->val.x - p->val.x), dy0 = (p->prev->val.y - p->val.y);
        int dx1 = (p->next->val.x - p->val.x), dy1 = (p->next->val.y - p->val.y);
        
        if (dx0 > 1) dx0 = -1; else if (dx0 < -1) dx0 = 1;
        if (dy0 > 1) dy0 = -1; else if (dy0 < -1) dy0 = 1;
        if (dx1 > 1) dx1 = -1; else if (dx1 < -1) dx1 = 1;
        if (dy1 > 1) dy1 = -1; else if (dy1 < -1) dy1 = 1;
        
        if ((dy0 == -1 && dy1 == 1) || (dy0 == 1 && dy1 == -1) ) {
            drawBody(p->val.x, p->val.y, 0, Color);
        }
        if ((dx0 == -1 && dx1 == 1) || (dx0 == 1 && dx1 == -1) ) {
            drawBody(p->val.x, p->val.y, 1, Color);
        }
        
        if ((dx0 == -1 && dy1 == -1) || (dx1 == -1 && dy0 == -1)) {
            drawTurn(p->val.x, p->val.y, 0, Color);
        }
        if ((dx0 == 1 && dy1 == -1) || (dx1 == 1 && dy0 == -1)) {
            drawTurn(p->val.x, p->val.y, 1, Color);
        }
        if ((dx0 == -1 && dy1 == 1) || (dx1 == -1 && dy0 == 1)) {
            drawTurn(p->val.x, p->val.y, 2, Color);
        }
        if ((dx0 == 1 && dy1 == 1) || (dx1 == 1 && dy0 == 1)) {
            drawTurn(p->val.x, p->val.y, 3, Color);
        }
        
        p = p->next;
    }
    
    int dx0 = (p->prev->val.x - p->val.x), dy0 = (p->prev->val.y - p->val.y);
    if (dx0 > 1) dx0 = -1; else if (dx0 < -1) dx0 = 1;
    if (dy0 > 1) dy0 = -1; else if (dy0 < -1) dy0 = 1;
    
    if (dy0 == 1) drawTail(p->val.x, p->val.y, 0, Color);
    if (dx0 == -1) drawTail(p->val.x, p->val.y, 1, Color);
    if (dy0 == -1) drawTail(p->val.x, p->val.y, 2, Color);
    if (dx0 == 1) drawTail(p->val.x, p->val.y, 3, Color);
}

void drawMap(struct ns_Snake::Snake *s) {
    
    Dbg
    if (DEBUG) ns_Map::OutMap();
    
    for (int x = 0; x < Width; x++) {
        for (int y = 0; y < High; y++) {
            if (isEdge(x, y)) {
                drawEdge(x, y);
                continue;
            }
            if (isGround(x, y)) {
                drawGround(x, y);
                continue;
            }
            if (isApple(x, y)) {
                drawApple(x, y);
                continue;
            }
            if (isSpApple(x, y)) {
                drawSpApple(x, y);
                continue;
            }
        }
    }
    drawSnake(s, SnakeColor);
    if (player_mode == 2) drawSnake(s + 1, Snake2Color);
}

} // namespace ns_Draw

namespace ns_Score {

const long long MaxScore = 1e7; // score 达到此分数即胜利
int Fullpc;      // 达到满分所需 pc 数，相当于难度，建议调整在 1000~10000 区间
int ScorePerpc;  // 每 pc 带来的分数

// pc 计算方法：floor(sqrt(combo)) * 所吃物品使蛇伸长的价值，poison为 0
// combo 计算方法：蛇长度每增加 1，combo数 +1，吃 poison 断 combo
// 最终显示的分数计算方法：蛇长度 len + score
// score 计算方法：所有的 pc * ScorePerpc 之和，再与 MaxScore 取最小值

long long score1 = 0, score2 = 0;
int combo1 = 0, combo2 = 0;
int len1 = 0, len2 = 0;


void init() {
    Fullpc = 5001;
    ScorePerpc = MaxScore / Fullpc;
    len1 = len2 = ns_Snake::rawLen;
    score1 = score2 = combo1 = combo2 = 0;
}

void getScore(int val, int opt) {
    long long& score = (!opt ? score1 : score2);
    int& combo = (!opt ? combo1 : combo2);
    int& len = (!opt ? len1 : len2);
    
    if (val == POISON_APPLE_VAL) {
        combo = 0;
        len -= ns_Apple::PoisonAppleEff;
        return;
    }
    
    int eff = 0;
    
    switch (val) {
        case APPLE_VAL:          eff = ns_Apple::AppleEff;         break;
        case BIG_APPLE_VAL:      eff = ns_Apple::BigAppleEff;      break;
        case SUPER_APPLE_VAL:    eff = ns_Apple::SuperAppleEff;    break;
        case ULTRA_APPLE_VAL:    eff = ns_Apple::UltraAppleEff;    break;
        default: puts("getScore error."); if (DEBUG) printf("val = %d\n", val);
    }
    
    combo += eff, len += eff;
    
    int pc = ((int)std::sqrt(combo)) * eff;
    score += pc * ScorePerpc;
    score = std::min(score, MaxScore);
}

std::wstring ltos(int val) {
    std::wstring s;
    for (int i = 0; i < 8; i++) {
        s.push_back(val % 10 + '0');
        val /= 10;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

void PrintScore1() {
    using namespace ns_Map;
    
    setfont(20, 15, "Consolas", 0, 0, 2, 0, 0, 0);
    setcolor(WHITE);
    setbkcolor(ns_Draw::EdgeBk);
    outtextxy(Edge, H - Edge / 2, (L"玩家1得分：" + ltos(score1 + len1)).c_str());
}

void PrintScore2() {
    using namespace ns_Map;
    
    setfont(20, 15, "Consolas", 0, 0, 2, 0, 0, 0);
    setcolor(WHITE);
    setbkcolor(ns_Draw::EdgeBk);
    outtextxy(Width / 2, H - Edge / 2, (L"玩家2得分：" + ltos(score2 + len2)).c_str());
}

} // namespace ns_Score

int MoveSnake(struct ns_Snake::Snake *s, int opt = 0) {
    struct ns_List::List* NewHead;
    NewHead = (struct ns_List::List*)malloc(sizeof(struct ns_List::List));
    ns_List::init(NewHead);
    
    if (DEBUG) printf("$ %d ", s->toward);
    
    NewHead->val = {
        (s->Head->val.x + dx[s->toward] + ns_Map::W) % ns_Map::W,
        (s->Head->val.y + dy[s->toward] + ns_Map::H) % ns_Map::H
    };
    
    int val = ns_Map::amap[NewHead->val.y][NewHead->val.x];
    
    if (DEBUG) {
        printf("Next: %d %d, val = %d\n", NewHead->val.y, NewHead->val.x, val);
        ns_Snake::OutSnake(s);
    }
    
    Dbg
    
    if (val == SNAKE_VAL || val == SNAKE2_VAL || val == DEATH_APPLE_VAL) return 0;
    
    switch (val) {
        case GROUND_VAL:         s->add +=   0;                          break;
        case APPLE_VAL:          s->add +=   ns_Apple::AppleEff;         break;
        case BIG_APPLE_VAL:      s->add +=   ns_Apple::BigAppleEff;      break;
        case SUPER_APPLE_VAL:    s->add +=   ns_Apple::SuperAppleEff;    break;
        case ULTRA_APPLE_VAL:    s->add +=   ns_Apple::UltraAppleEff;    break;
        case POISON_APPLE_VAL:   s->add +=   ns_Apple::PoisonAppleEff;   break;
        default: puts("MoveSnake error."); if (DEBUG) printf("val = %d\n", val);
    }
    
    if (val != GROUND_VAL) ns_Score::getScore(val, opt);
    
    if (opt == 0) ns_Map::amap[NewHead->val.y][NewHead->val.x] = SNAKE_VAL;
    if (opt == 1) ns_Map::amap[NewHead->val.y][NewHead->val.x] = SNAKE2_VAL;
    
    Dbg
    
    // 头前进一步
    s->Head->prev = NewHead;
    NewHead->next = s->Head;
    s->Head = NewHead;
    
    // 尾巴后退
    s->add--;
    
    Dbg
    
    while (s->add < 0 && s->Tail != s->Head) {
        struct ns_List::List* NewTail = s->Tail->prev;
        ns_Map::amap[s->Tail->val.y][s->Tail->val.x] = GROUND_VAL;
        ns_List::del_next(NewTail);
        s->Tail = NewTail;
        s->add++;
    }
    
    Dbg
    
    if (s->Tail == s->Head) return 0;
    
    return 1;
}

void Start() {
    PIMAGE img = newimage(Width, High);
    if (getimage(img, "Start.jpg", Width, High)) {
        puts("start image error.");
        exit(114);
    }
    putimage(0, 0, img);
    
    Dbg
    
    const int s_lx = 165, s_rx = 324;
    const int s_ly = 281, s_ry = 353;
    
    const int d_lx = 766, d_rx = 926;
    const int d_ly = 281, d_ry = 353;
    
    while (1) {
        delay_fps(10);
        
        while (keystate(key_mouse_l)) {
            int x, y;
            mousepos(&x, &y);
            if (s_lx <= x && x <= s_rx && s_ly <= y && y <= s_ry) {
                player_mode = 1;
                goto Single;
            }
            if (d_lx <= x && x <= d_rx && d_ly <= y && y <= d_ry) {
                player_mode = 2;
                goto Double;
            }
        }
    }
    
  Single:
  Double:
    
    ns_Random::init();
    ns_Apple::init();
    ns_Draw::init();
}

void Main() {
    using namespace ns_Score;
    
    ns_Snake::Snake *s;
    
    while (1) {
        
        ns_Map::init();
        ns_Score::init();
        
        Dbg
        
        s = (struct ns_Snake::Snake*)malloc(sizeof(struct ns_Snake::Snake));
        ns_Snake::init(s);
        
        Dbg
        
        ns_Draw::drawMap(s);
        
        Dbg
        
        ns_Apple::init();
        
        Dbg
        
        while (1) {
            if (DEBUG) getch();
            else delay_fps(fps);
            ns_Apple::Set();
            
            Dbg

            int usr1 = 0, pause = 0;
            
            while (kbhit()) {
                int k = getch();
                
                usr1 |= (k == 'w' || k == 'W') << 0;
                usr1 |= (k == 'a' || k == 'A') << 1;
                usr1 |= (k == 's' || k == 'S') << 2;
                usr1 |= (k == 'd' || k == 'D') << 3;
                
                usr1 |= (k == (0x100 | key_up)) << 0;
                usr1 |= (k == (0x100 | key_left)) << 1;
                usr1 |= (k == (0x100 | key_down)) << 2;
                usr1 |= (k == (0x100 | key_right)) << 3;
                
                pause |= (k == key_space || k == 'P' || k == 'p');
            }
            
            if (pause) {
                setfont(20, 15, "Consolas", 0, 0, 2, 0, 0, 0);
                setcolor(BLACK);
                setbkcolor(ns_Draw::GroundBk);
                ns_Output::_outtextxy(Width / 2, High / 2, L"暂停");
                while (!keystate(key_space) && !keystate('p') && !keystate('P')) {
                    delay_ms(10);
                }
            }
            
            if (s->toward == l || s->toward == r) {
                if (usr1 & 0x1) {s->toward = u; goto Nx;}
                if (usr1 & 0x4) {s->toward = d; goto Nx;}
            }
            if (s->toward == u || s->toward == d) {
                if (usr1 & 0x2) {s->toward = l; goto Nx;}
                if (usr1 & 0x8) {s->toward = r; goto Nx;}
            }
            
          Nx:
            
            int res = MoveSnake(s);
            
            if (DEBUG) printf("res = %d\n", res);
            
            if (!res) goto gamefail;
            
            ns_Draw::drawMap(s);
            ns_Score::PrintScore1();
            
            Dbg
        }
        
      gamefail:
        
        while (!keystate(key_mouse_l)) delay_ms(10);
        ns_Map::delMap();
        ns_Snake::del(s);
        
    }
}

void Main_2() {
    using namespace ns_Score;
    
    ns_Snake::Snake *s;
    
    while (1) { // 多局游戏
        
        ns_Map::init();
        ns_Score::init();
        
        Dbg
        
        s = (struct ns_Snake::Snake*)malloc(sizeof(struct ns_Snake::Snake) * 2);
        ns_Snake::init(s);
        ns_Snake::init_2(s + 1);
        
        Dbg
        
        ns_Draw::drawMap(s);
        
        Dbg
        
        ns_Apple::init();
        
        Dbg
        
        while (1) { // 一局内操作
            if (DEBUG) getch();
            else delay_fps(fps);
            ns_Apple::Set();
            
            int usr1 = 0, usr2 = 0, pause = 0;
            
            while (kbhit()) {
                int k = getch();
                
                usr1 |= (k == 'w' || k == 'W') << 0;
                usr1 |= (k == 'a' || k == 'A') << 1;
                usr1 |= (k == 's' || k == 'S') << 2;
                usr1 |= (k == 'd' || k == 'D') << 3;
                
                usr2 |= (k == (0x100 | key_up)) << 0;
                usr2 |= (k == (0x100 | key_left)) << 1;
                usr2 |= (k == (0x100 | key_down)) << 2;
                usr2 |= (k == (0x100 | key_right)) << 3;
                
                pause |= (k == key_space || k == 'P' || k == 'p');
            }
            
            if (pause) {
                setfont(20, 15, "Consolas", 0, 0, 2, 0, 0, 0);
                setcolor(BLACK);
                setbkcolor(ns_Draw::GroundBk);
                ns_Output::_outtextxy(Width / 2, High / 2, L"暂停");
                while (!keystate(key_space) && !keystate('p') && !keystate('P')) {
                    delay_ms(10);
                }
            }
            
            Dbg
            
            if (s->toward == l || s->toward == r) {
                if (usr1 & 0x1) {s->toward = u; goto Nx;}
                if (usr1 & 0x4) {s->toward = d; goto Nx;}
            }
            if (s->toward == u || s->toward == d) {
                if (usr1 & 0x2) {s->toward = l; goto Nx;}
                if (usr1 & 0x8) {s->toward = r; goto Nx;}
            }
            
          Nx:
            
            s++;
            
            if (s->toward == l || s->toward == r) {
                if (usr2 & 0x1) {s->toward = u; goto Nx2;}
                if (usr2 & 0x4) {s->toward = d; goto Nx2;}
            }
            if (s->toward == u || s->toward == d) {
                if (usr2 & 0x2) {s->toward = l; goto Nx2;}
                if (usr2 & 0x8) {s->toward = r; goto Nx2;}
            }
            
          Nx2:
            
            s--;
            
            int res = MoveSnake(s);
            int res2 = MoveSnake(s + 1, 1);
            
            if (!res && !res2) goto gamefail0;
            if (!res)          goto gamefail1;
            if (!res2)         goto gamefail2;
            if (ns_Score::score2 == ns_Score::MaxScore) goto gamefail1;
            if (ns_Score::score1 == ns_Score::MaxScore) goto gamefail2;
            
            ns_Draw::drawMap(s);
            ns_Score::PrintScore1();
            ns_Score::PrintScore2();
        }
        
      gamefail1:
        
        
        ns_Output::_outtextxy(Width / 2, High / 2, L"player 2 win!");
        goto ed;
      
      gamefail2:
        
        ns_Output::_outtextxy(Width / 2, High / 2, L"player 1 win!");
        
      gamefail0:
        
        goto ed;
        
      ed:
        
        while (!keystate(key_mouse_l)) delay_ms(10);
        ns_Map::delMap();
        ns_Snake::del(s);
        
    }
}

int main() {
    
	initgraph(Width, High);				// 初始化图形界面
	
	Start();
	
	if (player_mode == 1) Main();
	else Main_2();
	
	getch();                            // 暂停，等待键盘按键

	closegraph();						// 关闭图形界面
	
	return 0;
}
