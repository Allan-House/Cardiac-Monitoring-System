#pragma once

class BiquadFilter {
public:
    BiquadFilter() = default;

    void Configure(double b0, double b1, double b2,
                   double a0, double a1, double a2) {
        // Normalize so that a0 = 1.0
        b0_ = b0 / a0;
        b1_ = b1 / a0;
        b2_ = b2 / a0;
        a1_ = a1 / a0;
        a2_ = a2 / a0;
        Reset();
    }

    inline double Process(double x) {
        // Direct Form II Transposed
        double y = b0_ * x + z1_;
        z1_ = b1_ * x - a1_ * y + z2_;
        z2_ = b2_ * x - a2_ * y;
        return y;
    }

    void Reset() {
        z1_ = z2_ = 0.0;
    }

private:
    double b0_{0}, b1_{0}, b2_{0};
    double a1_{0}, a2_{0};
    double z1_{0}, z2_{0};
};
