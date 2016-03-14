// nya-n
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <queue>
#include <algorithm>
#include <iterator>
#include <tuple>
#include <cassert>

using namespace std; // (>_<)

/* type */
typedef unsigned int uint;
typedef uint uid_t;

struct Point { 
    int y = -1, x = -1;
    Point() = default;
    Point(int y, int x): y(y), x(x) {} 
};

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
    int fary = -1, farx = -1, fard = -INF;
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
                field[f.y][f.x].distdg = q.first + 1;
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
    SKILL sk = SKILL::NONE;
    uint val1 = 0, val2 = 0;
    bool cancel = false;
    ScoredMove() = default;
    ScoredMove(Move&& mv, int score, Status&& st, int y, int x, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), y(y), x(x), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
    ScoredMove(Move&& mv, int score, Status& st, int y, int x, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), y(y), x(x), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
    ScoredMove(Move& mv, int score, Status& st, int y, int x, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), y(y), x(x), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
};

struct Input {
    uint time;
    vector<uint> costs;
    Status st[2];
    inline const uint getCost(SKILL sk) const { return costs[static_cast<uint>(sk)]; }
};

struct Output {
    SKILL sk = SKILL::NONE;
    int val1 = 0, val2 = 0;
    Move mv[2];
    bool settle = false;
    bool cancel = false;
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
        if(fp.type == FIELD::FLOOR && fq.type == FIELD::FLOOR) {
            if(fp.soul && !isStoneMovable(st, fp.y, fp.x, i)) { continue; }
            if(fq.soul && !isStoneMovable(st, fq.y, fq.x, opdir(i))) { continue; }
            if(st.field[fp.y][fp.x].distch == -INF && st.field[fq.y][fq.x].distch == -INF) { continue; }
            return false;
        }
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
                if(!pre) { st.getField(e.second.y, e.second.x, i).threat++; }
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
        if(!pre) { 
            for(uint i = 0; i < it->first; i++) {
                st.getField(it->second.second.y, it->second.second.x, i).threat--;
            }
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
        it->reserve = true;
    }
    if(f.type == FIELD::ROCK) { moveStone(st, f.y, f.x, d); }
    f.chara[id] = true;
}

inline int cancelClosed(Status& st, const int y, const int x, Move& mv, Point& p, bool th = false) {
    queue<tuple<Point, int, Point, Point, DIR>> que, next;
    next.emplace(Point{y, x}, -1, Point{-1, -1}, Point{-1, -1}, DIR::NONE);
    int k = 0;
    for(auto&& d : mv) {
        next.swap(que);
        while(!que.empty()) {
            auto q = que.front(); que.pop();
            auto& f = st.getField(get<0>(q).y, get<0>(q).x, d);
            if(f.type == FIELD::WALL) {
                next.emplace(get<0>(q), get<1>(q), get<2>(q), get<3>(q), get<4>(q) == d ? d : DIR::NONE);
                continue;
            }
            auto pp = get<3>(q);
            if(f.type == FIELD::ROCK || get<4>(q) == d) {
                auto& ff = st.getField(f.y, f.x, d);
                if((get<1>(q) == -1 || (pp.y == ff.y && pp.x == ff.x))
                    && ff.type == FIELD::FLOOR && ff.dog == -1 && !ff.chara[0] && !ff.chara[1] && !ff.soul) {
                    if(th) { next.emplace(get<0>(q), k, Point{f.y, f.x}, Point{ff.y, ff.x}, d); }
                    else { next.emplace(get<0>(q), k, Point{ff.y, ff.x}, Point{ff.y, ff.x}, d); }
                } else if(ff.type == FIELD::FLOOR && ff.dog == -1 && !ff.chara[0] && !ff.chara[1]) {
                    next.emplace(Point{f.y, f.x}, get<1>(q), get<2>(q), get<3>(q), d);
                } else {
                    next.emplace(get<0>(q), get<1>(q), get<2>(q), get<3>(q), d);
                }
                continue;
            }
            if((get<1>(q) == -1 || (pp.y == f.y && pp.x == f.x)) 
                && f.dog == -1 && !f.chara[0] && !f.chara[1] && !f.soul) {
                auto& ff = st.getField(f.y, f.x, d);
                if(!isStoneMovable(st, f.y, f.x, d)) {
                    if(th && ff.type == FIELD::ROCK) {
                        next.emplace(get<0>(q), k, Point{ff.y, ff.x}, Point{f.y, f.x}, d);
                    } else {
                        next.emplace(get<0>(q), k, Point{f.y, f.x}, Point{f.y, f.x}, d);
                    }
                }
            }
            next.emplace(Point{f.y, f.x}, get<1>(q), get<2>(q), get<3>(q), get<4>(q) == d ? d : DIR::NONE);
        }
        ++k;
    }
    while(!next.empty()) {
        auto q = next.front(); next.pop();
        auto pq = get<0>(q);
        auto th = st.field[pq.y][pq.x].threat;
        if(th) { p = get<2>(q); return get<1>(q); }
    }
    return -1;
}

inline int cancelClosed(Status& st, const uid_t id, Move& mv, Point& p) {
    auto& ch = st.ninja[id];
    return cancelClosed(st, ch.y, ch.x, mv, p);
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
            auto d = dist(s.y, s.x, y, x);
            if(st.field[s.y][s.x].type == FIELD::ROCK && isStoneFixed(st, s.y, s.x)) {
                score -= 20000;
            } else if(k == 0) {
                score -= min(100, (int)pow(d, 2));
            } else {
                score += max(0, 100 - (int)pow(d, 2));
            }
            break;
        }
        ++k;
    }
    for(auto dy = max(ch.y - 2, 1); dy <= min(ch.y + 2, st.h - 2); dy++) {
        auto rx = 2 - abs(ch.y - dy);
        for(auto dx = max(ch.x - rx, 1); dx <= min(ch.x + rx, st.w - 2); dx++) {
            auto& f = st.field[dy][dx];
            if(f.dog) { ++around; }
        }
    }
    score -= around * 10;
    return max(score, -INF);
}

void safeMove(vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const uint range, const uint limit, const uint depth, bool val, bool none) {
    auto& ch = st.ninja[id];
    if(range == 0) { return; }
    for(uint i = (none ? 4 : 0); i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            auto score = 0;
            if(soul) { score += 10000 * depth; }
            if(range > 1) {
                vector<ScoredMove> mv2;
                safeMove(mv2, st, id, yy, xx, range - 1, limit, depth, val, i == 4);
                for(auto&& m : mv2) {
                    m.mv.insert(m.mv.begin(), static_cast<DIR>(i));
                    mv.emplace_back(std::move(m.mv), m.score + score, std::move(m.st), m.y, m.x);
                }
            } else {
                if(yy == ch.y && x == ch.x) { score -= 100; } 
                auto lim = limit + 2;
                dogSim(st, yy, xx, lim, lim, true, [&]() {
                    score += (val ? evaluate(st, id, yy, xx) : 0);
                    mv.emplace_back(Move{static_cast<DIR>(i)}, score, st, yy, xx);
                });
            }
        });
    }
}

inline void safeMove(vector<ScoredMove>& mv, Status& st, const uid_t id, const uint range, const uint limit, const uint depth, bool val) {
    auto& ch = st.ninja[id];
    safeMove(mv, st, id, ch.y, ch.x, range, limit, depth, val, false);
}

void fakeMove(vector<ScoredMove>& mv, Status& st, Status& ost, const uid_t id, const int y, const int x, const uint range, const uint limit, const uint depth, bool val, bool none) {
    auto& ch = st.ninja[id];
    if(range == 0) { return; }
    for(uint i = (none ? 4 : 0); i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(stone) { return; }
            if(range == 1 && st.field[yy][xx].threat) { return; }
            if(range == 1 && !ost.field[yy][xx].threat) { return; }
            if(range == 1 && yy == ch.y && xx == ch.x) { return; }
            auto score = 0;
            if(soul) { score += 10000 * depth; }
            if(range > 1) {
                vector<ScoredMove> mv2;
                fakeMove(mv2, st, ost, id, yy, xx, range - 1, limit, depth, val, i == 4);
                for(auto&& m : mv2) {
                    m.mv.insert(m.mv.begin(), static_cast<DIR>(i));
                    mv.emplace_back(std::move(m.mv), m.score + score, std::move(m.st), m.y, m.x);
                }
            } else {
                auto lim = limit + 2;
                score += (val ? evaluate(st, id, yy, xx) : 0);
                mv.emplace_back(Move{static_cast<DIR>(i)}, score, st, yy, xx);
            }
        });
    }
}

inline bool checkClosed(Status& st, const uid_t id, const int y, const int x, const uint range) {
    vector<ScoredMove> mv;
    safeMove(mv, st, id, y, x, range, range, 1, false, false);
    return mv.empty();
}

inline bool checkClosed(Status& st, const uid_t id, int range) {
    vector<ScoredMove> mv;
    safeMove(mv, st, id, range, range, 1, false);
    return mv.empty();
}

inline bool checkClosed2(Status& st, const uid_t id, int range) {
    vector<ScoredMove> mv;
    safeMove(mv, st, id, range, range, 1, true);
    if(mv.empty()) { return true; }
    return all_of(mv.begin(), mv.end(), [](ScoredMove& sm) { return sm.score <= -INF; });
}

bool greedRoute(Status& st, const int sy, const int sx, const int dy, const int dx) {
    if(sy == dy && sx == dx) { return true; }
    vector<DIR> cond;
    if(sy != dy) { cond.push_back(sy < dy ? DIR::DOWN : DIR::UP); }
    if(sx != dx) { cond.push_back(sx < dx ? DIR::RIGHT : DIR::LEFT); }
    bool res = false;
    for(auto&& c : cond) {
        auto& f = st.getField(sy, sx, c);
        tryMove(st, sy, sx, c, [&](const int yy, const int xx, bool stone, bool soul) {
            if(yy == dy && xx == dx) { res = true; return; }
            res = greedRoute(st, yy, xx, dy, dx);
        });
        if(res) { break; }
    }
    return res;
}

template <class F>
bool greedNeck(Output& ou, Status& st, const int sy, const int sx, const int dy, const int dx, const F&& func) {
    if(greedRoute(st, sy, sx, dy, dx)) { return true; }
    for(int y = min(sy, dy); y <= max(sy, dy); y++) {
        for(int x = min(sx, dx); x <= max(sx, dx); x++) {
            auto& f = st.field[y][x];
            if(f.type != FIELD::ROCK) { continue; }
            if(!isStoneFixed(st, f.y, f.x)) { continue; }
            bool exec = true;
            for(uint i = 0; i < 2; i++) {
                auto& fp = st.getField(f.y, f.x, i);
                if(fp.type != FIELD::FLOOR) { continue; }
                auto& fq = st.getField(f.y, f.x, opdir(i));
                if(fq.type != FIELD::FLOOR) { continue; }
                if(fp.dog != -1 || fq.dog != -1) { exec = false; break; }
                if(fp.distch == -INF || fq.distch == -INF) { break; }
                if(abs(fp.distch - fq.distch) > 15) { exec = false; break; }
            }
            if(!exec) { continue; }
            bool fin = false;
            deleteStone(st, y, x);
            if(greedRoute(st, sy, sx, dy, dx)) {
                fin = true;
                func(y, x);
                /*ou.sk = SKILL::MYTHUNDER;
                ou.val1 = y;
                ou.val2 = x;
                ou.cancel = true;
                return true;*/
            }
            newStone(st, y, x);
            if(fin) { return true; }
        }
    }
    return false;
}

void safeRoute(Input& in, Output& ou, vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const uint range, const uint beam, uint depth) {
    vector<ScoredMove> mv1;
    auto& ch = st.ninja[id];
    safeMove(mv1, st, id, y, x, range, range, depth, true, false);
    if(mv1.empty()) { return; }
    bool flag = false;
    if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)) {
        bool exec = ((ou.sk == SKILL::NONE || !ou.settle) && st.nin >= in.getCost(SKILL::MYTHUNDER));
        vector<Point> neckList;
        int neckc = 0;
        for(auto&& mm : mv1) {
            if(id == 0 && checkClosed(mm.st, 1, 2)) { continue; }
            Point neck;
            if(cancelClosed(st, y, x, mm.mv, neck, true) == -1) { continue; }
            flag = true;
            ++neckc;
            if(find_if(neckList.begin(), neckList.end(), [&](const Point& p) {
                return p.y == neck.y && p.x == neck.x;
            }) == neckList.end()) {
                neckList.push_back(neck);
            }
            if(exec) { mm.sk = SKILL::MYTHUNDER; mm.val1 = neck.y; mm.val2 = neck.x; }
        }
        if(neckList.size() == 1 || st.nin >= in.getCost(SKILL::MYTHUNDER) * 4) {
            for(auto&& mm : mv1) {
                if(mm.sk == SKILL::MYTHUNDER) { mm.score -= 50000; }
            }
        } else if(neckc > 0) {
            for(auto&& mm : mv1) {
                if(mm.sk == SKILL::MYTHUNDER) { mm.sk = SKILL::MYTHUNDER; mm.score -= INF / 4; }
            }
        }
    }
    if(ou.sk == SKILL::NONE
        && st.nin >= in.getCost(SKILL::MYTHUNDER) + min(10u, in.getCost(SKILL::TORNADO))
        && !ch.souls.empty()) {
        auto& s = st.souls[ch.souls[0]];
        if(dist(ch.y, ch.x, s.y, s.x) <= 2) {
            greedNeck(ou, st, ch.y, ch.x, s.y, s.x, [&](int yy, int xx) {
                vector<ScoredMove> mv4;
                safeMove(mv4, st, id, y, x, range, range, depth, true, false);
                if(mv4.empty()) { return; }
                for(auto&& mm : mv4) {
                    Point neck;
                    if(cancelClosed(st, y, x, mm.mv, neck, true) != -1) { continue; }
                    if(mm.score < 10000) { continue; }
                    mm.sk = SKILL::MYTHUNDER; mm.val1 = yy; mm.val2 = xx; mm.cancel = true; 
                    mv1.push_back(mm);
                }
            });
        }
    }

    if(id == 1 && !flag && (ou.sk == SKILL::NONE || !ou.settle) 
        && st.nin >= in.getCost(SKILL::MYGHOST) + min(10u, in.getCost(SKILL::TORNADO))
        && calcAroundDog(st, y, x) > 0) {
        vector<Point> cond ={{st.ninja[0].y, st.ninja[0].x},{st.ninja[1].y, st.ninja[1].x},{1, 1},{st.h -2 , 1},{st.h - 2, st.w - 2},{1, st.w - 2}};
        Status ost = st;
        for(auto&& c : cond) {
            if(st.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
            dogSim(st, c.y, c.x, INF, INF, false, [&]() {
                vector<ScoredMove> mv3;
                fakeMove(mv3, st, ost, id, y, x, range, range, depth, true, false);
                for(auto&& mm : mv3) {
                    Point neck;
                    if(cancelClosed(st, y, x, mm.mv, neck, true) != -1) { continue; }
                    if(mm.score < 10000) { continue; }
                    mm.sk = SKILL::MYGHOST; mm.val1 = c.y; mm.val2 = c.x;
                    mv1.push_back(mm);
                }
            });
        }
    }
    sort(mv1.begin(), mv1.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        if(p1.score == p2.score) {
            if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
            if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
            return p1.mv.size() > p2.mv.size();
        }
        return p1.score > p2.score;
    });
    if(mv1.size() > beam) { mv1.resize(beam); }
    if(depth <= 1) { return; }
    
    while(true) {
        --depth;
        vector<ScoredMove> next;
        for(auto && m : mv1) {
            vector<ScoredMove> mv2;
            safeMove(mv2, m.st, id, m.y, m.x, range, range, depth, true, false);
            if(mv2.empty()) {
                mv.emplace_back(std::move(m.mv), -INF, m.st, m.y, m.x, m.sk, m.val1, m.val2, m.cancel);
                continue;
            }
            for(auto&& mm : mv2) {
                Point neck;
                if(cancelClosed(st, m.y, m.x, mm.mv, neck) != -1) { mm.score -= 70000; }
                Move m1 = m.mv;
                for(auto&& m2 : mm.mv) { m1.push_back(m2); }
                next.emplace_back(std::move(m1), max(m.score + mm.score, -INF), mm.st, mm.y, mm.x, m.sk, m.val1, m.val2, m.cancel);
            }
        }
        if(next.empty()) { break; }
        sort(next.begin(), next.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
            if(p1.score == p2.score) { 
                if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
                if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
                return p1.mv.size() > p2.mv.size();
            }
            return p1.score > p2.score;
        });
        if(next.size() > beam) { next.resize(beam); }
        mv1 = next;
        if(depth <= 1) { break; }
    }
    for(auto&& mm : mv1) { mv.push_back(mm); }
}

void safeRoute(Input& in, Output& ou, vector<ScoredMove>& mv, Status& st, const uid_t id, const uint range, const uint beam, const uint depth) {
    auto& ch = st.ninja[id];
    safeRoute(in, ou, mv, st, id, ch.y, ch.x, range, beam, depth);
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
        if(!(dist(s1.y, s1.x, me.ninja[id].y, me.ninja[id].x) < limit)
            && !(dist(s2.y, s2.x, me.ninja[id].y, me.ninja[id].x) < limit)) {
            if(me.field[s1.y][s1.x].threat && !me.field[s2.y][s2.x].threat) { return false; }
            if(!me.field[s1.y][s1.x].threat && me.field[s2.y][s2.x].threat) { return true; }
        }
        if(me.nin < in.getCost(SKILL::MYTHUNDER) + min(10u, in.getCost(SKILL::TORNADO))) {
            if(me.field[s1.y][s1.x].distch == -INF && me.field[s2.y][s2.x].distch != -INF) { return false; }
            if(me.field[s1.y][s1.x].distch != -INF && me.field[s2.y][s2.x].distch == -INF) { return true; }
        }
        if(id == 0) {
            auto& ch2 = me.ninja[1];
            if(dist(s1.y, s1.x, ch2.y, ch2.x) < dist(s1.y, s1.x, ch.y, ch.x)) { return false; }
            if(dist(s2.y, s2.x, ch2.y, ch2.x) < dist(s2.y, s2.x, ch.y, ch.x)) { return true; }
        }
        return dist(s1.y, s1.x, ch.y, ch.x) < dist(s2.y, s2.x, ch.y, ch.x);
    });
    auto it = me.souls.begin();
    while(it != me.souls.end() && !it->reserve) {
        if(id == 0) {
            auto& ch2 = me.ninja[1];
            if(dist(ch2.y, ch2.x, it->y, it->x) < dist(ch.y, ch.x, it->y, it->x)) { ++it; continue; }
        }
        it->reserve = true;
        ch.souls.push_back(distance(me.souls.begin(), it));
        ++it;
    }
    
    vector<ScoredMove> mv2;
    safeRoute(in, ou, mv2, me, id, limit, 10, 7);
    if(mv2.empty()) { return; }
    sort(mv2.begin(), mv2.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        if(p1.score == p2.score) { 
            if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
            if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
            return p1.mv.size() > p2.mv.size();
        }
        return p1.score > p2.score;
    });
    mv2[0].mv.resize(limit);
    for(auto&& vv : mv2[0].mv) {
        moveChara(me, id, vv);
        mv.push_back(vv);
    }
    if(mv2[0].sk != SKILL::NONE) {
        ou.sk = mv2[0].sk;
        ou.val1 = mv2[0].val1;
        ou.val2 = mv2[0].val2;
        ou.cancel = mv2[0].cancel;
    }

    /*
    if(ou.sk == SKILL::NONE 
        && me.nin >= in.getCost(SKILL::MYTHUNDER) + min(10u, in.getCost(SKILL::TORNADO)) 
        && !ch.souls.empty()) {
        auto& s = me.souls[ch.souls[0]];
        if(dist(ch.y, ch.x, s.y, s.x) <= 4) { greedNeck(ou, me, ch.y, ch.x, s.y, s.x); }
    }
    */
    if(ou.sk == SKILL::NONE 
        && me.nin >= in.getCost(SKILL::MYGHOST)  + min(10u, in.getCost(SKILL::TORNADO))
        && mv2[0].score < -INF / 4
        && in.getCost(SKILL::MYGHOST) < in.getCost(SKILL::MYTHUNDER)) {
        int far = -INF, fary = -1, farx = -1;
        me.eachField([&](Field& f) {
            if(f.distch > far) {
                far = f.distch; fary = f.y; farx = f.x;
            }
        });
        if(far > 15) {
            ou.val1 = fary;
            ou.val2 = farx;
            ou.sk = SKILL::MYGHOST;
            ou.cancel = true;
        }
    }

    ou.mv[id] = std::move(mv);
}

bool escapeClosed(Output& ou, Input& in, const uid_t id) {
    auto& st = in.st[0];
    auto& ch = st.ninja[id];
    
    if(st.nin >= in.getCost(SKILL::TORNADO)) {
        auto dogs = calcAroundDog(st, ch.y, ch.x);
        bool exec = in.getCost(SKILL::TORNADO) <= dogs * 3;
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
                if(!checkClosed2(st, id, 2)) {
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
        vector<Point> cond ={{st.ninja[0].y, st.ninja[0].x},{st.ninja[1].y, st.ninja[1].x},{1, 1},{st.h -2 , 1},{st.h - 2, st.w - 2},{1, st.w - 2}};
        bool suc = false;
        for(auto&& c : cond) {
            if(st.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
            dogSim(st, c.y, c.x, INF, INF, false, [&]() {
                if(!checkClosed2(st, id, 2)) {
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
    static auto ghostcnt = 0;
    static bool strict = false;
    if(in.time == 300000) {
        prevop = op;
        ghostcnt = 0;
        strict = false;
    }

    if(!strict && prevop.cnt[static_cast<uint>(SKILL::MYGHOST)] < op.cnt[static_cast<uint>(SKILL::MYGHOST)]) {
        for(auto&& nj : op.ninja) {
            if(prevop.field[nj.y][nj.x].threat > 0 && op.nin >= prevop.nin + 2 - in.getCost(SKILL::MYGHOST)) {
                ++ghostcnt;
                if(ghostcnt >= 2) { strict = true; }
                break;
            }
        }
    }
    prevop = op;
    
    uint costmin = in.getCost(SKILL::MYTHUNDER);
    
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
            int r = 3;
            for(auto& s : op.souls) {
                if(!op.field[s.y][s.x].threat) {
                    if(any_of(op.ninja.begin(), op.ninja.end(), [&](const Chara& nj) {
                        return dist(nj.y, nj.x, s.y, s.x) <= 2;
                    })) { r = INF; break; } 
                    continue;
                }
                //if(calcAroundDog(op, s.y, s.x) < 2) { continue; }
                for(auto& nj : op.ninja) {
                    auto d = dist(nj.y, nj.x, s.y, s.x);
                    if(d < r) {
                        if(d != 1) {
                            ou.val1 = s.y;
                            ou.val2 = s.x;
                        } else {
                            int th = 5;
                            op.eachAround(s.y, s.x, 5, [&](Field& f) {
                                if(f.threat < th) {
                                    ou.val1 = f.y;
                                    ou.val2 = f.x;
                                    th = f.threat;
                                }
                            });
                        }
                        r = d;
                    }
                }
            }
            if(r < 3) { ou.sk = SKILL::OPGHOST; return; }
        }
    }
    if(me.nin >= in.getCost(SKILL::OPMATEOR)) {
        for(auto& nj : op.ninja) {
            vector<ScoredMove> mv;
            safeMove(mv, op, nj.id, 2, 2, 1, false);
            vector<Point> neckList;
            bool exec = true;
            for(auto&& mm : mv) {
                Point neck;
                if(cancelClosed(op, nj.id, mm.mv, neck) == -1) { exec = false; break; }
                if(find_if(neckList.begin(), neckList.end(), [&](const Point& p) {
                    return p.y == neck.y && p.x == neck.x;
                }) == neckList.end()) {
                    neckList.push_back(neck);
                }
            }
            if(!exec) { continue; }
            if(neckList.size() == 1) {
                auto& neck = neckList[0];
                ou.val1 = neck.y;
                ou.val2 = neck.x;
                ou.sk = SKILL::OPMATEOR;
                return;
            } else if(neckList.size() <= 2 && in.getCost(SKILL::OPGHOST) < in.getCost(SKILL::OPMATEOR)) {
                ou.val1 = nj.y;
                ou.val2 = nj.x;
                ou.sk = SKILL::OPGHOST;
            }
        }
    }
    if(me.nin >= in.getCost(SKILL::MYTHUNDER)) {
        for(auto& s : me.souls) {
            if(!s.inRock) { continue; }
            if(!isStoneFixed(me, s.y, s.x)) { continue; }
            bool exec = false;
            for(auto& nj : me.ninja) {
                if(dist(nj.y, nj.x, s.y, s.x) <= 2) { exec = true; break; }
            }
            if(!exec) { continue; }
            ou.val1 = s.y;
            ou.val2 = s.x;
            ou.sk = SKILL::MYTHUNDER;
            return;
        }
    }
    if(me.nin >= in.getCost(SKILL::OPMATEOR) + costmin 
        && in.getCost(SKILL::OPMATEOR) <= 5 
        && in.getCost(SKILL::MYTHUNDER) > in.getCost(SKILL::OPMATEOR)) {
        for(auto& nj : op.ninja) {
            DIR dir = DIR::NONE;
            for(auto& s : op.souls) {
                if(op.field[s.y][s.x].threat) { continue; }
                if(dist(nj.y, nj.x, s.y, s.x) == 1) { dir = DIR::NONE; break; }
                if(op.field[s.y][s.x].type == FIELD::ROCK && isStoneFixed(op, s.y, s.x)) { continue; }
                if(!isStoneFixed(op, s.y, s.x)) { continue; }
                if(dist(nj.y, nj.x, s.y, s.x) == 2 && (nj.y == s.y || nj.x == s.x)) {
                    if(nj.y > s.y) { dir = DIR::UP; }
                    else if(nj.y < s.y) { dir = DIR::DOWN; } 
                    else if(nj.x > s.x) { dir = DIR::LEFT; }
                    else if(nj.x < s.x) { dir = DIR::RIGHT; }
                }
            }
            if(dir == DIR::NONE) { continue; }
            auto& f = op.getField(nj.y, nj.x, dir);
            if(f.type != FIELD::FLOOR || f.soul || f.dog != -1) { continue; }
            if(f.chara[0] || f.chara[1]) { continue; }
            ou.val1 = f.y;
            ou.val2 = f.x;
            ou.sk = SKILL::OPMATEOR;
            return;
        }
    }
    if(me.nin >= in.getCost(SKILL::OPTHUNDER) + costmin && op.dogs.size() > 5 && in.getCost(SKILL::OPTHUNDER) <= 2) {
        int maxd = -INF;
        op.eachField([&](Field& f) {
            if(f.type != FIELD::ROCK) { return; }
            for(uint i = 0; i < 3; i++) {
                for(uint j = i + 1; j < 4; j++) {
                    auto& fp = op.getField(f.y, f.x, i);
                    if(fp.type != FIELD::FLOOR || fp.distch == -INF) { continue; }
                    auto& fq = op.getField(f.y, f.x, j);
                    if(fq.type != FIELD::FLOOR || fq.distch == -INF) { continue; }
                    if(!((fp.distch <= 3 && op.fard - fq.distch <= 2) 
                        || (fq.distch <= 3 && op.fard - fp.distch <= 2))) { continue; }
                    if(abs(fp.distch - fq.distch) > maxd) {
                        ou.val1 = f.y;
                        ou.val2 = f.x;
                        maxd = abs(fp.distch - fq.distch);
                    }
                }
            }
        });
        if(maxd > 15) {
            ou.sk = SKILL::OPTHUNDER;
            return;
        }
    }
}

Output resolve(Input in) {
    Output ou;
    auto& me = in.st[0];
    auto& op = in.st[1];

    for(auto& ch : me.ninja) { ch.closed = checkClosed(me, ch.id, 2); }
    
    if(me.ninja[0].closed && me.ninja[1].closed) {
        if(me.nin >= in.getCost(SKILL::SONIC) && in.getCost(SKILL::SONIC) < in.getCost(SKILL::MYGHOST) &&
            !checkClosed2(me, 0, 3) && !checkClosed2(me, 1, 3)) {
            ou.sk = SKILL::SONIC;
            ou.settle = true;
        } else if(me.nin >= in.getCost(SKILL::MYGHOST)) {
            vector<Point> cond ={{me.ninja[0].y, me.ninja[0].x},{me.ninja[1].y, me.ninja[1].x},{1, 1},{me.h -2 , 1},{me.h - 2, me.w - 2},{1, me.w - 2}};
            bool suc = false;
            for(auto&& c : cond) {
                if(me.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
                dogSim(me, c.y, c.x, INF, INF, false, [&]() {
                    if(!checkClosed2(me, 0, 2) && !!checkClosed2(me, 1, 2)) {
                        ou.val1 = c.y;
                        ou.val2 = c.x;
                        suc = true;
                    }
                });
                if(suc) { break; }
            }
            if(suc) { ou.sk = SKILL::MYGHOST; ou.settle = true; }
        }
    }
    if(!ou.settle && me.ninja[0].closed) { ou.settle = escapeClosed(ou, in, 0); }
    if(!ou.settle && me.ninja[1].closed) { ou.settle = escapeClosed(ou, in, 1); }
    if(!ou.settle) { findCritical(ou, in); }
    
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
    if(!ou.settle && checkClosed(me, 1, 2)) { ou.settle = escapeClosed(ou, in, 1); }
    if(ou.sk != SKILL::NONE && !ou.cancel) { ou.settle = true; }
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
            if(st.fard < q.first) {
                st.fard = q.first;
                st.fary = q.second.y;
                st.farx = q.second.x;
            }
            st.eachAround(q.second.y, q.second.x, 4, [&](Field& f) {
                if(f.type != FIELD::FLOOR) { return; }
                if(f.distch != -INF && f.distch <= q.first + 1) { return; }
                st.field[f.y][f.x].distch = q.first + 1;
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
