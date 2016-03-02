// nya-n

#if _DEBUG && _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <iterator>

using namespace std;

/* type */
typedef unsigned int uint;
typedef uint uid_t;
typedef struct { int y, x; } Point;

/* constant */
const string AINAME = "rhakt";
const int INF = 1 << 30;

/* util */
inline const auto rand(const uint range) {
    static random_device rnd;
    static mt19937 mt(rnd());
    uniform_int_distribution<> ud(0, range - 1);
    return ud(mt);
}

/* enum */
enum class FIELD : char {
    FLOOR = '_',
    ROCK = 'O',
    WALL = 'W'
};

enum class SKILL : uint {
    SONIC = 0,
    MYMATEOR,
    OPMATEOR,
    MYTHUNDER,
    OPTHUNDER,
    MYGHOST,
    OPGHOST,
    TORNADO,
    NONE
};

enum class DIR : uint {
    UP = 0,
    LEFT,
    RIGHT,
    DOWN,
    NONE
};

inline auto dir2p(uint d) {
    static const vector<Point> table = {{-1, 0}, {0, -1}, {0, 1},{1, 0}, {0, 0}};
    return table[d];
}
inline auto dir2p(DIR d) { return dir2p(static_cast<uint>(d)); }
inline DIR p2dir(int y, int x) { 
    if(y == 0) {
        if(x == -1) { return DIR::LEFT; }
        if(x == 1) { return DIR::RIGHT; }
    } else if(x == 0) {
        if(y == -1) { return DIR::UP; }
        if(y == 1) { return DIR::DOWN; }
    }
    return DIR::NONE;
} 
inline auto dir2char(uint d) {
    static const vector<char> table = {'U', 'L', 'R', 'D', 'N'};
    return table[d];
}
inline auto dir2char(DIR d) { return dir2char(static_cast<uint>(d)); }

inline auto dist(int y1, int x1, int y2, int x2) { return abs(y1 - y2) + abs(x1 - x2); }

/* class */
struct Chara {
    uid_t id;
    int y, x;
};

struct Soul {
    int y, x;
    bool reserve = false;
    bool inRock = false;
};

struct Field {
    FIELD type;
    int y, x;
    bool threat = false;
    int value = 0;
    bool dog = false;
    bool chara[2] = {false, false};
};

struct Status {
    uint nin;
    int h, w;
    vector<vector<Field>> field;
    vector<Soul> souls;
    vector<Chara> ninja;
    vector<Chara> dogs;
    vector<uint> cnt;
};

struct Input {
    uint time;
    vector<uint> costs;
    Status st[2];
    const uint getCost(SKILL sk) const { return costs[static_cast<uint>(sk)]; }
};

struct Output {
    SKILL sk;
    uint val1, val2;
    vector<DIR> mv[2];
};

/* IO */
template <class T, class F>
auto forvec(vector<T>& v, size_t n, F& f) {
    v.resize(n); for(auto&& e : v) { f(e); }
}


auto input() {
    Input in;
    uint k; string s;
    cin >> in.time;
    forvec(in.costs, (cin >> k, k), [](uint& u) { cin >> u; });
    for(auto&& st : in.st) {
        cin >> st.nin >> st.h >> st.w;
        uint j = 0;
        forvec(st.field, st.h, [&](vector<Field>& f) {
            uint i = 0;
            cin >> s;
            f.resize(st.w);
            for(auto&& e : f) {
                e.type = static_cast<FIELD>(s[i]);
                e.y = j;
                e.x = i;
                ++i;
            }
            ++j;
        });
        forvec(st.ninja, (cin >> k, k), [](Chara& d) { cin >> d.id >> d.y >> d.x; });
        forvec(st.dogs, (cin >> k, k), [](Chara& d) { cin >> d.id >> d.y >> d.x; });
        forvec(st.souls, (cin >> k, k), [](Soul& p) { cin >> p.y >> p.x; });
        forvec(st.cnt, in.costs.size(), [](uint& u) { cin >> u; });
        for(auto&& e : st.ninja) {
            st.field[e.y][e.x].chara[e.id] = true;
        }
        for(auto&& e : st.dogs) {
            st.field[e.y][e.x].dog = true;
            for(uint i = 0; i < 5; i++) {
                auto p = dir2p(i);
                st.field[e.y + p.y][e.x + p.x].threat = true;
            }
        }
        for(auto&& e : st.souls) {
            if(st.field[e.y][e.x].type == FIELD::ROCK) { e.inRock = true; }
        }
    }
    return std::move(in);
}

inline void output(const Output& ou) {
    if(ou.sk == SKILL::NONE) {
        cout << 2 << endl;
    } else {
        cout << 3 << endl << static_cast<uint>(ou.sk);
        switch(ou.sk) {
        case SKILL::SONIC: break;
        case SKILL::TORNADO: cout << " " << ou.val1 << endl;
        default: cout << " " << ou.val1 << " " << ou.val2 << endl;
        }
        cout << endl;
    }
    for(auto&& mv : ou.mv) {
        for(auto&& e : mv) { cout << dir2char(e); }
        cout << endl;
    }
    cout.flush();
}

/* main */
inline bool isStoneMovable(Status& st, const int y, const int x, DIR d) {
    auto& p = dir2p(d);
    auto& ff = st.field[y + p.y][x + p.x];
    if(ff.type != FIELD::FLOOR) { return false; }
    if(ff.dog) { return false; }
    if(any_of(begin(ff.chara), end(ff.chara), [](bool b) { return b; })) { return false; }
    return true;
}

inline void newStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::ROCK;
}

inline void deleteStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::FLOOR;
}

inline void moveStone(Status& st, const int y1, const int x1, const int y2, const int x2) {
    deleteStone(st, y1, x1);
    newStone(st, y2, x2);
}

inline void moveStone(Status& st, const int y1, const int x1, DIR d) {
    deleteStone(st, y1, x1);
    auto& p = dir2p(d);
    newStone(st, y1 + p.y, x1 + p.x);
}

inline void backStone(Status& st, const int y1, const int x1, DIR d) {
    deleteStone(st, y1, x1);
    auto& p = dir2p(3 - static_cast<uint>(d));
    newStone(st, y1 + p.y, x1 + p.x);
}

inline void moveChara(Status& st, const uid_t id, DIR d) {
    auto& ch = st.ninja[id];
    st.field[ch.y][ch.x].chara[id] = false;
    auto& p = dir2p(d);
    ch.y += p.y;
    ch.x += p.x;
    auto& f = st.field[ch.y][ch.x];
    if(f.type == FIELD::ROCK) { moveStone(st, f.y, f.x, d); }
    f.chara[id] = true;
}

inline bool isMoveable(Status& st, const int y, const int x, DIR d) {
    auto& p = dir2p(d);
    auto& f = st.field[y + p.y][x + p.x];
    if(f.type == FIELD::WALL) { return false; }
    if(f.threat) { return false; }
    if(f.type == FIELD::ROCK) {
        if(!isStoneMovable(st, f.y, f.x, d)) { return false; }
    }
    return true;
}


auto route_value(Status& st, const int sy, const int sx, const int dy, const int dx, int r, int limit) {
    vector<DIR> cond;
    pair<DIR, int> res = {DIR::NONE, -INF};
    if(sy == dy && sx == dx) {
        if(r < limit) {
            bool flag = true;
            for(uint i = 0; i < 5; i++) {
                if(isMoveable(st, dy, dx, static_cast<DIR>(i))) { flag = false; break; }
            }
            if(flag) { return res; }
        }
        res.second = 100;
        return res;
    }
    DIR diry = (sy < dy ? DIR::DOWN : DIR::UP);
    DIR dirx = (sx < dx ? DIR::RIGHT : DIR::LEFT);
    if(sy != dy) { cond.push_back(diry); }
    if(sx != dx) { cond.push_back(dirx); }

    for(auto&& c : cond) {
        auto& dp = dir2p(c);
        auto& f = st.field[sy + dp.y][sx + dp.x];
        if(f.type == FIELD::WALL) { continue; }
        if(r + 1 == limit && f.threat) { continue; }
        if(f.type == FIELD::ROCK) {
            if(!isStoneMovable(st, f.y, f.x, c)) { continue; }
            moveStone(st, f.y, f.x, c);
        }
        auto p = route_value(st, f.y, f.x, dy, dx, r + 1, limit);
        if(f.type == FIELD::ROCK) {
            backStone(st, f.y, f.x, c);
        }
        if(p.second > res.second) {
            res.first = c;
            res.second = p.second;
        }
    }
    return res;
}

auto route(vector<DIR>& mv, Status& st, const int sy, const int sx, const int dy, const int dx, int limit) {
    vector<DIR> cond;
    if(sy == dy && sx == dx) { return false; }
    DIR diry = (sy < dy ? DIR::DOWN : DIR::UP);
    DIR dirx = (sx < dx ? DIR::RIGHT : DIR::LEFT);
    if(sy != dy) { cond.push_back(diry); }
    if(sx != dx) { cond.push_back(dirx); }
    
    pair<DIR, int> res = {DIR::NONE, -INF};
    DIR resdir = DIR::NONE;
    for(auto&& c : cond) {
        auto& dp = dir2p(c);
        auto& f = st.field[sy + dp.y][sx + dp.x];
        if(f.type == FIELD::WALL) { continue; }
        if(f.type == FIELD::ROCK) {
            if(!isStoneMovable(st, f.y, f.x, c)) { continue; }
            moveStone(st, f.y, f.x, c);
        }
        if(limit == 1 && f.threat) { continue; }
        auto p = route_value(st, f.y, f.x, dy, dx, 1, limit);
        if(f.type == FIELD::ROCK) {
            backStone(st, f.y, f.x, c);
        }
        if(p.second > res.second) {
            res = p;
            resdir = c;
        }
    }
    if(resdir == DIR::NONE) { return false; }
    mv.push_back(resdir);
    if(res.first != DIR::NONE && limit > 1) { mv.push_back(res.first); }
    return true;
}

auto brain(const uid_t id, Input& in, uint limit = 2) {
    vector<DIR> mv;
    auto& st = in.st[0];
    auto& ch = st.ninja[id];

    sort(st.souls.begin(), st.souls.end(), [&](const Soul& s1, const Soul& s2) {
        if(s1.reserve) { return false; }
        if(s2.reserve) { return true; }
        return dist(s1.y, s1.x, ch.y, ch.x) < dist(s2.y, s2.x, ch.y, ch.x);
    });

    for(auto&& s : st.souls) {
        if(s.reserve) { break; }
        if(route(mv, st, ch.y, ch.x, s.y, s.x, limit - mv.size())) {
            for(auto&& m : mv) { moveChara(st, id, m); }
            s.reserve = true;
            if(mv.size() == limit) { break; }
        }
    }

    if(mv.size() < limit) {
        for(uint i = 0; i < 4; i++) {
            auto& p = dir2p(i);
            auto& f = st.field[ch.y + p.y][ch.x + p.x];
            if(f.type == FIELD::WALL) { continue; }
            if(f.type == FIELD::ROCK) {
                if(!isStoneMovable(st, f.y, f.x, static_cast<DIR>(i))) { continue; }
            }
            if(f.threat) { continue; }
            mv.push_back(static_cast<DIR>(i));
            break;
        }
        if(mv.empty()) {
            mv.push_back(DIR::NONE);
        }
    }
    return std::move(mv);
}

inline bool check_closed(const uid_t id, Input& in) {
    
    return false;
}


auto resolve(Input& in) {
    auto& me = in.st[0];
    Output ou;
    ou.sk = SKILL::NONE;

    auto limit = (ou.sk == SKILL::SONIC ? 3 : 2);
    
    ou.mv[0] = brain(0, in, limit);
    ou.mv[1] = brain(1, in, limit);
    
    return std::move(ou);
}

auto main()-> int {

#if _DEBUG && _MSC_VER
    // check memory leak (only VS)
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ios::sync_with_stdio(false);

    cout << AINAME << endl; cout.flush();
    while(true) { output(resolve(input())); }
}
