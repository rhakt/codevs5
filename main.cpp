// nya-n
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <queue>
#include <algorithm>
#include <iterator>
#include <cassert>

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
    vector<uint> souls;
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

struct ScoredMove {
    Move mv = {DIR::NONE};
    int score = -INF;
    Status st;
    int y, x;
    ScoredMove() = default;
    ScoredMove(Move&& mv, int score, Status&& st, int y, int x): mv(mv), score(score), st(st), y(y), x(x) {}
    ScoredMove(Move&& mv, int score, Status& st, int y, int x): mv(mv), score(score), st(st), y(y), x(x) {}
    ScoredMove(Move& mv, int score, Status& st, int y, int x): mv(mv), score(score), st(st), y(y), x(x) {}
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

inline bool isStoneFixed(Status& st, const int y, const int x) {
    for(uint i = 0; i < 2; i++) {
        auto& fp = st.getField(y, x, i);
        auto& fq = st.getField(y, x, opdir(i));
        if(fp.type == FIELD::FLOOR && fq.type == FIELD::FLOOR) { return false; }
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
inline void moveDog(Status& st, const int y1, const int x1, const int id, D d, bool pre) {
    deleteDog(st, y1, x1);
    const auto& p = dir2p(d);
    newDog(st, y1 + p.y, x1 + p.x, id, pre);
}

template <class D>
inline void backDog(Status& st, const int y1, const int x1, const int id, D d, bool pre) {
    const auto& p = dir2p(d);
    deleteDog(st, y1 + p.y, x1 + p.x, pre);
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

inline int calcAroundDog(Status& st, const int y, const int x) {
    int around = 0;
    for(uint i = 0; i < 8; i++) {
        const auto& p = dir2p8(i);
        if(st.field[y + p.y][x + p.x].dog != -1) { ++around; }
    }
    return around;
}

template <class F>
void dogSim(Status& st, const int y, const int x, const int range, const int limit, bool pre, const F& func) {
    vector<pair<uint, pair<int, Point>>> temp;
    st.updateDistDog(y, x, limit);
    vector<pair<int, Point>> dog;
    st.eachField([&](Field& f) {
        if(f.dog == -1) { return; }
        if(dist(y, x, f.y, f.x) > range) { return; }
        if(st.field[f.y][f.x].distdg == -INF) { return; }
        dog.emplace_back(f.dog, Point{f.y, f.x});
    });
    if(dog.empty()) { func(); return; }
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
            if(!isDogMovable(st, e.second.y, e.second.x, i)) { 
                st.getField(e.second.y, e.second.x, i).threat++;
                continue;
            }
            moveDog(st, e.second.y, e.second.x, e.first, i, pre);
            temp.emplace_back(i, make_pair(e.first, Point{e.second.y, e.second.x}));
            break;
        }
    }
    func();
    for(auto it = temp.rbegin(); it != temp.rend(); ++it) {
        backDog(st, it->second.second.y, it->second.second.x, it->second.first, it->first, pre);
        for(uint i = 0; i < it->first; i++) {
            st.getField(it->second.second.y, it->second.second.x, i).threat--;
        }
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
    if(f.soul) {
        f.soul = false;
        auto it = find_if(st.souls.begin(), st.souls.end(), [&](Soul& s) { return s.y == f.y && s.x == f.x; });
        assert(it != st.souls.end());
        it->reserve = true;
    }
    if(f.type == FIELD::ROCK) { moveStone(st, f.y, f.x, d); }
    f.chara[id] = true;
}

inline int cancelClosed(Status& st, const int y, const int x, Move& mv) {
    queue<pair<Point, int>> que, next;
    next.emplace(Point{y, x}, -1);
    int k = 0;
    for(auto&& d : mv) {
        next.swap(que);
        while(!que.empty()) {
            auto p = que.front(); que.pop();
            auto& f = st.getField(p.first.y, p.first.x, d);
            if(f.type == FIELD::WALL) {
                next.emplace(p.first, p.second);
                continue;
            }
            if(f.type == FIELD::ROCK) {
                auto& ff = st.getField(f.y, f.x, d);
                if(p.second == -1 && ff.type == FIELD::FLOOR && ff.dog == -1 && !ff.chara[0] && !ff.chara[1] && !ff.soul) {
                    next.emplace(p.first, k);
                } else {
                    next.emplace(p.first, p.second);
                }
                continue;
            }
            if(p.second == -1 && f.dog == -1 && !f.chara[0] && !f.chara[1] && !f.soul) {
                if(!isStoneMovable(st, f.y, f.x, d)) {
                    next.emplace(p.first, k);
                    continue;
                }
            }
            next.emplace(Point{f.y, f.x}, p.second);
        }
        ++k;
    }
    while(!next.empty()) {
        auto q = next.front(); next.pop();
        if(st.field[q.first.y][q.first.x].threat) { return q.second; }
    }
    return -1;
}

inline int cancelClosed(Status& st, const uid_t id, Move& mv) {
    auto& ch = st.ninja[id];
    return cancelClosed(st, ch.y, ch.x, mv);
}


template <class D, class F>
inline bool tryMove(Status& st, const int y, const int x, D d, const F& func) {
    if(!isMovable(st, y, x, d)) { return false; }
    auto& f = st.getField(y, x, d);
    bool soul = f.soul;
    if(soul) { f.soul = false; }
    if(f.type == FIELD::ROCK) {
        moveStone(st, f.y, f.x, d);
        func(f.y, f.x, true, soul);
        backStone(st, f.y, f.x, d);
    } else {
        func(f.y, f.x, false, soul);
    }
    if(soul) { f.soul = true; }
    return true;
}

bool checkClosed(Status&, const uid_t, const int, const int, const uint);
inline int evaluate(Status& st, const uid_t id, const int y, const int x) {
    auto& ch = st.ninja[id];
    int score = 0;
    int around = 0;
    if(checkClosed(st, id, y, x, 2)) { score = -INF; }
    uint k = 0;
    for(auto&& si : ch.souls) {
        auto&s = st.souls[si];
        if(st.field[s.y][s.x].soul) {
            if(st.field[s.y][s.x].type == FIELD::ROCK && isStoneFixed(st, s.y, s.x)) {
                score -= 20000;
            } else if(k == 0) {
                score -= min(100, (int)pow(dist(s.y, s.x, y, x), 2));
            } else {
                score += max(0, 100 - (int)pow(dist(s.y, s.x, y, x), 2));
            }
            break;
        }
        ++k;
    }
    return max(score, -INF);
}

void safeMove(vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const uint range, bool val, bool none) {
    if(range == 0) { return; }
    for(uint i = (none ? 4 : 0); i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            auto score = 0;
            if(soul) { score += 10000; }
            if(range > 1) {
                vector<ScoredMove> mv2;
                safeMove(mv2, st, id, yy, xx, range - 1, val, i == 4);
                for(auto&& m : mv2) {
                    m.mv.insert(m.mv.begin(), static_cast<DIR>(i));
                    mv.emplace_back(std::move(m.mv), m.score + score, std::move(m.st), m.y, m.x);
                }
            } else {
                dogSim(st, yy, xx, 4, 4, true, [&]() {
                    score += (val ? evaluate(st, id, yy, xx) : 0);
                    mv.emplace_back(Move{static_cast<DIR>(i)}, score, st, yy, xx);
                });
            }
        });
        //if(!val && !mv.empty()) { break; }
    }
}

inline void safeMove(vector<ScoredMove>& mv, Status& st, const uid_t id, const uint range, bool val) {
    auto& ch = st.ninja[id];
    safeMove(mv, st, id, ch.y, ch.x, range, val, false);
}

inline bool checkClosed(Status& st, const uid_t id, const int y, const int x, const uint range) {
    vector<ScoredMove> mv;
    safeMove(mv, st, id, y, x, range, false, false);
    return mv.empty();
}

inline bool checkClosed(Status& st, const uid_t id, int range) {
    vector<ScoredMove> mv;
    safeMove(mv, st, id, range, false);
    return mv.empty();
}

void safeRoute(vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const uint range, const uint beam, uint depth) {
    vector<ScoredMove> mv1;
    safeMove(mv1, st, id, y, x, range, true, false);
    if(mv1.empty()) { return; }
    for(auto&& mm : mv1) {
        if(cancelClosed(st, y, x, mm.mv) != -1) { mm.score = -INF + 1; }
    }
    sort(mv1.begin(), mv1.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        return p1.score > p2.score;
    });
    if(mv1.size() > beam) { mv1.resize(beam); }
    if(depth <= 1) { return; }
    
    while(true) {
        vector<ScoredMove> next;
        for(auto && m : mv1) {
            vector<ScoredMove> mv2;
            safeMove(mv2, m.st, id, m.y, m.x, range, true, false);
            if(mv2.empty()) {
                mv.emplace_back(std::move(m.mv), -INF, m.st, m.y, m.x);
                continue;
            }
            for(auto&& mm : mv2) {
                if(cancelClosed(st, m.y, m.x, mm.mv) != -1) { mm.score = -INF + 1; }
                Move m1 = m.mv;
                for(auto&& m2 : mm.mv) { m1.push_back(m2); }
                next.emplace_back(std::move(m1), max(m.score + mm.score, -INF), mm.st, mm.y, mm.x);
            }
        }
        if(next.empty()) { break; }
        sort(next.begin(), next.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
            return p1.score > p2.score;
        });
        if(next.size() > beam) { next.resize(beam); }
        --depth;
        if(depth < 1) { break; }
        mv1 = next;
    }
    for(auto&& mm : mv1) { mv.push_back(mm); }
}

void safeRoute(vector<ScoredMove>& mv, Status& st, const uid_t id, const uint range, const uint beam, const uint depth) {
    auto& ch = st.ninja[id];
    safeRoute(mv, st, id, ch.y, ch.x, range, beam, depth);
}

/* main */
void brain(Input& in, Output& ou, const uid_t id, uint limit) {
    Move mv;
    auto& me = in.st[0];
    auto& op = in.st[1];
    auto& ch = me.ninja[id];

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
    auto it = me.souls.begin();
    while(!it->reserve) {
        if(id == 0) {
            auto& ch2 = me.ninja[1];
            if(dist(ch2.y, ch2.x, it->y, it->x) < dist(ch.y, ch.x, it->y, it->x)) { break; }
        }
        it->reserve = true;
        ch.souls.push_back(distance(me.souls.begin(), it));
        ++it;
        if(it == me.souls.end()) { break; }
    }
    
    vector<ScoredMove> mv2;
    safeRoute(mv2, me, id, limit, 7, 7);
    if(mv2.empty()) { return; }
    sort(mv2.begin(), mv2.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        return p1.score > p2.score;
    });
    mv2[0].mv.resize(limit);
    for(auto&& vv : mv2[0].mv) {
        moveChara(me, id, vv);
        mv.push_back(vv);
    }
    
    ou.mv[id] = std::move(mv);
}

bool escapeClosed(Output& ou, Input& in, const uid_t id) {
    auto& st = in.st[0];
    auto& ch = st.ninja[id];
    
    /*if(st.nin >= in.getCost(SKILL::TORNADO)) {
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
    }*/
    
    if(st.nin >= in.getCost(SKILL::SONIC) && !checkClosed(st, id, 3)) {
        ou.sk = SKILL::SONIC;
        return true;
    }
    /*if(st.nin >= in.getCost(SKILL::MYTHUNDER)) {
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
    }*/
    if(st.nin >= in.getCost(SKILL::MYGHOST)) {
        vector<Point> cond ={{1, 1},{st.h -2 , 1},{st.h - 2, st.w - 2},{1, st.w - 2}};
        bool suc = false;
        for(auto&& c : cond) {
            if(st.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
            dogSim(st, c.y, c.x, INF, INF, false, [&]() {
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
    if(in.time == 300000) {
        prevop = op;
    }

    if(!strict && prevop.cnt[static_cast<uint>(SKILL::MYGHOST)] < op.cnt[static_cast<uint>(SKILL::MYGHOST)]) {
        for(auto&& nj : op.ninja) {
            if(prevop.field[nj.y][nj.x].threat > 0) { strict = true; break; }
        }
    }
    prevop = op;
    
    uint costmin = 0; //min(12u, in.getCost(SKILL::TORNADO)) + in.getCost(SKILL::OPMATEOR);
    
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
    
    if(me.nin >= in.getCost(SKILL::OPMATEOR)) {
        for(auto& nj : op.ninja) {
            vector<ScoredMove> mv;
            DIR d = DIR::NONE;
            safeMove(mv, op, nj.id, 2, false);
            if(mv.size() == 1) {
                auto k = cancelClosed(op, nj.id, mv[0].mv);
                if(k == -1) { continue; }
                int y = nj.y, x = nj.x;
                for(int i = 0; i < k; i++) {
                    auto& f = op.getField(y, x, mv[0].mv[i]);
                    y = f.y; x = f.x;
                }
                auto& f = op.getField(y, x, mv[0].mv[k]);
                ou.val1 = f.y;
                ou.val2 = f.x;
                ou.sk = SKILL::OPMATEOR;
                return;
            } else {
                for(auto&& m : mv) {
                    if(cancelClosed(op, nj.id, m.mv) != 0) { d = DIR::NONE; break; }
                    if(d == DIR::NONE) { d = m.mv[0]; }
                    else if(m.mv[0] != d) { d = DIR::NONE; break; }
                }
                if(d == DIR::NONE) { continue; }
                auto& f = op.getField(nj.y, nj.x, d);
                ou.val1 = f.y;
                ou.val2 = f.x;
                ou.sk = SKILL::OPMATEOR;
                return;
            }
        }
    }

    if(me.nin >= in.getCost(SKILL::OPMATEOR) + costmin) {
        for(auto& s : op.souls) {
            DIR dir = DIR::NONE;
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
            if(!isStoneFixed(op, s.y, s.x)) { continue; }
            auto& f = op.getField(s.y, s.x, opdir(dir));
            if(f.type != FIELD::FLOOR || f.soul || f.dog != -1) { continue; }
            if(f.chara[0] || f.chara[1]) { continue; }
            ou.val1 = f.y;
            ou.val2 = f.x;
            ou.sk = SKILL::OPMATEOR;
            return;
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
                if(fp.distch > 2 && fq.distch > 2) { continue; }
                if(fp.distch > 2 && fp.dog == -1) { continue; }
                if(fq.distch > 2 && fq.dog == -1) { continue; }
                if(abs(fp.distch - fq.distch) > maxd) {
                    ou.val1 = f.y;
                    ou.val2 = f.x;
                    maxd = abs(fp.distch - fq.distch);
                }
            }
        });
        if(maxd > 15) {
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
    
    if(me.nin >= in.getCost(SKILL::TORNADO)) {
        for(auto&& ch : me.ninja) {
            if(in.getCost(SKILL::TORNADO) <= calcAroundDog(me, ch.y, ch.x) * 4) {
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
    /*if(me.nin >= in.getCost(SKILL::SONIC) + costmin && in.getCost(SKILL::SONIC) <= 2) {
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
                dogSim(me, c.y, c.x, INF, INF, false, [&]() {
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
        dogSim(me, ou.val1, ou.val2, INF, INF, false, [&]() {
            brain(in, ou, 0, 2);
            brain(in, ou, 1, 2);
        });
        return std::move(ou);
    }

    // chara0
    brain(in, ou, 0, ou.sk == SKILL::SONIC ? 3 : 2);

    // chara1
    //if(!settle && checkClosed(me, 1, 2)) { settle = escapeClosed(ou, in, 1); }
    brain(in, ou, 1, ou.sk == SKILL::SONIC ? 3 : 2);

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
