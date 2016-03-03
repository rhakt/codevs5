// nya-n

#if _DEBUG && _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <queue>
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
    static const vector<Point> table = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, {0, 0}};
    return table[d];
}
inline auto dir2p8(uint d) {
    static const vector<Point> table ={{-1, 0}, {0, -1}, {0, 1}, {1, 0}, {-1, 1}, {-1, -1}, {1, 1}, {1, -1}};
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
    int threat = 0;
    int visit = -INF;
    bool dog = false;
    bool soul = false;
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
    int neardog = INF;
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

inline bool isStoneMovable(Status& st, const int y, const int x, DIR d) {
    auto& p = dir2p(d);
    auto& ff = st.field[y + p.y][x + p.x];
    if(ff.type != FIELD::FLOOR) { return false; }
    if(ff.dog) { return false; }
    if(any_of(begin(ff.chara), end(ff.chara), [](bool b) { return b; })) { return false; }
    return true;
}

inline int calcAround(Status& st, const int y, const int x) {
    int around = 0;
    for(uint i = 0; i < 8; i++) {
        auto& p = dir2p8(i);
        auto& f = st.field[y + p.y][x + p.x];
        if(f.type == FIELD::WALL) { around += 3; }
        else if(f.threat) { around++; }
        else if(f.type == FIELD::ROCK) {
            around += 2;
            if(i < 4) {
                if(!isStoneMovable(st, f.y, f.x, static_cast<DIR>(i))) { around++; }
            }
        }
    }
    return around;
}

inline int calcAroundDog(Status& st, const int y, const int x) {
    int around = 0;
    for(uint i = 0; i < 8; i++) {
        auto& p = dir2p8(i);
        auto& f = st.field[y + p.y][x + p.x];
        if(f.dog) { around++; }
    }
    return around;
}

inline void deleteDog(Status& st, const int y, const int x) {
    st.field[y][x].dog = false;
    for(uint i = 0; i < 5; i++) {
        auto& p = dir2p(i);
        auto& f = st.field[y + p.y][x + p.x];
        f.threat--;
    }
}

inline void newStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::ROCK;
}

inline void deleteStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::FLOOR;
}

inline auto moveStone(Status& st, const int y1, const int x1, DIR d) {
    deleteStone(st, y1, x1);
    auto& p = dir2p(d);
    newStone(st, y1 + p.y, x1 + p.x);
    return p;
}

inline auto backStone(Status& st, const int y1, const int x1, DIR d) {
    auto& p = dir2p(static_cast<uint>(d));
    deleteStone(st, y1 + p.y, x1 + p.x);
    newStone(st, y1, x1);
    return p;
}

inline auto moveChara(Status& st, const uid_t id, DIR d) {
    auto& ch = st.ninja[id];
    st.field[ch.y][ch.x].chara[id] = false;
    auto& p = dir2p(d);
    ch.y += p.y;
    ch.x += p.x;
    auto& f = st.field[ch.y][ch.x];
    if(f.type == FIELD::ROCK) { moveStone(st, f.y, f.x, d); }
    f.chara[id] = true;
    return p;
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
        
        queue<pair<int, Point>> que;
        for(auto&& e : st.ninja) {
            st.field[e.y][e.x].chara[e.id] = true;
            que.push(make_pair(0, Point{e.y, e.x}));
        }
        while(!que.empty()) {
            auto q = que.front();
            que.pop();
            st.field[q.second.y][q.second.x].visit = q.first;
            if(st.field[q.second.y][q.second.x].dog && q.first < 5) { st.neardog++; }
            for(uint i = 0; i < 4; i++) {
                auto& p = dir2p(i);
                auto& f = st.field[q.second.y + p.y][q.second.x + p.x];
                if(f.type != FIELD::FLOOR) { continue; }
                if(f.visit != -INF && f.visit <= q.first + 1) { continue; }
                que.push(make_pair(q.first + 1, Point{f.y, f.x}));
            }
        }

        for(auto&& e : st.dogs) {
            st.field[e.y][e.x].dog = true;
            for(uint i = 0; i < 5; i++) {
                auto p = dir2p(i);
                st.field[e.y + p.y][e.x + p.x].threat++;
            }
        }
        for(auto&& e : st.souls) {
            if(st.field[e.y][e.x].type == FIELD::ROCK) { e.inRock = true; }
            st.field[e.y][e.x].soul = true;
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
        case SKILL::TORNADO: cout << " " << ou.val1; break;
        default: cout << " " << ou.val1 << " " << ou.val2;
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
bool route(vector<DIR>&, Status&, const int, const int, const int, const int, int);
inline bool check_closed(const int y, const int x, Status& st, int limit) {
    int r = limit;
    for(auto dy = max(y - r, 1); dy <= min(y + r, st.h - 2); dy++) {
        auto rx = r - abs(y - dy);
        for(auto dx = max(x - rx, 1); dx <= min(x + rx, st.w - 2); dx++) {
            auto& f = st.field[dy][dx];
            if(f.type == FIELD::WALL) { continue; }
            if(f.threat) { continue; }
            std::vector<DIR> mv;
            if(route(mv, st, y, x, dy, dx, limit)) {
                return false;
            }
        }
    }
    return true;
}
inline bool check_closed(const uid_t id, Status& st, int limit) {
    auto& ch = st.ninja[id];
    return check_closed(ch.y, ch.x, st, limit);
}

auto route_value(Status& st, const int sy, const int sx, const int dy, const int dx, int r, int limit) {
    vector<DIR> cond;
    pair<pair<DIR, DIR>, int> res = {{DIR::NONE, DIR::NONE}, -INF};
    if(sy == dy && sx == dx) {
        if(r < limit) {
            /*bool flag = true;
            for(uint i = 0; i < 5; i++) {
                if(isMoveable(st, dy, dx, static_cast<DIR>(i))) { flag = false; break; }
            }
            if(flag) { return res; }*/
            if(check_closed(dy, dx, st, limit - r)) { return res; }
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
        /*if(r + 1 == limit) {
            bool flag = true;
            for(uint i = 0; i < 5; i++) {
                if(isMoveable(st, f.y, f.x, static_cast<DIR>(i))) { flag = false; break; }
            }
            if(flag) {continue; }
        }*/
        bool ms = false;
        if(f.type == FIELD::ROCK) {
            if(!isStoneMovable(st, f.y, f.x, c)) { continue; }
            moveStone(st, f.y, f.x, c);
            ms = true;
        }
        auto p = route_value(st, f.y, f.x, dy, dx, r + 1, limit);
        if(ms) {
            p.second -= 10;
            auto& q = backStone(st, f.y, f.x, c);
            auto& ff = st.field[f.y + q.y][f.x + q.x];
            if(ff.soul) {
                bool flag = true;
                for(uint i = 0; i < 4; i++) {
                    if(isStoneMovable(st, ff.y, ff.x, static_cast<DIR>(i))) { flag = false; break; }
                }
                if(flag) { p.second = -INF; }
            }
        }
        if(f.dog) { p.second -= 100; }
        if(p.second > res.second) {
            res.first = make_pair(c, p.first.first);
            res.second = p.second;
        }
    }
    return res;
}

bool route(vector<DIR>& mv, Status& st, const int sy, const int sx, const int dy, const int dx, int limit) {
    vector<DIR> cond;
    if(sy == dy && sx == dx) { return false; }
    DIR diry = (sy < dy ? DIR::DOWN : DIR::UP);
    DIR dirx = (sx < dx ? DIR::RIGHT : DIR::LEFT);
    if(sy != dy) { cond.push_back(diry); }
    if(sx != dx) { cond.push_back(dirx); }
    
    pair<pair<DIR, DIR>, int> res = {{DIR::NONE, DIR::NONE}, -INF};
    DIR resdir = DIR::NONE;
    for(auto&& c : cond) {
        auto& dp = dir2p(c);
        auto& f = st.field[sy + dp.y][sx + dp.x];
        if(f.type == FIELD::WALL) { continue; }
        if(limit == 1 && f.threat) { continue; }
        bool ms = false;
        if(f.type == FIELD::ROCK) {
            if(!isStoneMovable(st, f.y, f.x, c)) { continue; }
            moveStone(st, f.y, f.x, c);
            ms = true;
        }
        auto p = route_value(st, f.y, f.x, dy, dx, 1, limit);
        if(ms) {
            p.second -= 10;
            auto& q = backStone(st, f.y, f.x, c);
            auto& ff = st.field[f.y + q.y][f.x + q.x];
            if(ff.soul) {
                bool flag = true;
                for(uint i = 0; i < 4; i++) {
                    if(isStoneMovable(st, ff.y, ff.x, static_cast<DIR>(i))) { flag = false; break; }
                }
                if(flag){ p.second = -INF; }
            }
        }
        if(f.dog) { p.second -= 100; }
        if(p.second > res.second) {
            res = p;
            resdir = c;
        }
    }
    if(resdir == DIR::NONE) { return false; }
    mv.push_back(resdir);
    if(res.first.first != DIR::NONE && limit > 1) { mv.push_back(res.first.first); }
    if(res.first.second != DIR::NONE && limit > 2) { mv.push_back(res.first.second); }
    return true;
}

auto brain(const uid_t id, Input& in, uint limit = 2) {
    vector<DIR> mv;
    auto& st = in.st[0];
    auto& ch = st.ninja[id];

    while(true) {
        sort(st.souls.begin(), st.souls.end(), [&](const Soul& s1, const Soul& s2) {
            if(s1.reserve) { return false; }
            if(s2.reserve) { return true; }
            if(st.field[s1.y][s1.x].threat && !st.field[s2.y][s2.x].threat) { return false; }
            if(!st.field[s1.y][s1.x].threat && st.field[s2.y][s2.x].threat) { return true; }
            if(id == 0) {
                auto& ch2 = st.ninja[1];
                if(dist(s1.y, s1.x, ch2.y, ch2.x) < dist(s1.y, s1.x, ch.y, ch.x)) { return false; }
                if(dist(s2.y, s2.x, ch2.y, ch2.x) < dist(s2.y, s2.x, ch.y, ch.x)) { return true; }
            }
            return dist(s1.y, s1.x, ch.y, ch.x) < dist(s2.y, s2.x, ch.y, ch.x);
        });
        bool next = false;
        for(auto&& s : st.souls) {
            if(s.reserve) { break; }
            if(id == 0) {
                auto& ch2 = st.ninja[1];
                if(dist(s.y, s.x, ch2.y, ch2.x) < dist(s.y, s.x, ch.y, ch.x)) { continue; }
            }
            vector<DIR> mv2;
            if(route(mv2, st, ch.y, ch.x, s.y, s.x, limit - mv.size())) {
                for(auto&& m : mv2) { moveChara(st, id, m); mv.push_back(m); }
                s.reserve = true;
                if(mv.size() < limit) { next = true; }
                break;
            }
        }
        if(!next) { break; }
    }

    if(mv.size() < limit) {
        int r = limit;
        vector<pair<int, Point>> cond;
        for(auto dy = max(ch.y - r, 1); dy <= min(ch.y + r, st.h - 2); dy++) {
            auto rx = r - abs(ch.y - dy);
            for(auto dx = max(ch.x - rx, 1); dx <= min(ch.x + rx, st.w - 2); dx++) {
                auto& f = st.field[dy][dx];
                if(f.type == FIELD::WALL) { continue; }
                if(f.threat) { continue; }
                cond.emplace_back(calcAround(st, f.y, f.x), Point{dy, dx});
            }
        }
        sort(cond.begin(), cond.end(), [](const auto& p1, const auto& p2) {
            if(p1.first == p2.first) {
                return abs(p2.second.y + p2.second.x) < abs(p1.second.y + p1.second.x);
            }
            return p1.first < p2.first;
        });
        for(auto&& e : cond) {
            vector<DIR> mv2;
            if(route(mv2, st, ch.y, ch.x, e.second.y, e.second.x, limit - mv.size())) {
                for(auto&& m : mv2) { moveChara(st, id, m); mv.push_back(m); }
                break;
            }
        }
        if(mv.empty()) {
            mv.push_back(DIR::NONE);
        }
    }
    return std::move(mv);
}

inline auto escape_closed(Input& in, Output& ou, const uid_t id) {
    auto& st = in.st[0];
    auto& ch = st.ninja[id];
    if(st.nin >= in.getCost(SKILL::TORNADO)) {
        auto dogs = calcAroundDog(st, ch.y, ch.x);
        auto exec = in.getCost(SKILL::TORNADO) < 15 && dogs > 2;
        exec = exec || dogs > 3;
        if(exec) {
            for(uint i = 0; i < 8; i++) {
                auto& p = dir2p8(i);
                auto& f = st.field[ch.y + p.y][ch.x + p.x];
                if(f.dog) { deleteDog(st, f.y, f.x); }
            }
            ou.val1 = id;
            return ou.sk = SKILL::TORNADO;
        }
    }
    if(st.nin >= in.getCost(SKILL::SONIC) && !check_closed(id, st, 3)) {
        return ou.sk = SKILL::SONIC;
    }
    if(st.nin >= in.getCost(SKILL::MYTHUNDER)) {
        for(auto dy = max(ch.y - 2, 1); dy <= min(ch.y + 2, st.h - 2); dy++) {
            auto rx = 2 - abs(ch.y - dy);
            for(auto dx = max(ch.x - rx, 1); dx <= min(ch.x + rx, st.w - 2); dx++) {
                auto& f = st.field[dy][dx];
                if(f.type != FIELD::ROCK) { continue; }
                deleteStone(st, dy, dx);
                if(!check_closed(id, st, 2)) {
                    ou.val1 = dy;
                    ou.val2 = dx;
                    return ou.sk = SKILL::MYTHUNDER;
                }
                newStone(st, dy, dx);
            }
        }
    }
    if(st.nin >= in.getCost(SKILL::TORNADO) && calcAroundDog(st, ch.y, ch.x) > 1) {
        for(uint i = 0; i < 8; i++) {
            auto& p = dir2p8(i);
            auto& f = st.field[ch.y + p.y][ch.x + p.x];
            if(f.dog) { deleteDog(st, f.y, f.x); }
        }
        ou.val1 = id;
        return ou.sk = SKILL::TORNADO;
    }
    if(st.nin >= in.getCost(SKILL::MYGHOST)) {
        /*ou.val1 = ch.y;
        ou.val2 = ch.x;
        for(uint i = 0; i < 4; i++) {
            auto& p = dir2p(i);
            auto& f = st.field[ch.y + p.y][ch.x + p.x];
            if(f.dog) { deleteDog(st, f.y, f.x); }
        }
        st.field[ch.y][ch.x].threat++;
        return ou.sk = SKILL::MYGHOST; */
        int far = -INF, fary = -1, farx = -1;
        for(auto&& fy : st.field) {
            for(auto&& fxy : fy) {
                if(fxy.visit > far) {
                    far = fxy.visit;
                    fary = fxy.y;
                    farx = fxy.x;
                }
            }
        }
        ou.val1 = fary;
        ou.val2 = farx;
        return ou.sk = SKILL::MYGHOST;
    }
    return ou.sk;
}

void find_critical(Input& in, Output& ou) {
    auto& me = in.st[0];
    auto& op = in.st[1];
    uint costmin = (me.neardog > 4 ? min(15u, in.getCost(SKILL::TORNADO)) : in.getCost(SKILL::TORNADO));

    if(me.nin >= in.getCost(SKILL::OPMATEOR) + costmin) {
        for(auto& s : op.souls) {
            DIR dir = DIR::NONE;
            if(s.inRock) {
                for(auto& nj : op.ninja) {
                    if(dist(nj.y, nj.x, s.y, s.x) <= 2 && (nj.y == s.y || nj.x == s.x)) {
                        if(nj.y > s.y) { dir = DIR::UP; }
                        else if(nj.y < s.y) { dir = DIR::DOWN; }
                        else if(nj.x > s.x) { dir = DIR::LEFT; }
                        else if(nj.x < s.x) { dir = DIR::RIGHT; }
                        break;
                    }
                }
                if(dir == DIR::NONE) { continue; }
                auto& p = dir2p(dir);
                auto& q = dir2p(3 - static_cast<uint>(dir));
                auto& fp = op.field[s.y + p.y][s.x + p.x];
                auto& fq = op.field[s.y + q.y][s.x + q.x];
                if(fp.type != FIELD::FLOOR || fp.soul || fp.dog || 
                    any_of(begin(fp.chara), end(fp.chara), [](bool b) { return b; })) { 
                    if(fq.type != FIELD::FLOOR || fq.soul || fq.dog ||
                        any_of(begin(fq.chara), end(fq.chara), [](bool b) { return b; })) {
                        ou.val1 = s.y + q.y;
                        ou.val2 = s.x + q.x;
                        ou.sk = SKILL::OPMATEOR;
                        return;
                    }
                } else {
                    ou.val1 = s.y + p.y;
                    ou.val2 = s.x + p.x;
                    ou.sk = SKILL::OPMATEOR;
                    return;
                }
                continue;
            }
            if(op.field[s.y][s.x].threat) { continue; }
            for(auto& nj : op.ninja) {
                if(dist(nj.y, nj.x, s.y, s.x) == 2 && (nj.y == s.y || nj.x == s.x)) {
                    if(nj.y > s.y) { dir = DIR::UP; }
                    else if(nj.y < s.y) { dir = DIR::DOWN; }
                    else if(nj.x > s.x) { dir = DIR::LEFT; }
                    else if(nj.x < s.x) { dir = DIR::RIGHT; }
                    break;
                }
            }
            if(dir == DIR::NONE) { continue; }
            if(isStoneMovable(op, s.y, s.x, dir)) { continue; }
            auto& p = dir2p(3 - static_cast<uint>(dir));
            auto& f = op.field[s.y + p.y][s.x + p.x];
            if(f.type != FIELD::FLOOR || f.soul || f.dog) { continue; }
            if(any_of(begin(f.chara), end(f.chara), [](bool b) { return b; })) { continue; }
            ou.val1 = s.y + p.y;
            ou.val2 = s.x + p.x;
            ou.sk = SKILL::OPMATEOR;
            return;
        }
        for(auto& nj : op.ninja) {
            int ways = 0;
            for(uint i = 0; i < 4; i++) {
                auto& p = dir2p(i);
                auto& f = op.field[nj.y + p.y][nj.x + p.x];
                if(f.type != FIELD::FLOOR || f.dog || f.threat) { continue; }
                ++ways;
            }
            if(ways > 1) { continue; }
            for(uint i = 0; i < 4; i++) {
                auto& p = dir2p(i);
                auto& f = op.field[nj.y + p.y][nj.x + p.x];
                if(f.type != FIELD::FLOOR || f.dog || f.soul || f.threat) { continue; }
                if(any_of(begin(f.chara), end(f.chara), [](bool b) { return b; })) { continue; }
                if(isStoneMovable(op, f.y, f.x, static_cast<DIR>(i))) { continue; }
                ou.val1 = f.y;
                ou.val2 = f.x;
                ou.sk = SKILL::OPMATEOR;
                return;
            }
        }
    }

    if(me.nin >= in.getCost(SKILL::OPTHUNDER) + costmin) {
        for(auto&& fy : op.field) {
            for(auto&& fxy : fy) {
                if(fxy.type != FIELD::ROCK) { continue; }
                for(uint i = 0; i < 2; i++) {
                    auto& p = dir2p(i);
                    auto& fp = op.field[fxy.y + p.y][fxy.x + p.x];
                    if(fp.type != FIELD::FLOOR || fp.visit == -INF) { continue; }
                    auto& q = dir2p(3-i);
                    auto& fq = op.field[fxy.y + q.y][fxy.x + q.x];
                    if(fq.type != FIELD::FLOOR || fq.visit == -INF) { continue; }
                    if(abs(fp.visit - fq.visit) > 15) {
                        ou.val1 = fxy.y;
                        ou.val2 = fxy.x;
                        ou.sk = SKILL::OPTHUNDER;
                        return;
                    }
                }
            }
        }
    }

    if(me.nin >= in.getCost(SKILL::MYGHOST) + costmin) {
        int far = -INF, fary = -1, farx = -1;
        for(auto&& fy : me.field) {
            for(auto&& fxy : fy) {
                if(fxy.visit > far) {
                    far = fxy.visit;
                    fary = fxy.y;
                    farx = fxy.x;
                }
            }
        } 
        if(far > 15) {
            ou.val1 = fary;
            ou.val2 = farx;
            ou.sk = SKILL::MYGHOST;
        }
        return;
    }

    if(me.nin >= in.getCost(SKILL::SONIC) + costmin && in.getCost(SKILL::SONIC) <= 2) {
        ou.sk = SKILL::SONIC;
    }
}

auto resolve(Input& in) {
    auto& st = in.st[0];
    bool settle = false;
    Output ou;
    ou.sk = SKILL::NONE;

    if(check_closed(0, st, 2)) {
        if(escape_closed(in, ou, 0) != SKILL::NONE) { settle = true; }
    }
    if(!settle) { find_critical(in, ou); }

    auto limit = (ou.sk == SKILL::SONIC ? 3 : 2);
    ou.mv[0] = brain(0, in, limit);
    
    if(!settle) {
        if(check_closed(1, st, limit)) {
            if(escape_closed(in, ou, 1) != SKILL::NONE) { settle = true; }
        }
    }
    limit = (ou.sk == SKILL::SONIC ? 3 : 2);
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
