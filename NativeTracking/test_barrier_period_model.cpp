#include "../ProjectPB/BarrierPeriodModel.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace barrier_period_model;
int main() {
    const double gap = 5.0 / 1.49 + 1.0 / 1.5;
    double h = 0, h800, h1000, h1200;
    assert(fromCalibration(1000, 1000, 330, gap, h));
    assert(std::abs(h - 330) < 1e-8);
    assert(fromDimensions(800, gap, .7434006084025878, .273, h800));
    assert(fromDimensions(1000, gap, .7434006084025878, .273, h1000));
    assert(fromDimensions(1200, gap, .7434006084025878, .273, h1200));
    assert(h800 > h1000 && h1000 > h1200);
    assert(std::abs(h1000 - 59.60435) < .001);
    for (int distance = 800; distance <= 1200; ++distance) {
        double absolute, calibrated;
        assert(fromDimensions(distance, gap, .7434006084025878, .273, absolute));
        assert(fromCalibration(distance, 1000, h1000, gap, calibrated));
        assert(std::abs(absolute - calibrated) < 1e-8);
    }
    double linear;
    assert(linearApproximation(1000, 1000, 330, gap, linear));
    assert(std::abs(linear - 330) < 1e-10);
    assert(fromCalibration(1200, 1000, 330, 0, h));
    assert(std::abs(h - 330) < 1e-8);
    h = 123;
    assert(!fromDimensions(1000, gap, .7434, .4, h));
    assert(h == 123);
    assert(!fromCalibration(0, 1000, 330, gap, h));
    assert(!fromCalibration(1000, 1000, 0, gap, h));
    assert(!fromCalibration(std::numeric_limits<double>::quiet_NaN(), 1000, 330, gap, h));
    std::cout << "PASS: anchor, direction, absolute/calibrated equivalence, linear anchor, domain checks\n";
}
