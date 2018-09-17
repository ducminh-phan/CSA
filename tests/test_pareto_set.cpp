#include "catch.hpp"
#include "pareto_set.hpp"

using Pair = Element<int, int>;
using Triple = Element<int, int, int>;
using ParetoPair = ParetoSet<int, int>;
using ParetoTriple = ParetoSet<int, int, int>;

TEST_CASE("The the comparison of two points") {
    REQUIRE(Pair(1, 2) == Pair(1, 2));
    REQUIRE(Triple(9, 8, 7) == Triple(9, 8, 7));

    REQUIRE(Pair(1, 2) != Pair(2, 1));
    REQUIRE(Triple(7, 8, 9) != Triple(9, 8, 7));
}

TEST_CASE("Test the domination of two points") {
    SECTION("A point does not dominate itself") {
        REQUIRE(!Pair(1, 2).dominates(Pair(1, 2)));
        REQUIRE(!Triple(3, 4, 5).dominates(Triple(3, 4, 5)));
    }

    SECTION("Check domination in a pair by one or both criteria") {
        REQUIRE(Pair(1, 2).dominates(Pair(2, 2)));
        REQUIRE(Pair(1, 2).dominates(Pair(1, 3)));
        REQUIRE(Pair(1, 2).dominates(Pair(2, 3)));

        REQUIRE(!Pair(1, 2).dominates(Pair(0, 2)));
        REQUIRE(!Pair(1, 2).dominates(Pair(1, 1)));
        REQUIRE(!Pair(1, 2).dominates(Pair(0, 1)));
    }

    SECTION("Check domination in a triple by one or all criteria") {
        REQUIRE(Triple(1, 2, 3).dominates(Triple(2, 2, 3)));
        REQUIRE(Triple(1, 2, 3).dominates(Triple(1, 3, 3)));
        REQUIRE(Triple(1, 2, 3).dominates(Triple(1, 2, 4)));
        REQUIRE(Triple(1, 2, 3).dominates(Triple(2, 3, 4)));

        REQUIRE(!Triple(1, 2, 3).dominates(Triple(0, 2, 3)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(1, 1, 3)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(1, 2, 2)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(0, 1, 2)));
    }

    SECTION("A point does not dominate the other if better at one criterion and worse at another") {
        REQUIRE(!Pair(1, 2).dominates(Pair(2, 1)));
        REQUIRE(!Pair(2, 1).dominates(Pair(1, 2)));

        REQUIRE(!Triple(1, 2, 3).dominates(Triple(2, 1, 3)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(1, 3, 2)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(3, 2, 1)));

        REQUIRE(!Triple(1, 2, 3).dominates(Triple(2, 1, 2)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(0, 3, 2)));
        REQUIRE(!Triple(1, 2, 3).dominates(Triple(0, 1, 4)));
    }
}

TEST_CASE("A Pareto set keeps only non-dominating elements") {
    ParetoPair pp;
    ParetoPair test_pp;

    pp.emplace(1, 2);
    REQUIRE(pp.size() == 1);

    pp.emplace(2, 1);
    REQUIRE(pp.size() == 2);

    ParetoPair old_pp = pp;

    SECTION("Dominated points are not inserted") {
        pp.emplace(2, 2);
        REQUIRE(pp == old_pp);

        pp.emplace(2, 3);
        REQUIRE(pp == old_pp);

        pp.emplace(3, 2);
        REQUIRE(pp == old_pp);
    }

    SECTION("If a new point is inserted, dominated old points are removed") {
        SECTION("") {
            pp.emplace(1, 1);

            test_pp.emplace(1, 1);

            REQUIRE(pp == test_pp);
        }

        SECTION("") {
            pp = ParetoPair();

            pp.emplace(0, 2);
            pp.emplace(2, 0);

            REQUIRE(pp.size() == 2);

            SECTION("") {
                pp.emplace(1, -1);

                test_pp.emplace(0, 2);
                test_pp.emplace(1, -1);

                REQUIRE(pp == test_pp);
            }

            SECTION("") {
                pp.emplace(-1, 1);

                test_pp.emplace(2, 0);
                test_pp.emplace(-1, 1);

                REQUIRE(pp == test_pp);
            }
        }
    }

    SECTION("A non-dominating new point does not remove old points") {
        pp.emplace(0, 3);
        REQUIRE(pp.size() == 3);

        pp.emplace(3, 0);
        REQUIRE(pp.size() == 4);

        pp.emplace(-1, 100);
        REQUIRE(pp.size() == 5);

        pp.emplace(100, -1);
        REQUIRE(pp.size() == 6);
    }
}

TEST_CASE("Check if two Pareto sets are merged correctly") {
    ParetoPair p1, p2, p_test;
    ParetoTriple t1, t2, t_test;

    SECTION("") {
        p1.emplace(0, 2);
        p1.emplace(2, 1);

        p2.emplace(1, 1);
        p2.emplace(3, 0);

        p_test.emplace(0, 2);
        p_test.emplace(1, 1);
        p_test.emplace(3, 0);

        p1.merge(p2);
        REQUIRE(p1 == p_test);
    }

    SECTION("") {
        t1.emplace(3, 0, 0);
        t1.emplace(0, 3, 0);
        t1.emplace(0, 0, 3);
        t1.emplace(2, 2, 2);

        t2.emplace(1, 2, 2);
        t2.emplace(2, 1, 2);
        t2.emplace(2, 2, 1);

        SECTION("") {
            t_test.emplace(3, 0, 0);
            t_test.emplace(0, 3, 0);
            t_test.emplace(0, 0, 3);

            t_test.emplace(1, 2, 2);
            t_test.emplace(2, 1, 2);
            t_test.emplace(2, 2, 1);

            t1.merge(t2);
            REQUIRE(t1 == t_test);
        }

        SECTION("") {
            t1.emplace(2, 1, 1);
            t2.emplace(1, 1, 2);

            t_test.emplace(3, 0, 0);
            t_test.emplace(0, 3, 0);
            t_test.emplace(0, 0, 3);
            t_test.emplace(1, 2, 2);
            t_test.emplace(2, 1, 1);
            t_test.emplace(1, 1, 2);

            t1.merge(t2);
            REQUIRE(t1 == t_test);
        }
    }
}
