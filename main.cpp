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

/* type */
typedef unsigned int uint;
typedef uint uid_t;
typedef struct { int y, x; } Point;

/* constant */
const std::string AINAME = "matcha";
const int INF = 1 << 30;
const uint SIZE = 100;

/* util */
const auto randgen = []() {
    static std::random_device rnd;
    static std::mt19937 mt(rnd());
    return [](const uint range) {
        static std::uniform_int_distribution<> ud(0, range - 1);
        return []() { return ud(mt); };
    };
}();
inline auto rand(const uint range) { return randgen(range)(); }

/* enum */
template <class T>
inline uint toUint(T t) { return static_cast<uint>(t); }

enum class UNIT : uint {
    WORKER = 0,
    KNIGHT,
    FIGHTER,
    ASSASSIN,
    CASTLE,
    VILLAGE,
    BASE,
    SIZE
};

inline UNIT toUnit(uint u) { return static_cast<UNIT>(u); }

enum class DIR : uint {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT
};

inline DIR toDir(uint d) { return static_cast<DIR>(d); }
inline auto dir2p(uint d) {
    static const std::vector<Point> table = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    return table[d];
}
inline auto dir2p(DIR d) { return dir2p(toUint(d)); }

enum class COMMAND : char {
    UP = 'U',
    DOWN = 'D',
    LEFT = 'L',
    RIGHT = 'R',
    WORKER = '0',
    KNIGHT = '1',
    FIGHTER = '2',
    ASSASSIN = '3',
    VILLAGE = '5',
    BASE = '6'
};

inline COMMAND toCmd(UNIT type) { return static_cast<COMMAND>('0' + toUint(type)); }
inline COMMAND toCmd(DIR dir) {
    static const std::vector<char> table = {'U', 'D', 'L', 'R'};
    return static_cast<COMMAND>(table[toUint(dir)]);
}

/* class */
struct Unit {
    uid_t id;
    int y;
    int x;
    int hp;
    UNIT type;
};

struct Input {
    uint time;
    uint stage;
    uint turn;
    uint res;
    std::vector<Unit> vp;
    std::vector<Unit> ve;
    std::vector<Point> vr;
};

struct Command {
    uid_t id;
    COMMAND type;
};

/* IO */
template <class T, class F>
inline auto readVec(std::vector<T>& v, uint k, F&& f) {
    v.resize(k);
    std::generate(v.begin(), v.end(), [&f]() { T t; f(t); return std::move(t); });
};

auto input(Input& in) {
    using namespace std;
    uint k; string s;
    cin >> in.time >> in.stage >> in.turn >> in.res;
    readVec(in.vp, (cin >> k, k), [](Unit& u) { cin >> u.id >> u.y >> u.x >> u.hp >> (uint&)u.type; });
    readVec(in.ve, (cin >> k, k), [](Unit& u) { cin >> u.id >> u.y >> u.x >> u.hp >> (uint&)u.type; });
    readVec(in.vr, (cin >> k, k), [](Point& r){ cin >> r.y >> r.x; });
    return cin >> s, s == "END";
}

inline void output(const std::vector<Command>& cmd) {
    using namespace std;
    cout << cmd.size() << endl;
    for(auto&& c : cmd) { cout << c.id << " " << static_cast<char>(c.type) << endl; }
    cout.flush();
}

/* main */
auto resolve(const Input& in) {
    std::vector<Command> cmd;

    return std::move(cmd);
}

auto main()-> int {

#if _DEBUG && _MSC_VER
    // check memory leak (only VS)
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    using namespace std;
    ios::sync_with_stdio(false);

    Input in;
    cout << AINAME << endl; cout.flush();
    while(input(in)) { output(resolve(in)); }
}
