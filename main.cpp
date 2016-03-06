// nya-n
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <queue>
#include <algorithm>
#include <iterator>

using namespace std; // (>_<)

/* type */
typedef unsigned int uint;
typedef uint uid_t;
typedef struct Point_ { int y, x; Point_(int y, int x): y(y), x(x) {} } Point;

/* constant */
const string AINAME = "rhakt";
const int INF = 1 << 30;

/* util */
inline const int rand(const uint range) {
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
    SONIC = 0,  // [1, 8]
    MYMATEOR,   // [3, 7]
    OPMATEOR,   // [3, 7]
    MYTHUNDER,  // [3, 7]
    OPTHUNDER,  // [1, 5]
    MYGHOST,    // [2, 4]
    OPGHOST,    // [2, 4]
    TORNADO,    // [6, 30]
    NONE
};

enum class DIR : uint {
    UP = 0,
    LEFT,
    RIGHT,
    DOWN,
    NONE
};

inline const Point& dir2p(uint d) {
    static const vector<Point> table = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, {0, 0}};
    return table[d];
}
inline const Point& dir2p8(uint d) {
    static const vector<Point> table ={{-1, 0}, {0, -1}, {0, 1}, {1, 0}, {-1, 1}, {-1, -1}, {1, 1}, {1, -1}};
    return table[d];
}
inline const Point& dir2p(DIR d) { return dir2p(static_cast<uint>(d)); }
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
inline char dir2char(uint d) {
    static const vector<char> table = {'U', 'L', 'R', 'D', 'N'};
    return table[d];
}
inline char dir2char(DIR d) { return dir2char(static_cast<uint>(d)); }
inline uint opdir(uint d) { return 3 - d; }
inline DIR opdir(DIR d) { return static_cast<DIR>(opdir(static_cast<uint>(d))); }
inline uint rotdir(uint d) {
    static const vector<uint> table = {2, 0, 3, 1, 4};
    return table[d];
}
inline DIR rotdir(DIR d) { return static_cast<DIR>(rotdir(static_cast<uint>(d))); }

inline int dist(int y1, int x1, int y2, int x2) { return abs(y1 - y2) + abs(x1 - x2); }

/* class */
typedef vector<DIR> Move;

struct Chara {
    uid_t id;
    int y, x;
    bool closed = false;
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
    int distch = -INF;
    int distdg = -INF;
    int dog = -1;
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
    template <class D>
    inline Field& getField(const int y, const int x, D d) {
        const auto& p = dir2p(d);
        return field[y + p.y][x + p.x];
    }
    template <class F>
    inline void eachField(const F& f) {
        for(auto& fy : field) { for(auto& fxy : fy) { f(fxy); } }
    }
    template <class F>
    inline void eachAround(const int y, const int x, const uint max, const F& f) {
        for(uint i = 0; i < max; i++) { f(getField(y, x, i)); }
    }
    inline void updateDistDog(const int y, const int x, const int range) {
        eachField([](Field& f) { f.distdg = -INF; });
        queue<pair<int, Point>> que;
        que.emplace(0, Point{y, x});
        while(!que.empty()) {
            auto q = que.front(); que.pop();
            field[q.second.y][q.second.x].distdg = q.first;
            if(q.first + 1 > range) { continue; }
            eachAround(q.second.y, q.second.x, 4, [&](Field& f) {
                if(f.type != FIELD::FLOOR) { return; }
                if(f.distdg != -INF && f.distdg <= q.first + 1) { return; }
                que.emplace(q.first + 1, Point{f.y, f.x});
            });
        }
    }
};

struct Input {
    uint time;
    vector<uint> costs;
    Status st[2];
    inline const uint getCost(SKILL sk) const { return costs[static_cast<uint>(sk)]; }
};

struct Output {
    SKILL sk = SKILL::NONE;
    uint val1 = 0, val2 = 0;
    Move mv[2];
};

/* func */
template <class D>
inline bool isStoneMovable(Status& st, const int y, const int x, D d, bool ch = true) {
    auto& f = st.getField(y, x, d);
    if(f.type != FIELD::FLOOR || f.dog != -1) { return false; }
    if(ch) {
        if(f.chara[0] || f.chara[1]) { return false; }
    }
    return true;
}

template <class D>
inline bool isMovable(Status& st, const int y, const int x, D d) {
    auto& f = st.getField(y, x, d);
    if(f.type == FIELD::WALL) { return false; }
    if(f.type == FIELD::ROCK && !isStoneMovable(st, f.y, f.x, d)) { return false; }
    return true;
}

inline void deleteDog(Status& st, const int y, const int x, bool th = true) {
    st.field[y][x].dog = -1;
    st.field[y][x].threat--;
    if(th) { st.eachAround(y, x, 4, [&](Field& f) { f.threat--; }); }
}

inline void newDog(Status& st, const int y, const int x, const int id, bool th = true) {
    st.field[y][x].dog = id;
    st.field[y][x].threat++;
    if(th) { st.eachAround(y, x, 4, [&](Field& f) { f.threat++; }); }
}

template <class D>
inline void moveDog(Status& st, const int y1, const int x1, const int id, D d) {
    deleteDog(st, y1, x1);
    const auto& p = dir2p(d);
    newDog(st, y1 + p.y, x1 + p.x, id, false);
}

template <class D>
inline void backDog(Status& st, const int y1, const int x1, const int id, D d) {
    const auto& p = dir2p(d);
    deleteDog(st, y1 + p.y, x1 + p.x, false);
    newDog(st, y1, x1, id);
}

inline void newStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::ROCK;
}

inline void deleteStone(Status& st, const int y, const int x) {
    st.field[y][x].type = FIELD::FLOOR;
}

template <class D>
inline bool isDogMovable(Status& st, const int y, const int x, D d) {
    auto cur = st.field[y][x].distdg;
    auto& f = st.getField(y, x, d);
    if(f.type != FIELD::FLOOR || f.dog != -1) { return false; }
    return f.distdg == cur - 1;
}

template <class F>
void dogSim(Status& st, const int y, const int x, const int range, const int limit, const F& func) {
    vector<pair<uint, pair<int, Point>>> temp;
    st.updateDistDog(y, x, limit);
    vector<pair<int, Point>> dog;
    st.eachField([&](Field& f) {
        if(f.dog == -1) { return; }
        if(dist(y, x, f.y, f.x) > range) { return; }
        if(st.field[f.y][f.x].distdg == -INF) { return; }
        dog.emplace_back(f.dog, Point{f.y, f.x});
    });
    if(dog.empty()) { return; }
    sort(dog.begin(), dog.end(), [&](const pair<int, Point>& ch1, const pair<int, Point>& ch2) {
        auto d1 = st.field[ch1.second.y][ch1.second.x].distdg;
        auto d2 = st.field[ch2.second.y][ch2.second.x].distdg;
        if(d1 == -INF) { d1 = INF; }
        if(d2 == -INF) { d2 = INF; }
        if(d1 == d2) { return ch1.first < ch2.first; }
        return d1 < d2;
    });
    for(auto&& e : dog) {
        for(uint i = 0; i < 4; i++) {
            if(!isDogMovable(st, e.second.y, e.second.x, i)) { continue; }
            moveDog(st, e.second.y, e.second.x, e.first, i);
            temp.emplace_back(i, make_pair(e.first, Point{e.second.y, e.second.x}));
            break;
        }
    }
    func();
    for(auto it = temp.rbegin(); it != temp.rend(); ++it) {
        backDog(st, it->second.second.y, it->second.second.x, it->second.first, it->first);
    }
}

template <class D>
inline void moveStone(Status& st, const int y1, const int x1, D d) {
    deleteStone(st, y1, x1);
    const auto& p = dir2p(d);
    newStone(st, y1 + p.y, x1 + p.x);
}

template <class D>
inline void backStone(Status& st, const int y1, const int x1, D d) {
    const auto& p = dir2p(d);
    deleteStone(st, y1 + p.y, x1 + p.x);
    newStone(st, y1, x1);
}

template <class D>
inline void moveChara(Status& st, const uid_t id, D d) {
    auto& ch = st.ninja[id];
    st.field[ch.y][ch.x].chara[id] = false;
    const auto& p = dir2p(d);
    ch.y += p.y;
    ch.x += p.x;
    auto& f = st.field[ch.y][ch.x];
    if(f.type == FIELD::ROCK) { moveStone(st, f.y, f.x, d); }
    f.chara[id] = true;
}

inline bool cancelClosed(Status& st, const uid_t id, Move& mv) {
    auto& ch = st.ninja[id];
    queue<pair<Point, bool>> que, next;
    next.emplace(Point{ch.y, ch.x}, false);
    for(auto&& d : mv) {
        next.swap(que);
        while(!que.empty()) {
            auto p = que.front(); que.pop();
            auto& f = st.getField(p.first.y, p.first.x, d);
            if(f.type != FIELD::FLOOR) {
                next.emplace(p.first, p.second);
                continue;
            }
            if(!p.second && f.dog == -1 && !f.chara[0] && !f.chara[1] && !f.soul) {
                if(!isStoneMovable(st, f.y, f.x, d)) {
                    next.emplace(p.first, true);
                    continue;
                }
            }
            next.emplace(Point{f.y, f.x}, p.second);
        }
    }
    while(!next.empty()) {
        auto q = next.front(); next.pop();
        if(st.field[q.first.y][q.first.x].threat) { return true; }
    }
    return false;
}


template <class D, class F>
inline bool tryMove(Status& st, const int y, const int x, D d, const F& func) {
    if(!isMovable(st, y, x, d)) { return false; }
    auto& f = st.getField(y, x, d);
    if(f.type == FIELD::ROCK) {
        moveStone(st, f.y, f.x, d);
        func(f.y, f.x, true);
        backStone(st, f.y, f.x, d);
    } else {
        func(f.y, f.x, false);
    }
    return true;
}

bool checkClosed(Status&, const int, const int, const uint);
void safeMove(vector<pair<int, Move>>&, Status&, const int, const int, uint, bool);
inline int evaluate(Status& st, const int y, const int x) {
    int score = 0;
    int around = 0;
    dogSim(st, y, x, 4, 4, [&]() {
        vector<pair<int, Move>> mv;
        safeMove(mv, st, y, x, 2, false);
        if(mv.empty()) { score = -INF; }
        //score += mv.size() * 100;
        auto dogc = 0, fc = 0, dogp = 0;
        st.eachField([&](Field& f) {
            if(dist(y, x, f.y, f.x) > 4) { return; }
            if(f.type != FIELD::FLOOR) { return; }
            fc++;
            if(f.dog == -1 || f.distdg == -INF || f.distdg > 4) { return; }
            dogp -= max(6 - f.distdg, 0) * max(6 - f.distdg, 0) * 10;
            dogc++;
        });
        //if(fc > 0) { score -= dogc * 1000 / fc; }
        if(dogc > 0) { score -= dogp / dogc; }
        for(uint i = 0; i < 4; i++) {
            auto& f = st.getField(y, x, i);
            if(f.type == FIELD::FLOOR) {
                if(f.threat) { around += f.threat; }
            }
        }
    });
    for(uint i = 0; i < 4; i++) {
        auto& f = st.getField(y, x, i);
        if(f.type == FIELD::WALL) {
            ++around;
        } else if(f.type == FIELD::ROCK) {
            if(!isStoneMovable(st, f.y, f.x, i)) { ++around; }
            score -= 50;
        }
    }
    score += (min(y - 1, st.h - 2 - y) + min(x - 1, st.w - 2 - x)) * 5;
    score -= around * around * 100;
    return score;
}

void safeMove(vector<pair<int, Move>>& mv, Status& st, const int y, const int x, uint range, bool val) {
    if(range == 0) { return; }
    for(uint i = 0; i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            if(range > 1) {
                vector<pair<int, Move>> mv2;
                safeMove(mv2, st, yy, xx, range - 1, val);
                auto score = val ? (stone ? -10 : 0) : 0;
                for(auto&& m : mv2) {
                    m.second.insert(m.second.begin(), static_cast<DIR>(i));
                    mv.emplace_back(m.first + score, m.second);
                }
            } else {
                auto score = val ? evaluate(st, yy, xx) : 0;
                mv.emplace_back(score, Move{static_cast<DIR>(i)});
            }
        });
    }
}

inline void safeMove(vector<pair<int, Move>>& mv, Status& st, const uid_t id, const uint range, bool val) {
    auto& ch = st.ninja[id];
    safeMove(mv, st, ch.y, ch.x, range, val);
}

inline bool checkClosed(Status& st, const int y, const int x, const uint range) {
    vector<pair<int, Move>> mv;
    safeMove(mv, st, y, x, range, false);
    return mv.empty();
}

inline bool checkClosed(Status& st, const uid_t id, int range) {
    vector<pair<int, Move>> mv;
    safeMove(mv, st, id, range, false);
    return mv.empty();
}

bool greedRoute(vector<pair<int, Move>>& mv, Status& st, const int sy, const int sx, const int dy, const int dx, int limit) {
    if(sy == dy && sx == dx) { return true; }
    vector<DIR> cond;
    if(sy != dy) { cond.push_back(sy < dy ? DIR::DOWN : DIR::UP); }
    if(sx != dx) { cond.push_back(sx < dx ? DIR::RIGHT : DIR::LEFT); }

    for(auto&& c : cond) {
        auto& f = st.getField(sy, sx, c);
        if(limit == 0 && f.dog != -1) { continue; }
        if(limit == 0 && f.threat) { continue; }
        if(limit == 1 && f.threat) { continue; }
        tryMove(st, sy, sx, c, [&](const int yy, const int xx, bool stone) {
            if(yy == dy && xx == dx) {
                if(limit > 1 && checkClosed(st, yy, xx, limit - 1)) { return; }
                auto score = evaluate(st, yy, xx);
                mv.emplace_back(score, Move{c});
                return;
            }
            vector<pair<int, Move>> mv2;
            if(!greedRoute(mv2, st, yy, xx, dy, dx, max(limit - 1, 0))) { return; } 
            auto scote = (stone ? -10 : 0);
            for(auto&& m : mv2) {
                m.second.insert(m.second.begin(), c);
                mv.emplace_back(m.first + stone, m.second);
            }
        });
    }
    return !mv.empty();
} 

/* main */
Move brain(Input& in, const uid_t id, int limit) {
    Move mv;
    auto& me = in.st[0];
    auto& op = in.st[1];
    auto& ch = me.ninja[id];

    while(true) {
        sort(me.souls.begin(), me.souls.end(), [&](const Soul& s1, const Soul& s2) {
            if(s1.reserve) { return false; }
            if(s2.reserve) { return true; }
            if(!(dist(s1.y, s1.x, me.ninja[0].y, me.ninja[0].x) < limit)
                && !(dist(s1.y, s1.x, me.ninja[1].y, me.ninja[1].x) < limit)) {
                if(me.field[s1.y][s1.x].threat && !me.field[s2.y][s2.x].threat) { return false; }
            }
            if(!(dist(s2.y, s2.x, me.ninja[0].y, me.ninja[0].x) < limit)
                && !(dist(s2.y, s2.x, me.ninja[1].y, me.ninja[1].x) < limit)) {
                if(!me.field[s1.y][s1.x].threat && me.field[s2.y][s2.x].threat) { return true; }
            }
            if(id == 0) {
                auto& ch2 = me.ninja[1];
                if(dist(s1.y, s1.x, ch2.y, ch2.x) < dist(s1.y, s1.x, ch.y, ch.x)) { return false; }
                if(dist(s2.y, s2.x, ch2.y, ch2.x) < dist(s2.y, s2.x, ch.y, ch.x)) { return true; }
            }
            return dist(s1.y, s1.x, ch.y, ch.x) < dist(s2.y, s2.x, ch.y, ch.x);
        });
        bool next = false;
        for(auto&& s : me.souls) {
            if(s.reserve) { break; }
            if(id == 0) {
                auto& ch2 = me.ninja[1];
                if(dist(s.y, s.x, ch2.y, ch2.x) < dist(s.y, s.x, ch.y, ch.x)) { continue; }
            }
            vector<pair<int, Move>> vmv;
            if(greedRoute(vmv, me, ch.y, ch.x, s.y, s.x, limit - mv.size())) {
                for(auto&& v : vmv) { 
                    if(v.second.size() > limit - mv.size()) {
                        v.second.resize(limit - mv.size());
                        if(cancelClosed(me, id, v.second)) { v.first -= INF / 4; }
                    }
                }
                auto it = max_element(vmv.begin(), vmv.end(),
                    [&](const pair<int, Move>& p1, const pair<int, Move>& p2) { return p1.first > p2.first; });
                auto& v = it->second;
                //if(v.size() > limit - mv.size()) { v.resize(limit - mv.size()); }
                for(auto&& vv : v) {
                    moveChara(me, id, vv);
                    mv.push_back(vv);
                }
                s.reserve = true;
                if(mv.size() < limit) { next = true; }
                break;
            }
        }
        if(!next) { break; }
    }
    if(mv.size() < limit) {
        int r = limit - mv.size();
        vector<pair<int, Move>> mv2;
        safeMove(mv2, me, id, r, true);
        if(!mv2.empty()) {
            for(auto&& v : mv2) {
                if(cancelClosed(me, id, v.second)) { v.first -= INF / 4; }
            }
            sort(mv2.begin(), mv2.end(), [&](const pair<int, Move>& p1, const pair<int, Move>& p2) {
                return p1.first > p2.first;
            });
            for(auto&& vv : mv2[0].second) {
                moveChara(me, id, vv);
                mv.push_back(vv);
            }
        }
    }
    return std::move(mv);
}

inline int calcAroundDog(Status& st, const int y, const int x) {
    int around = 0;
    for(uint i = 0; i < 8; i++) {
        const auto& p = dir2p8(i);
        if(st.field[y + p.y][x + p.x].dog != -1) { ++around; }
    }
    return around;
}

bool escapeClosed(Output& ou, Input& in, const uid_t id) {
    auto& st = in.st[0];
    auto& ch = st.ninja[id];
    
    if(st.nin >= in.getCost(SKILL::TORNADO)) {
        auto dogs = calcAroundDog(st, ch.y, ch.x);
        bool exec = in.getCost(SKILL::TORNADO) < dogs * 5;
        exec = exec || dogs >= 4;
        if(exec) {
            for(uint i = 0; i < 8; i++) {
                const auto& p = dir2p8(i);
                auto& f = st.field[ch.y + p.y][ch.x + p.x];
                if(f.dog != -1) { deleteDog(st, f.y, f.x); }
            }
            ou.val1 = id;
            ou.sk = SKILL::TORNADO;
            return true;
        }
    }
    
    if(st.nin >= in.getCost(SKILL::SONIC) && !checkClosed(st, id, 3)) {
        ou.sk = SKILL::SONIC;
        return true;
    }
    if(st.nin >= in.getCost(SKILL::MYTHUNDER)) {
        for(auto dy = max(ch.y - 2, 1); dy <= min(ch.y + 2, st.h - 2); dy++) {
            auto rx = 2 - abs(ch.y - dy);
            for(auto dx = max(ch.x - rx, 1); dx <= min(ch.x + rx, st.w - 2); dx++) {
                auto& f = st.field[dy][dx];
                if(f.type != FIELD::ROCK) { continue; }
                deleteStone(st, dy, dx);
                if(!checkClosed(st, id, 2)) {
                    ou.val1 = dy;
                    ou.val2 = dx;
                    ou.sk = SKILL::MYTHUNDER;
                    return true;
                }
                newStone(st, dy, dx);
            }
        }
    }
    if(st.nin >= in.getCost(SKILL::MYGHOST)) {
        vector<Point> cond ={{1, 1},{st.h -2 , 1},{st.h - 2, st.w - 2},{1, st.w - 2}};
        bool suc = false;
        for(auto&& c : cond) {
            if(st.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
            dogSim(st, c.y, c.x, INF, INF, [&]() {
                if(!checkClosed(st, id, 2)) {
                    ou.val1 = c.y;
                    ou.val2 = c.x;
                    suc = true;
                }
            });
            if(suc) { break; }
        }
        if(suc){ ou.sk = SKILL::MYGHOST; return true; }
    }
    if(st.nin >= in.getCost(SKILL::TORNADO) && calcAroundDog(st, ch.y, ch.x) > 0) {
        for(uint i = 0; i < 8; i++) {
            const auto& p = dir2p8(i);
            auto& f = st.field[ch.y + p.y][ch.x + p.x];
            if(f.dog != -1) { deleteDog(st, f.y, f.x); }
        }
        ou.val1 = id;
        ou.sk = SKILL::TORNADO;
        return true;
    }
    
    return false;
}

void findCritical(Output& ou, Input& in) {
    auto& me = in.st[0];
    auto& op = in.st[1];
    static auto prevop = op;
    static bool strict = false;

    if(!strict && prevop.cnt[static_cast<uint>(SKILL::MYGHOST)] < op.cnt[static_cast<uint>(SKILL::MYGHOST)]) {
        for(auto&& nj : op.ninja) {
            if(prevop.field[nj.y][nj.x].threat > 0) { strict = true; break; }
        }
    }
    prevop = op;
    
    uint costmin = min(12u, in.getCost(SKILL::TORNADO)) + in.getCost(SKILL::OPMATEOR);
    
    if(me.nin >= in.getCost(SKILL::OPGHOST) && op.nin >= in.getCost(SKILL::MYGHOST)) {
        for(auto&& nj : op.ninja) {
            if(checkClosed(op, nj.id, 3)) {
                ou.val1 = nj.y;
                ou.val2 = nj.x;
                ou.sk = SKILL::OPGHOST;
                return;
            }
        }
        if(strict) {
            for(auto& s : op.souls) {
                if(!op.field[s.y][s.x].threat) { continue; }
                for(auto& nj : op.ninja) {
                    if(dist(nj.y, nj.x, s.y, s.x) <= 2) {
                        ou.val1 = s.y;
                        ou.val2 = s.x;
                        ou.sk = SKILL::OPGHOST;
                        return;
                    }
                }
            }
        }
    }

    if(me.nin >= in.getCost(SKILL::OPMATEOR) + costmin) {
        for(auto& s : op.souls) {
            DIR dir = DIR::NONE;
            if(s.inRock) {
                for(auto& nj : op.ninja) {
                    if(dist(nj.y, nj.x, s.y, s.x) <= 2 && (nj.y == s.y || nj.x == s.x)) {
                        if(nj.y > s.y) { dir = DIR::UP; } else if(nj.y < s.y) { dir = DIR::DOWN; }
                        else if(nj.x > s.x) { dir = DIR::LEFT; } else if(nj.x < s.x) { dir = DIR::RIGHT; }
                        break;
                    }
                }
                if(dir == DIR::NONE) { continue; }
                auto& fp = op.getField(s.y, s.x, dir);
                auto& fq = op.getField(s.y, s.x, opdir(dir));
                if(fp.type != FIELD::FLOOR || fp.soul || fp.dog != -1 || fp.chara[0] || fp.chara[1]) {
                    if(fq.type != FIELD::FLOOR || fq.soul || fq.dog != -1 || fq.chara[0] || fq.chara[1]) {
                        ou.val1 = fq.y;
                        ou.val2 = fq.x;
                        ou.sk = SKILL::OPMATEOR;
                        return;
                    }
                } else {
                    ou.val1 = fp.y;
                    ou.val2 = fp.x;
                    ou.sk = SKILL::OPMATEOR;
                    return;
                }
                continue;
            }
            if(op.field[s.y][s.x].threat) { continue; }
            for(auto& nj : op.ninja) {
                if(dist(nj.y, nj.x, s.y, s.x) == 2 && (nj.y == s.y || nj.x == s.x)) {
                    if(nj.y > s.y) { dir = DIR::UP; } else if(nj.y < s.y) { dir = DIR::DOWN; } 
                    else if(nj.x > s.x) { dir = DIR::LEFT; } else if(nj.x < s.x) { dir = DIR::RIGHT; }
                    break;
                }
            }
            if(dir == DIR::NONE) { continue; }
            if(isStoneMovable(op, s.y, s.x, dir, false)) { continue; }
            if(isStoneMovable(op, s.y, s.x, rotdir(dir), false)) { continue; }
            auto& f = op.getField(s.y, s.x, opdir(dir));
            if(f.type != FIELD::FLOOR || f.soul || f.dog != -1) { continue; }
            if(f.chara[0] || f.chara[1]) { continue; }
            ou.val1 = f.y;
            ou.val2 = f.x;
            ou.sk = SKILL::OPMATEOR;
            return;
        }
        for(auto& nj : op.ninja) {
            int ways = 0;
            for(uint i = 0; i < 4; i++) {
                auto& f = op.getField(nj.y, nj.x, i);
                if(f.type != FIELD::FLOOR || f.dog != -1) { continue; }
                ++ways;
            }
            if(ways > 1) { continue; }
            for(uint i = 0; i < 4; i++) {
                auto& f = op.getField(nj.y, nj.x, i);
                if(f.type != FIELD::FLOOR || f.dog != -1 || f.soul || f.threat) { continue; }
                if(f.chara[0] || f.chara[1]) { continue; }
                if(isStoneMovable(op, f.y, f.x, i, false)) { continue; }
                ou.val1 = f.y;
                ou.val2 = f.x;
                ou.sk = SKILL::OPMATEOR;
                return;
            }
        }
    }
    
    if(me.nin >= in.getCost(SKILL::OPTHUNDER) + costmin) {
        int maxd = -INF;
        op.eachField([&](Field& f) {
            if(f.type != FIELD::ROCK) { return; }
            for(uint i = 0; i < 2; i++) {
                auto& fp = op.getField(f.y, f.x, i);
                if(fp.type != FIELD::FLOOR || fp.distch == -INF) { continue; }
                auto& fq = op.getField(f.y, f.x, opdir(i));
                if(fq.type != FIELD::FLOOR || fq.distch == -INF) { continue; }
                if(fp.dog == -1 && fq.dog == -1) { continue; }
                if(fp.distch > 5 && fq.distch > 5) { continue; }
                if(fp.distch > 5 && fp.dog == -1) { continue; }
                if(fq.distch > 5 && fq.dog == -1) { continue; }
                if(abs(fp.distch - fq.distch) > maxd) {
                    ou.val1 = f.y;
                    ou.val2 = f.x;
                    maxd = abs(fp.distch - fq.distch);
                }
            }
        });
        if(maxd > 12) {
            ou.sk = SKILL::OPTHUNDER;
            return;
        }
    }

    /*if(me.nin >= in.getCost(SKILL::MYGHOST) + costmin) {
        int far = -INF, fary = -1, farx = -1;
        me.eachField([&](Field& f) {
            if(f.distch > far) {
                far = f.distch;
                fary = f.y;
                farx = f.x;
            }
        });
        if(far > 15) {
            ou.val1 = fary;
            ou.val2 = farx;
            ou.sk = SKILL::MYGHOST;
            return;
        }
    }*/

    if(in.getCost(SKILL::TORNADO) < 10) {
        for(auto&& ch : me.ninja) {
            if(me.nin >= in.getCost(SKILL::TORNADO) && calcAroundDog(me, ch.y, ch.x) > 2) {
                for(uint i = 0; i < 8; i++) {
                    const auto& p = dir2p8(i);
                    auto& f = me.field[ch.y + p.y][ch.x + p.x];
                    if(f.dog != -1) { deleteDog(me, f.y, f.x); }
                }
                ou.val1 = ch.id;
                ou.sk = SKILL::TORNADO;
                return;
            }
        }
    }
    /*
    if(me.nin >= in.getCost(SKILL::SONIC) + costmin && in.getCost(SKILL::SONIC) <= 2) {
        ou.sk = SKILL::SONIC;
        return;
    }*/
}

Output resolve(Input in) {
    Output ou;
    bool settle = false;
    auto& me = in.st[0];
    auto& op = in.st[1];
    
    for(auto& ch : me.ninja) { ch.closed = checkClosed(me, ch.id, 2); }
    
    if(me.ninja[0].closed && me.ninja[1].closed) {
        if(me.nin >= in.getCost(SKILL::SONIC) && in.getCost(SKILL::SONIC) < in.getCost(SKILL::MYGHOST) &&
            !checkClosed(me, 0, 3) && !checkClosed(me, 1, 3)) {
            ou.sk = SKILL::SONIC;
            settle = true;
        } else if(me.nin >= in.getCost(SKILL::MYGHOST)) {
            vector<Point> cond ={{1, 1},{me.h -2 , 1},{me.h - 2, me.w - 2},{1, me.w - 2}};
            bool suc = false;
            for(auto&& c : cond) {
                if(me.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
                dogSim(me, c.y, c.x, INF, INF, [&]() {
                    if(!checkClosed(me, 0, 2) && !!checkClosed(me, 1, 2)) {
                        ou.val1 = c.y;
                        ou.val2 = c.x;
                        suc = true;
                    }
                });
                if(suc) { break; }
            }
            if(suc) { ou.sk = SKILL::MYGHOST; settle = true; }
        }
    }
    if(!settle && me.ninja[0].closed) { settle = escapeClosed(ou, in, 0); }
    if(!settle && me.ninja[1].closed) { settle = escapeClosed(ou, in, 1); }
    if(!settle){ findCritical(ou, in); }
    
    if(ou.sk == SKILL::MYGHOST) {
        dogSim(me, ou.val1, ou.val2, INF, INF, [&]() {
            ou.mv[0] = brain(in, 0, 2);
            ou.mv[1] = brain(in, 1, 2);
        });
        return std::move(ou);
    }

    // chara0
    ou.mv[0] = brain(in, 0, ou.sk == SKILL::SONIC ? 3 : 2);

    // chara1
    //if(!settle && checkClosed(me, 1, 2)) { settle = escapeClosed(ou, in, 1); }
    ou.mv[1] = brain(in, 1, ou.sk == SKILL::SONIC ? 3 : 2);

    return std::move(ou);
}

/* IO */
template <class T, class F>
void forvec(vector<T>& v, size_t n, const F& f) {
    v.resize(n); for(auto&& e : v) { f(e); }
}

Input input() {
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
                e.y = j; e.x = i; ++i;
            } ++j;
        });
        forvec(st.ninja, (cin >> k, k), [](Chara& d) { cin >> d.id >> d.y >> d.x; });
        forvec(st.dogs, (cin >> k, k), [](Chara& d) { cin >> d.id >> d.y >> d.x; });
        forvec(st.souls, (cin >> k, k), [](Soul& p) { cin >> p.y >> p.x; });
        forvec(st.cnt, in.costs.size(), [](uint& u) { cin >> u; });

        queue<pair<int, Point>> quech;
        for(auto&& e : st.ninja) {
            st.field[e.y][e.x].chara[e.id] = true;
            quech.emplace(0, Point{e.y, e.x});
        }

        for(auto&& e : st.dogs) {
            st.field[e.y][e.x].dog = e.id;
            st.eachAround(e.y, e.x, 5, [&](Field& f) { f.threat++; });
        }

        for(auto&& e : st.souls) {
            if(st.field[e.y][e.x].type == FIELD::ROCK) { e.inRock = true; }
            st.field[e.y][e.x].soul = true;
        }

        while(!quech.empty()) {
            auto q = quech.front(); quech.pop();
            st.field[q.second.y][q.second.x].distch = q.first;
            st.eachAround(q.second.y, q.second.x, 4, [&](Field& f) {
                if(f.type != FIELD::FLOOR) { return; }
                if(f.distch != -INF && f.distch <= q.first + 1) { return; }
                quech.emplace(q.first + 1, Point{f.y, f.x});
            });
        }
    }
    return std::move(in);
}

inline void output(const Output ou) {
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

auto main()-> int {
    ios::sync_with_stdio(false);
    cout << AINAME << endl; cout.flush();
    while(true) { output(resolve(input())); }
}
