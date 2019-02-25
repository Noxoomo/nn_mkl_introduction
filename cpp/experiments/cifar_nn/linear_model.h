#pragma once

#include "model.h"
#include "linear_function.h"

#include <torch/torch.h>
#include <torch/csrc/autograd/function.h>
#include <cstdint>

class LinearModel : public Model {
public:
    LinearModel(uint32_t in, uint32_t out) : Model() {
        weights_ = register_parameter("weights", torch::randn({out, in}, torch::kFloat32));
    }

    torch::Tensor forward(torch::Tensor x) override {
        LinearFunction f;
        return f.apply({x, weights_})[0];
//        return torch::mm(weights_, x.t()).t();
    }

private:
    torch::Tensor weights_;
};
