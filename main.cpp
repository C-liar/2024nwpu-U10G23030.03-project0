/*******************************************
 * 
 * ������ҵ��ѧ������ƻ���IIIʵ��δ���ҵ
 * 
 * ��Ŀ���ƣ�̰����
 * С���Ա�������������ء���˼��
 * 
 * 
 *******************************************/
#include <graphics.h>
#include <bits/stdc++.h>

// ��������ʹ�ã��Ƿ���� debug ģʽ�����������Ϣ
#define DEBUG 0
#if DEBUG == 1
#   define Dbg printf("%d\n", __LINE__);
#elif DEBUG == 0
#   define Dbg
#endif

/* ͼ������˵��
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

int player_mode; // 1 ���� 2 ˫��

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

int rand(int x, int y) {
    return _rand() % (y - x + 1) + x;
}

} // namespace ns_Random

struct Pos {
    int x, y;
};

void init_pos(struct Pos *p) {
    p->x = p->y = 0;
}

namespace ns_List {

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

} // namespace ns_List

namespace ns_Map {

/**
 * ��ɵģ�
 *  1. �ɱ��С�ĵ�ͼ��ֱ���޸� Width �� High��
 *  2. ˫�˶�սģʽ
 * 
 * TODO:
 *  1. ����ƻ��ģ��
 * 
 **/

const int Edge = 40;
const int aPixel = 20;   // ��Ҫ�������
const int W = (Width - Edge * 2) / aPixel;
const int H = (High - Edge * 2) / aPixel;

#define GROUND_VAL 0                // ����
#define SNAKE_VAL 1                 // ��
#define SNAKE2_VAL 2                // �ڶ����ߣ�˫��ģʽ��
#define APPLE_VAL 3                 // ƻ��
#define BIG_APPLE_VAL 4             // ��ƻ�����°汾���£�
#define SUPER_APPLE_VAL 5           // ����ƻ����������
#define ULTRA_APPLE_VAL 6           // ����ƻ������

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

/**
 * done:
 *  1. �ɱ䳤��
 *
 * TODO:
 *  1. ˫�˶�ս
 *  2. �Բ�ͬ��ƻ�����䳤��ͬ
 *  3. �˻���
 **/

struct Snake {
    struct ns_List::List* Head;
    struct ns_List::List* Tail;
    int add;
    enum _toward toward;
};

/** Ҫ����λ�õ��ߺܳ�ʱ����������˳�����ߵ�λ��
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
    static ::Pos now = {-1, 0};
    
    // ����һ��λ��
    if (reset) {
        now = {nx, ny};
        return now;
    }
    
    if (now.y & 1) { // odd
        if (now.x > 0) {
            now.x--;
        }
        else if (now.x == 0) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = {0, 0};
        }
    }
    else { // even
        if (now.x < ns_Map::W - 1) {
            now.x++;
        }
        else if (now.x == ns_Map::W - 1) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = {0, 0};
        }
    }
    return now;
};

// �����������෴�����ĶԳ�
struct ::Pos initNextPos_2(int reset = 0, int nx = -1, int ny = 0) {
    static ::Pos now = {-1, 0};
    
    // ����һ��λ��
    if (reset) {
        now = {nx, ny};
        return now;
    }
    
    if (now.y & 1) { // odd
        if (now.x > 0) {
            now.x--;
        }
        else if (now.x == 0) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = {0, 0};
        }
    }
    else { // even
        if (now.x < ns_Map::W - 1) {
            now.x++;
        }
        else if (now.x == ns_Map::W - 1) {
            // assert(now.y != Map::H - 1);
            now.y++;
            if (now.y == ns_Map::H) now = {0, 0};
        }
    }
    
    Pos the_EmpErroR = {ns_Map::W - now.x - 1, ns_Map::H - now.y - 1};
    
    return the_EmpErroR;
};

void Reverse(struct Snake* s) {
    ns_List::List *p0 = s->Head, *p1 = s->Tail;
    while (p0 != NULL && p1 != NULL && p0 != p1 && p0->prev != p1) {
        std::swap(p0->val, p1->val);
        p0 = p0->next, p1 = p1->prev;
    }
}

// �����ߵĿɱ䳤��
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
     * ���� p & now ����������
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
    
    // ����β�ͺ�����
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Head;
    
    // ���߷Ž� Map ��
    do {
        p->val = initNextPos();
        ns_Map::amap[p->val.y][p->val.x] = SNAKE_VAL;
        p = p->next;
        
        Dbg
        if (DEBUG) printf("p = 0x%p\n", p);
        
    } while (p != NULL);
    
    // ��ת�ߣ���Ϊ��ֵĳ�ʼ����ʽ���������ԣ�
    
    Reverse(s);
    
    p = s->Head;
    
    // ����ͷ��������
    if (p->val.y & 1) { // ������
        if (p->val.x > 0) {
            s->toward = l;
        }
        else {
            s->toward = d;
        }
    }
    else { // ż����
        if (p->val.x < ns_Map::W - 1) {
            s->toward = r;
        }
        else {
            s->toward = d;
        }
    }
    
    // ����λ��
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
     * ���� p & now ����������
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
    
    // ����β�ͺ�����
    p->next = s->Tail;
    s->Tail->prev = p;
    
    p = s->Head;
    
    // ���߷Ž� Map ��
    do {
        p->val = initNextPos_2();
        ns_Map::amap[p->val.y][p->val.x] = SNAKE2_VAL;
        p = p->next;
        
        Dbg
        if (DEBUG) printf("p = 0x%p\n", p);
        
    } while (p != NULL);
    
    // ��ת�ߣ���Ϊ��ֵĳ�ʼ����ʽ���������ԣ�
    Reverse(s);
    
    p = s->Head;
    
    Pos val2 = {ns_Map::W - 1 - p->val.x, ns_Map::H - 1 - p->val.y};
    
    // ����ͷ��������
    if (val2.y & 1) { // ������
        if (val2.x > 0) {
            s->toward = r;
        }
        else {
            s->toward = u;
        }
    }
    else { // ż����
        if (val2.x < ns_Map::W - 1) {
            s->toward = l;
        }
        else {
            s->toward = u;
        }
    }
    
    // ����λ��
    initNextPos_2(1);
}

void del(struct Snake *s) {
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

namespace ns_Image {

int whiteAbs(color_t x) {
    return (x & 0xff) + ((x >> 8) & 0xff) + ((x >> 16) & 0xff);
}

bool isWhite(color_t x) {
    if (whiteAbs(x) > 0xd0 * 3) return 1;
    else return 0;
}

} // namespace ns_Image

namespace ns_Draw {

const color_t EdgeBk = 0xff80090d;
const color_t GroundBk = 0xffffe7c5;
const color_t SnakeColor = 0xfff91a09;
const color_t Snake2Color = 0xffffcd02;

color_t AppleImage[ns_Map::aPixel][ns_Map::aPixel];

void init() {
    // ����ƻ����ͼ��Ŀǰ��һ�����ֲ�Բ��ã�ֻ���� 20*20 ��
    freopen("apple.txt", "r", stdin);
    
    for (int y = 0; y < ns_Map::aPixel; y++) {
        for (int x = 0; x < ns_Map::aPixel; x++) {
            scanf("%x", &AppleImage[y][x]);
        }
    }
    
    freopen(NULL, "r", stdin);
}

int isEdge(int,int);

struct ::Pos PosOfPixel(int x, int y) {
    // assert(!isEdge(x, y));
    struct Pos res;
    res.x = (x - ns_Map::Edge) / ns_Map::aPixel;
    res.y = (y - ns_Map::Edge) / ns_Map::aPixel;
    return res;
};

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
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == SNAKE_VAL;
}

int isSnake_2(int x, int y) {
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == SNAKE2_VAL;
}

int isApple(int x, int y) {
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == APPLE_VAL;
}

int isGround(int x, int y) {
    if (!isNotEdge(x, y)) return 0;
    struct Pos p = PosOfPixel(x, y);
    return ns_Map::amap[p.y][p.x] == GROUND_VAL;
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

void drawSnake(struct ns_Snake::Snake *s) {
    Pos HeadPos = s->Head->val;
    // �¸������¸��汾���¡�
}

void drawSnake_2() {
    // ��������
}

void drawMap() {
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
            if (isSnake(x, y)) {
                putpixel(x, y, SnakeColor);
            }
            if (isSnake_2(x, y)) {
                putpixel(x, y, Snake2Color);
            }
        }
    }
}

} // namespace ns_Draw

namespace ns_Apple {

const int MaxAppleCount = 5;

struct ::Pos *PosPool;
int nHead;

void init() {
    PosPool = (struct Pos*)malloc(sizeof(Pos) * ns_Map::H * ns_Map::W);
    memset(PosPool, 0, sizeof(Pos) * ns_Map::H * ns_Map::W);
    nHead = 0;
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
                PosPool[nHead++] = {x, y};
            }
        }
    }
    if (cnt == MaxAppleCount) return;
    while (cnt < MaxAppleCount) {
        int x = ns_Random::rand(0, nHead - 1);
        ns_Map::amap[PosPool[x].y][PosPool[x].x] = APPLE_VAL;
        for (int j = x + 1; j < nHead; j++) {
            PosPool[j - 1] = PosPool[j];
        }
        nHead--;
        cnt++;
    }
}

} // namespace ns_Apple

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
    
    if (val == SNAKE_VAL || val == SNAKE2_VAL) return 0;
    
    switch (val) {
        case APPLE_VAL :         s->add += 1;   break;
        case BIG_APPLE_VAL :     s->add += 3;   break;
        case SUPER_APPLE_VAL :   s->add += 5;   break;
        case ULTRA_APPLE_VAL :   s->add += 10;  break;
    }
    
    if (opt == 0) ns_Map::amap[NewHead->val.y][NewHead->val.x] = SNAKE_VAL;
    if (opt == 1) ns_Map::amap[NewHead->val.y][NewHead->val.x] = SNAKE2_VAL;
    
    // ͷǰ��һ��
    s->Head->prev = NewHead;
    NewHead->next = s->Head;
    s->Head = NewHead;
    
    // β�ͺ���
    if (s->add) s->add--;
    else {
        struct ns_List::List* NewTail = s->Tail->prev;
        ns_Map::amap[s->Tail->val.y][s->Tail->val.x] = GROUND_VAL;
        ns_List::del_next(NewTail);
        s->Tail = NewTail;
    }
    
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
    
    ns_Snake::Snake *s;
    
    while (1) {
        
        ns_Map::init();
        
        Dbg
        
        s = (struct ns_Snake::Snake*)malloc(sizeof(struct ns_Snake::Snake));
        ns_Snake::init(s);
        
        Dbg
        
        ns_Draw::drawMap();
        
        Dbg
        
        ns_Apple::init();
        
        Dbg
        
        while (1) {
            if (DEBUG) getch();
            else delay_fps(fps);
            ns_Apple::Set();
            
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
            
            ns_Draw::drawMap();
        }
        
      gamefail:
        
        ns_Map::delMap();
        ns_Snake::del(s);
        
    }
}

void Main_2() {
    
    ns_Snake::Snake *s;
    
    while (1) {
        
        ns_Map::init();
        
        Dbg
        
        s = (struct ns_Snake::Snake*)malloc(sizeof(struct ns_Snake::Snake) * 2);
        ns_Snake::init(s);
        ns_Snake::init_2(s + 1);
        
        Dbg
        
        ns_Draw::drawMap();
        
        Dbg
        
        ns_Apple::init();
        
        Dbg
        
        while (1) {
            if (DEBUG) getch();
            else delay_fps(fps);
            ns_Apple::Set();
            
            Dbg
            
            if (keystate(key_W)) if (s->toward == l || s->toward == r) {s->toward = u; goto Nx;}
            if (keystate(key_A)) if (s->toward == u || s->toward == d) {s->toward = l; goto Nx;}
            if (keystate(key_S)) if (s->toward == l || s->toward == r) {s->toward = d; goto Nx;}
            if (keystate(key_D)) if (s->toward == u || s->toward == d) {s->toward = r; goto Nx;}
            
          Nx:
            
            s++;
            
            if (keystate(key_up)) if (s->toward == l || s->toward == r) {s->toward = u; goto Nx2;}
            if (keystate(key_left)) if (s->toward == u || s->toward == d) {s->toward = l; goto Nx2;}
            if (keystate(key_down)) if (s->toward == l || s->toward == r) {s->toward = d; goto Nx2;}
            if (keystate(key_right)) if (s->toward == u || s->toward == d) {s->toward = r; goto Nx2;}
            
          Nx2:
            
            s--;
            
            int res = MoveSnake(s);
            int res2 = MoveSnake(s + 1, 1);
            
            if (!res && !res2) goto gamefail0;
            if (!res)          goto gamefail1;
            if (!res2)         goto gamefail2;
            
            ns_Draw::drawMap();
        }
        
      gamefail1:
        
        ns_Output::_outtextxy(Width / 2, High / 2, L"1 loser, 2 winner.");
        goto ed;
      
      gamefail2:
        
        ns_Output::_outtextxy(Width / 2, High / 2, L"2 loser, 1 winner.");
        
      gamefail0:
        
        goto ed;
        
      ed:
        
        while (!keystate(key_mouse_l)) delay_ms(10);
        ns_Map::delMap();
        ns_Snake::del(s);
        
    }
}

int main()
{
	initgraph(Width, High);				//��ʼ��ͼ�ν���
	
	Start();
	
	if (player_mode == 1) Main();
	else Main_2();
	
	getch();                            //��ͣ���ȴ����̰���

	closegraph();						//�ر�ͼ�ν���
	
	return 0;
}
