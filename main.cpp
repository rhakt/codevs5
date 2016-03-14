// nya-n
#include <iostream>
#include <vector>
#include <array>
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
    inline const uint getCnt(SKILL sk) const { return cnt[static_cast<uint>(sk)]; }
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
    inline void updateDistDog(const int y1, const int x1, const int y2, const int x2, const int range) {
        eachField([](Field& f) { f.distdg = -INF; });
        queue<pair<int, Point>> que;
        que.emplace(0, Point{y1, x1});
        que.emplace(0, Point{y2, x2});
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
    array<Move, 2> mv = {Move{DIR::NONE}, Move{DIR::NONE}};
    array<int, 2> score = {-INF, -INF};
    Status st;
    array<Point, 2> ch;
    SKILL sk = SKILL::NONE;
    uint val1 = 0, val2 = 0;
    bool cancel = false;
    ScoredMove() = default;
    ScoredMove(array<Move, 2>&& mv, array<int, 2>& score, Status&& st, array<Point, 2> ch, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), ch(ch), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
    ScoredMove(array<Move, 2>&& mv, array<int, 2>& score, Status& st, array<Point, 2> ch, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), ch(ch), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
    ScoredMove(array<Move, 2>& mv, array<int, 2>& score, Status& st, array<Point, 2> ch, SKILL sk = SKILL::NONE, uint val1 = -1, uint val2 = -1, bool cancel = false)
        : mv(mv), score(score), st(st), ch(ch), sk(sk), val1(val1), val2(val2), cancel(cancel) {}
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
void dogSim(Status& st, const int y1, const int x1, const int y2, const int x2, const int range, const int limit, bool pre, const F& func) {
    vector<pair<uint, pair<int, Point>>> temp;
    st.updateDistDog(y1, x1, y2, x2, limit);
    vector<pair<int, Point>> dog;
    st.eachField([&](Field& f) {
        if(f.dog == -1) { return; }
        if(dist(y1, x1, f.y, f.x) > range && dist(y2, x2, f.y, f.x) > range) { return; }
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

bool greedRoute(Status& st, const int sy, const int sx, const int dy, const int dx) {
    if(sy == dy && sx == dx) { return true; }
    vector<DIR> cond;
    if(sy != dy) { cond.push_back(sy < dy ? DIR::DOWN : DIR::UP); }
    if(sx != dx) { cond.push_back(sx < dx ? DIR::RIGHT : DIR::LEFT); }
    bool res = false;
    for(auto&& c : cond) {
        auto& f = st.getField(sy, sx, c);
        if(f.threat) { continue; }
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
            if(dist(sy, sx, y, x) > 2) { continue; }
            auto& f = st.field[y][x];
            if(f.type != FIELD::ROCK) { continue; }
            if(!isStoneFixed(st, f.y, f.x)) { continue; }
            bool exec = true;
            for(uint i = 0; i < 2; i++) {
                auto& fp = st.getField(f.y, f.x, i);
                auto& fq = st.getField(f.y, f.x, opdir(i));
                if(fp.dog != -1 || fq.dog != -1) { exec = false; break; }
            }
            if(!exec) { continue; }
            bool fin = false;
            deleteStone(st, y, x);
            if(greedRoute(st, sy, sx, dy, dx)) {
                fin = true;
                func(y, x);
            }
            newStone(st, y, x);
            if(fin) { return true; }
        }
    }
    return false;
}

inline int evaluate(Status& st, const uid_t id, const int y, const int x) {
    auto& ch = st.ninja[id];
    int score = 0;
    int around = 0;
    uint k = 0;
    for(auto&& si : ch.souls) {
        auto&s = st.souls[si];
        if(st.field[s.y][s.x].soul) {
            auto d = dist(s.y, s.x, y, x);
            if(k == 0) {
                if(greedRoute(st, y, x, s.y, s.x)) {
                    score -= min(100, (int)pow(d, 2));
                } else {
                    score -= min(100, (int)pow(d, 2)) + 1000;
                }
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
            if(f.dog != -1) { ++around; }
        }
    }
    score -= around * 10;
    return max(score, -INF);
}

void safeMoveI(vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const uint range, bool none = false) {
    auto& ch = st.ninja[id];
    if(range == 0) { return; }
    for(uint i = (none ? 4 : 0); i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            if(range > 1) {
                vector<ScoredMove> mv2;
                safeMoveI(mv2, st, id, yy, xx, range - 1, i == 4);
                for(auto&& m : mv2) {
                    m.mv[id].insert(m.mv[id].begin(), static_cast<DIR>(i));
                    mv.emplace_back(std::move(m.mv), m.score, std::move(m.st), m.ch);
                }
            } else {
                array<Move, 2> emv;
                array<Point, 2> ech;
                emv[id] = Move{static_cast<DIR>(i)};
                ech[id] = Point{yy, xx};
                mv.emplace_back(emv, array<int, 2>{0, 0}, st, ech);
            }
        });
    }
}

void safeMove(vector<ScoredMove>& mv, Status& st, const uid_t id, const int y, const int x, const int y2, const int x2, const uint range, const uint limit, const uint depth, bool val, bool none) {
    auto& ch = st.ninja[id];
    if(range == 0) { return; }
    for(uint i = (none ? 4 : 0); i < 5; i++) {
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            auto score = 0;
            if(soul) { score += 10000 * depth; }
            if(range > 1) {
                vector<ScoredMove> mv2;
                safeMove(mv2, st, id, yy, xx, y2, x2,range - 1, limit, depth, val, i == 4);
                for(auto&& m : mv2) {
                    m.mv[id].insert(m.mv[id].begin(), static_cast<DIR>(i));
                    m.score[id] += score;
                    mv.emplace_back(std::move(m.mv), std::move(m.score), std::move(m.st), std::move(m.ch));
                }
            } else if(id == 0) {
                vector<ScoredMove> mv2;
                safeMove(mv2, st, id + 1, y2, x2, yy, xx, limit, limit, depth, val, false);
                for(auto&& m : mv2) {
                    m.mv[id].insert(m.mv[id].begin(), static_cast<DIR>(i));
                    m.score[id] += score;
                    m.ch[id] = Point{yy, xx};
                    mv.emplace_back(std::move(m.mv), std::move(m.score), std::move(m.st), std::move(m.ch));
                }
            } else {
                auto lim = (limit + 1) * depth + 1;
                //auto lim = (limit + 1) + depth;
                dogSim(st, y2, x2, yy, xx, lim, lim, true, [&]() {
                    array<int, 2> esc = {0, 0};
                    esc[0] += (val ? evaluate(st, 0, y2, x2) : 0);
                    esc[1] += (val ? score + evaluate(st, 1, yy, xx) : 0);
                    array<Move, 2> emv;
                    array<Point, 2> ech;
                    emv[id] = Move{static_cast<DIR>(i)};
                    ech[id] = Point{yy, xx};
                    mv.emplace_back(emv, esc, st, ech);
                });
            }
        });
    }
}

inline void safeMove(vector<ScoredMove>& mv, Status& st, const uint range, const uint limit, const uint depth, bool val) {
    auto& ch1 = st.ninja[0];
    auto& ch2 = st.ninja[1];
    safeMove(mv, st, 0, ch1.y, ch1.x, ch2.y, ch2.x, range, limit, depth, val, false);
}

void fakeMove(vector<ScoredMove>& mv, Status& st, Status& ost, const uid_t id, const int y, const int x, const int y2, const int x2, const uint range, const uint limit, const uint depth, bool val, bool none) {
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
                fakeMove(mv2, st, ost, id, yy, xx, y2, x2, range - 1, limit, depth, val, i == 4);
                for(auto&& m : mv2) {
                    m.mv[id].insert(m.mv[id].begin(), static_cast<DIR>(i));
                    m.score[id] += score;
                    mv.emplace_back(std::move(m.mv), std::move(m.score), std::move(m.st), std::move(m.ch));
                }
            } else if(id == 0) {
                vector<ScoredMove> mv2;
                fakeMove(mv2, st, ost, id + 1, y2, x2, yy, xx, limit, limit, depth, val, false);
                for(auto&& m : mv2) {
                    m.mv[id].insert(m.mv[id].begin(), static_cast<DIR>(i));
                    m.score[id] += score;
                    m.ch[id] = Point{yy, xx};
                    mv.emplace_back(std::move(m.mv), std::move(m.score), std::move(m.st), std::move(m.ch));
                }
            } else {
                array<int, 2> esc ={0, 0};
                esc[0] += (val ? evaluate(st, 0, y2, x2) : 0);
                esc[1] += (val ? score + evaluate(st, 1, yy, xx) : 0);
                array<Move, 2> emv;
                array<Point, 2> ech;
                emv[id] = Move{static_cast<DIR>(i)};
                ech[id] = Point{yy, xx};
                mv.emplace_back(emv, esc, st, ech);
            }
        });
    }
}

bool checkMove(Status& st, const uid_t id, const int y, const int x, const uint range) {
    if(range == 0) { return true; }
    for(uint i = 0; i < 5; i++) {
        bool flag = false;
        tryMove(st, y, x, i, [&](const int yy, const int xx, bool stone, bool soul) {
            if(range == 1 && st.field[yy][xx].threat) { return; }
            if(range > 1) {
                flag = checkMove(st, id, yy, xx, range - 1);
            } else {
                flag = true;
            }
        });
        if(flag) { return true; }
    }
    return false;
}

inline bool checkClosed(Status& st, const uid_t id, int range) {
    auto& ch = st.ninja[id];
    return !checkMove(st, id, ch.y, ch.x, range);
}

void safeRoute(Input& in, Output& ou, vector<ScoredMove>& mv, Status& st, const uint beam, uint depth) {
    vector<ScoredMove> mv1;
    auto& ch1 = st.ninja[0];
    auto& ch2 = st.ninja[1];
    safeMove(mv1, st, 2, 2, depth, true);
    if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)) {
        vector<Point> neckList;
        int neckc = 0;
        for(auto&& mm : mv1) {
            for(uid_t i = 0; i < 2; i++) {
                Point neck;
                if(cancelClosed(st, st.ninja[i].y, st.ninja[i].x, mm.mv[i], neck) == -1) { continue; }
                ++neckc;
                if(find_if(neckList.begin(), neckList.end(), [&](const Point& p) {
                    return p.y == neck.y && p.x == neck.x;
                }) == neckList.end()) {
                    neckList.push_back(neck);
                }
                if(st.nin >= in.getCost(SKILL::MYTHUNDER)) { mm.sk = SKILL::MYTHUNDER; mm.val1 = neck.y; mm.val2 = neck.x; }
            }
        }
        if(neckList.size() == 1 || st.nin >= in.getCost(SKILL::MYTHUNDER) * 4) {
            for(auto&& mm : mv1) {
                if(mm.sk == SKILL::MYTHUNDER) { mm.score[0] -= 50000; mm.score[1] -= 50000; }
            }
        } else if(neckc > 0) {
            for(auto&& mm : mv1) {
                if(mm.sk == SKILL::MYTHUNDER) { mm.sk = SKILL::MYTHUNDER; mm.score[0] -= INF / 4; mm.score[1] -= INF / 4; }
            }
        }
    }
    /* TODO: */
    for(uid_t i = 0; i < 2; i++) {
        auto& ch = st.ninja[i];
        auto dogs = calcAroundDog(st, ch.y, ch.x);
        if(st.nin >= in.getCost(SKILL::TORNADO) && (in.getCost(SKILL::TORNADO) <= dogs * 2 || dogs > 4)) {
            vector<pair<int, Point>> dog;
            for(uint i = 0; i < 8; i++) {
                const auto& p = dir2p8(i);
                auto& f = st.field[ch.y + p.y][ch.x + p.x];
                if(f.dog != -1) { deleteDog(st, f.y, f.x); dog.emplace_back(f.dog, Point{f.y, f.x}); }
            }
            vector<ScoredMove> mv2;
            safeMove(mv2, st, 2, 2, depth, true);
            for(auto&& mm : mv2) {
                Point neck;
                for(uid_t i = 0; i < 2; i++) {
                    if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)
                        && cancelClosed(st, st.ninja[i].y, st.ninja[i].x, mm.mv[i], neck) != -1) { goto failtor; }
                }
                mm.sk = SKILL::TORNADO; mm.val1 = i;
                mv1.push_back(mm);
                failtor:;
            }
            for(auto&& d : dog) { newDog(st, d.second.y, d.second.x, d.first); }
        }
    } 
    if(st.nin >= in.getCost(SKILL::SONIC)) {
        vector<ScoredMove> mv2;
        safeMove(mv2, st, 3, 3, depth, true);
        for(auto&& mm : mv2) {
            Point neck;
            for(uid_t i = 0; i < 2; i++) {
                mm.score[i] -= in.getCost(SKILL::SONIC) * depth * 10000;
                if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)
                    && cancelClosed(st, st.ninja[i].y, st.ninja[i].x, mm.mv[i], neck) != -1) { goto failson; }
            }
            mm.sk = SKILL::SONIC;
            mv1.push_back(mm);
            failson:;
        }
    }
    if(st.nin >= in.getCost(SKILL::MYTHUNDER)) {
        for(uid_t i = 0; i < 2; i++) {
            auto& ch = st.ninja[i];
            for(auto dy = max(ch.y - 2, 1); dy <= min(ch.y + 2, st.h - 2); dy++) {
                auto rx = 2 - abs(ch.y - dy);
                for(auto dx = max(ch.x - rx, 1); dx <= min(ch.x + rx, st.w - 2); dx++) {
                    auto& f = st.field[dy][dx];
                    if(f.type != FIELD::ROCK) { continue; }
                    deleteStone(st, dy, dx);
                    vector<ScoredMove> mv2;
                    safeMove(mv2, st, 2, 2, depth, true);
                    for(auto&& mm : mv2) {
                        Point neck;
                        for(uid_t i = 0; i < 2; i++) {
                            mm.score[i] -= in.getCost(SKILL::MYTHUNDER) * 10000;
                            if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)
                                && cancelClosed(st, st.ninja[i].y, st.ninja[i].x, mm.mv[i], neck) != -1) { goto failthu; }
                        }
                        mm.sk = SKILL::MYTHUNDER; mm.val1 = dy; mm.val2 = dx;
                        mv1.push_back(mm);
                        failthu:;
                    }
                    newStone(st, dy, dx);
                }
            }
        }
    }
    if(st.nin >= in.getCost(SKILL::MYGHOST)) {
        vector<Point> cond ={{st.ninja[0].y, st.ninja[0].x},{st.ninja[1].y, st.ninja[1].x},{1, 1},{st.h -2 , 1},{st.h - 2, st.w - 2},{1, st.w - 2}};
        bool suc = false;
        for(auto&& c : cond) {
            if(st.field[c.y][c.x].type != FIELD::FLOOR) { continue; }
            dogSim(st, c.y, c.x, c.y, c.x, INF, INF, false, [&]() {
                vector<ScoredMove> mv2;
                safeMove(mv2, st, 2, 2, depth, true);
                for(auto&& mm : mv2) {
                    Point neck;
                    for(uid_t i = 0; i < 2; i++) {
                        mm.score[i] -= in.getCost(SKILL::MYGHOST) * depth * 10000;
                        if(in.st[1].nin >= in.getCost(SKILL::OPMATEOR)
                            && cancelClosed(st, st.ninja[i].y, st.ninja[i].x, mm.mv[i], neck) != -1) { goto failgoh; }
                    }
                    mm.sk = SKILL::MYGHOST; mm.val1 = c.y; mm.val2 = c.x;
                    mv1.push_back(mm);
                    failgoh:;
                }
            });
        }
    }
    if(mv1.empty()) { return; }
    sort(mv1.begin(), mv1.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        auto s1 = min(p1.score[0], p1.score[1]) == -INF ? -INF : p1.score[0] + p1.score[1];
        auto s2 = min(p2.score[0], p2.score[1]) == -INF ? -INF : p2.score[0] + p2.score[1];
        if(s1 == s2) {
            if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
            if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
            return min(p1.mv[0].size(), p1.mv[1].size()) >  min(p2.mv[0].size(), p2.mv[1].size());
        }
        return s1 > s2;
    });
    if(mv1.size() > beam) { mv1.resize(beam); }
    if(depth <= 1) { return; }
    
    while(true) {
        --depth;
        vector<ScoredMove> next;
        for(auto&& m : mv1) {
            vector<ScoredMove> mv2;
            safeMove(mv2, m.st, 0, m.ch[0].y, m.ch[0].x, m.ch[1].y, m.ch[1].x, 2, 2, depth, true, false);
            if(mv2.empty()) {
                mv.emplace_back(std::move(m.mv), array<int, 2>{-INF, -INF}, m.st, m.ch, m.sk, m.val1, m.val2, m.cancel);
                continue;
            }
            for(auto&& mm : mv2) {
                Point neck;
                for(uid_t i = 0; i < 2; i++) {
                    if(cancelClosed(st, m.ch[i].y, m.ch[i].x, mm.mv[i], neck) != -1) { mm.score[i] -= INF / 4; }
                }
                array<Move, 2> m1 = m.mv;
                array<int, 2> s1 = {0, 0};
                for(uid_t i = 0; i < 2; i++) {
                    for(auto&& m2 : mm.mv[i]) { m1[i].push_back(m2); }
                    s1[i] = max(mm.score[i] + m.score[i] , -INF);
                }
                next.emplace_back(std::move(m1), s1, mm.st, mm.ch, m.sk, m.val1, m.val2, m.cancel);
            }
        }
        if(next.empty()) { mv1.clear(); break; }
        sort(next.begin(), next.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
            auto s1 = min(p1.score[0], p1.score[1]) == -INF ? -INF : p1.score[0] + p1.score[1];
            auto s2 = min(p2.score[0], p2.score[1]) == -INF ? -INF : p2.score[0] + p2.score[1];
            if(s1 == s2) {
                if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
                if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
                return min(p1.mv[0].size(), p1.mv[1].size()) >  min(p2.mv[0].size(), p2.mv[1].size());
            }
            return s1 > s2;
        });
        if(next.size() > beam) { next.resize(beam); }
        mv1 = next;
        if(depth <= 1) { break; }
    }
    for(auto&& mm : mv1) { mv.push_back(mm); }
}

/* main */
void brain(Input& in, Output& ou) {
    Move mv;
    auto& me = in.st[0];
    auto& ch1 = me.ninja[0];
    auto& ch2 = me.ninja[1];
    
    sort(me.souls.begin(), me.souls.end(), [&](const Soul& s1, const Soul& s2) {
        return dist(s1.y, s1.x, ch1.y, ch1.x) < dist(s2.y, s2.x, ch1.y, ch1.x);
    });
    auto it = me.souls.begin();
    while(it != me.souls.end() && !it->reserve) {    
        if(dist(ch2.y, ch2.x, it->y, it->x) > 2 && dist(ch1.y, ch1.x, it->y, it->x) > 2) {
            if(me.field[it->y][it->x].threat) { ++it; continue; }
        }
        if(dist(ch2.y, ch2.x, it->y, it->x) < dist(ch1.y, ch1.x, it->y, it->x)) { 
            ch2.souls.push_back(distance(me.souls.begin(), it));
        } else {
            ch1.souls.push_back(distance(me.souls.begin(), it));
        }
        ++it;
    }
    sort(ch2.souls.begin(), ch2.souls.end(), [&](const uint i1, const uint i2) {
        auto& s1 = me.souls[i1];
        auto& s2 = me.souls[i2];
        return dist(s1.y, s1.x, ch2.y, ch2.x) < dist(s2.y, s2.x, ch2.y, ch2.x);
    });

    int around = 0;
    for(auto dy = max(ch1.y - 4, 1); dy <= min(ch1.y + 4, me.h - 2); dy++) {
        auto rx = 4 - abs(ch1.y - dy);
        for(auto dx = max(ch1.x - rx, 1); dx <= min(ch1.x + rx, me.w - 2); dx++) {
            if(me.field[dy][dx].dog != -1) { ++around; };
        }
    }
    for(auto dy = max(ch2.y - 4, 1); dy <= min(ch2.y + 4, me.h - 2); dy++) {
        auto rx = 4 - abs(ch2.y - dy);
        for(auto dx = max(ch2.x - rx, 1); dx <= min(ch2.x + rx, me.w - 2); dx++) {
            if(me.field[dy][dx].dog != -1) { ++around; };
        }
    }

    vector<ScoredMove> mv2;
    safeRoute(in, ou, mv2, me, 32, around == 0 ? 3 : 7);
    if(mv2.empty()) { return; }
    sort(mv2.begin(), mv2.end(), [&](const ScoredMove& p1, const ScoredMove& p2) {
        auto s1 = min(p1.score[0], p1.score[1]) == -INF ? -INF : p1.score[0] + p1.score[1];
        auto s2 = min(p2.score[0], p2.score[1]) == -INF ? -INF : p2.score[0] + p2.score[1];
        if(s1 == s2) {
            if(p1.sk == SKILL::NONE && p2.sk != SKILL::NONE) { return true; }
            if(p1.sk != SKILL::NONE && p2.sk == SKILL::NONE) { return false; }
            return min(p1.mv[0].size(), p1.mv[1].size()) >  min(p2.mv[0].size(), p2.mv[1].size());
        }
        return s1 > s2;
    });
    if(mv2[0].sk != SKILL::NONE) {
        ou.sk = mv2[0].sk;
        ou.val1 = mv2[0].val1;
        ou.val2 = mv2[0].val2;
    }
    mv2[0].mv[0].resize(mv2[0].sk == SKILL::SONIC ? 3 : 2);
    mv2[0].mv[1].resize(mv2[0].sk == SKILL::SONIC ? 3 : 2);
    ou.mv[0] = std::move(mv2[0].mv[0]);
    ou.mv[1] = std::move(mv2[0].mv[1]);
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
            safeMoveI(mv, op, nj.id, nj.y, nj.x, 2);
            vector<Point> neckList;
            bool exec = true;
            for(auto&& mm : mv) {
                Point neck;
                if(cancelClosed(op, nj.id, mm.mv[nj.id], neck) == -1) { exec = false; break; }
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
}

Output resolve(Input in) {
    Output ou;
    brain(in, ou);
    if(ou.sk == SKILL::NONE) { findCritical(ou, in); }
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
