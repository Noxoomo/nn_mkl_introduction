#include <core/vec.h>
#include <core/vec_factory.h>
#include <core/vec_tools/fill.h>
#include <core/funcs/linear.h>
#include <util/exception.h>
#include <core/funcs/p_dist_func.h>

#include <gtest/gtest.h>

#include <cassert>
#include <cmath>

#include <stdio.h>

#define EPS 1e-5

TEST(FuncTests, Linear) {
    Vec param = VecFactory::create(VecType::Cpu, 3);

    param.set(0, 1);
    param.set(1, -2);
    param.set(2, 3);


    Vec x = VecFactory::create(VecType::Cpu, 2);
    x.set(0, 10);
    x.set(1, 20);

    Linear linear(param);
    double res = linear.value(x);
    EXPECT_EQ(res, 41);

}

TEST(FuncTests, PDistFunc) {
    const double p = 2;

    Vec x = VecFactory::create(VecType::Cpu, 3);
    x.set(0, -1);
    x.set(1, 2);
    x.set(2, 3);

    Vec b = VecFactory::create(VecType::Cpu, 3);
    b.set(0, -1);
    b.set(1, 5);
    b.set(2, 7);

    PDistFunc d(p, b);
    auto res = d.value(x);
    EXPECT_EQ(res, 5);

    Vec c = VecFactory::create(VecType::Cpu, 3);
    auto grad = d.gradient();
    grad->trans(x, c);
    for (auto i = 0; i < 3; i++) {
        EXPECT_NEAR(c(i), p * std::pow(x(i) - b(i), p - 1), EPS);
    }
}
