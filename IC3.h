/*********************************************************************
Copyright (c) 2013, Aaron Bradley

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

#ifndef IC3_h_INCLUDED
#define IC3_h_INCLUDED

#include "Model.h"

namespace IC3 {

    // A CubeSet is a set of ordered (by integer value) vectors of
    // Minisat::Lits.
    static bool _LitVecComp(const LitVec &v1, const LitVec &v2) {
        if (v1.size() < v2.size()) return true;
        if (v1.size() > v2.size()) return false;
        for (size_t i = 0; i < v1.size(); ++i) {
            if (v1[i] < v2[i]) return true;
            if (v2[i] < v1[i]) return false;
        }
        return false;
    }

    static bool _LitVecEq(const LitVec &v1, const LitVec &v2) {
        if (v1.size() != v2.size()) return false;
        for (size_t i = 0; i < v1.size(); ++i)
            if (v1[i] != v2[i]) return false;
        return true;
    }

    class LitVecComp {
    public:
        bool operator()(const LitVec &v1, const LitVec &v2) {
            return _LitVecComp(v1, v2);
        }
    };

    // The State structures are for tracking trees of (lifted) CTIs.
    // Because States are created frequently, I want to avoid dynamic
    // memory management; instead their (de)allocation is handled via
    // a vector-based pool.
    struct State {
        size_t successor;  // successor State
        LitVec latches;
        LitVec inputs;
        size_t index;      // for pool
        bool used;         // for pool
    };

    // A proof obligation.
    struct Obligation {
        Obligation(size_t st, size_t l, size_t d) :
                state(st), level(l), depth(d) {}

        size_t state;  // Generalize this state...
        size_t level;  // ... relative to this level.
        size_t depth;  // Length of CTI suffix to error.
    };

    typedef set<LitVec, LitVecComp> CubeSet;

    // For IC3's overall frame structure.
    struct Frame {
        size_t k;             // steps from initial state
        CubeSet borderCubes;  // additional cubes in this and previous frames
        Minisat::Solver *consecution;
    };

    class ObligationComp {
    public:
        bool operator()(const Obligation &o1, const Obligation &o2) {
            if (o1.level < o2.level) return true;  // prefer lower levels (required)
            if (o1.level > o2.level) return false;
            if (o1.depth < o2.depth) return true;  // prefer shallower (heuristic)
            if (o1.depth > o2.depth) return false;
            if (o1.state < o2.state) return true;  // canonical final decider
            return false;
        }
    };

    typedef set<Obligation, ObligationComp> PriorityQueue;



    class IC3 {
    public:
        IC3(Model &_model);

        ~IC3();

        bool check();

        void printWitness();

    private:
        int verbose;
        int random;

        string stringOfLitVec(const LitVec &vec);

        Model &model;
        size_t k;
        vector<State> states;
        size_t nextState;

        State &state(size_t sti);

        size_t newState();

        void delState(size_t sti);

        void resetStates();

        vector<Frame> frames;
        Minisat::Solver *lifts;
        Minisat::Lit notInvConstraints;

        void extend();

        // Structure and methods for imposing priorities on literals
        // through ordering the dropping of literals in mic (drop leftmost
        // literal first) and assumptions to Minisat.  The implemented
        // ordering prefers to keep literals that appear frequently in
        // addCube() calls.
        struct HeuristicLitOrder {
            HeuristicLitOrder() : _mini(1 << 20) {}

            vector<float> counts;
            size_t _mini;

            void count(const LitVec &cube) {
                assert (!cube.empty());
                // assumes cube is ordered
                size_t sz = (size_t) Minisat::toInt(Minisat::var(cube.back()));
                if (sz >= counts.size()) counts.resize(sz + 1);
                _mini = (size_t) Minisat::toInt(Minisat::var(cube[0]));
                for (LitVec::const_iterator i = cube.begin(); i != cube.end(); ++i)
                    counts[(size_t) Minisat::toInt(Minisat::var(*i))] += 1;
            }

            void decay() {
                for (size_t i = _mini; i < counts.size(); ++i)
                    counts[i] *= 0.99;
            }
        } litOrder;

        struct SlimLitOrder {
            HeuristicLitOrder *heuristicLitOrder;

            SlimLitOrder() {}

            bool operator()(const Minisat::Lit &l1, const Minisat::Lit &l2) const {
                // l1, l2 must be unprimed
                size_t i2 = (size_t) Minisat::toInt(Minisat::var(l2));
                if (i2 >= heuristicLitOrder->counts.size()) return false;
                size_t i1 = (size_t) Minisat::toInt(Minisat::var(l1));
                if (i1 >= heuristicLitOrder->counts.size()) return true;
                return (heuristicLitOrder->counts[i1] < heuristicLitOrder->counts[i2]);
            }
        } slimLitOrder;

        float numLits, numUpdates;

        void updateLitOrder(const LitVec &cube, size_t level);

        void orderCube(LitVec &cube);

        typedef Minisat::vec<Minisat::Lit> MSLitVec;

        void orderAssumps(MSLitVec &cube, bool rev, int start = 0);

        size_t stateOf(Frame &fr, size_t succ = 0);

        bool initiation(const LitVec &latches);

        bool consecution(size_t fi, const LitVec &latches, size_t succ = 0,
                         LitVec *core = NULL, size_t *pred = NULL,
                         bool orderedCore = false);

        size_t maxDepth, maxCTGs, maxJoins, micAttempts;

        bool ctgDown(size_t level, LitVec &cube, size_t keepTo, size_t recDepth);

        void mic(size_t level, LitVec &cube, size_t recDepth);

        void mic(size_t level, LitVec &cube);

        size_t earliest;  // track earliest modified level in a major iteration

        void addCube(size_t level, LitVec &cube, bool toAll = true,
                     bool silent = false);

        size_t generalize(size_t level, LitVec cube);

        size_t cexState;  // beginning of counterexample trace

        bool handleObligations(PriorityQueue obls);

        bool trivial;  // indicates whether strengthening was required
        // during major iteration

        bool strengthen();

        bool propagate();

        int nQuery, nCTI, nCTG, nmic;
        clock_t startTime, satTime;
        int nCoreReduced, nAbortJoin, nAbortMic;

        clock_t time();

        clock_t timer;

        void startTimer();

        void endTimer(clock_t &t);

        void printStats();

        friend bool check(Model &, IC3&, int, bool, bool);

        vector<LitVec> lifted_states;
        PriorityQueue all_obligations;

        void reuse_previous_obligations();
    };

    bool check(Model &model,
               IC3& ic3,
               int verbose = 0,       // 0: silent, 1: stats, 2: informative
               bool basic = false,    // simple inductive generalization
               bool random = false);  // random runs for statistical profiling

    static void print_lit(Minisat::Lit l, VarVec vars) {
        if (Minisat::sign(l)) {
            cout << "!";
        }
        cout << vars[var(l)].name() << ", ";
    }

    static void print_cube(LitVec s, VarVec vars) {
        cout << "{";
        for (Minisat::Lit l: s) {
            print_lit(l, vars);
        }
        cout << "}";
        cout << endl;
    }

    static void print_frame(Frame f, VarVec vars) {
        cout << "\t";
        cout << "Frame index: " << f.k << endl;
        CubeSet cs = f.borderCubes;
        for (LitVec c: cs) {
            cout << "\t";
            print_cube(c, vars);
        }
        cout << endl << endl;
    }

    static void print_frames(vector<Frame> f, VarVec vars) {
        for (int i = 0; i < f.size(); i++) {
            Frame x = f[i];
            print_frame(x, vars);
        }
        cout << "<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    }
}

#endif
