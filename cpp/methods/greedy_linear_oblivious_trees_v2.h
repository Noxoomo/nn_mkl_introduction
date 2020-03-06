#pragma once

#include <unordered_set>
#include <vector>
#include <memory>

#include "optimizer.h"

#include <models/model.h>
#include <models/bin_optimized_model.h>

#include <data/grid.h>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/LU>


class GreedyLinearObliviousTreeLearnerV2;

[[nodiscard]] inline Eigen::MatrixXd DiagMx(int dim, double v) {
    Eigen::MatrixXd mx(dim, dim);
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            if (j == i) {
                mx(i, j) = v;
            } else {
                mx(i, j) = 0;
            }
        }
    }
    return mx;
}

class BinStat {
public:
    explicit BinStat(int size, int filledSize)
            : size_(size)
            , filledSize_(filledSize) {
        XTX_.resize(size * (size + 1) / 2, 0.0);
        XTy_ = std::vector<float>(size, 0.0);
        w_ = 0.;
        trace_ = 0.0;
        maxUpdatedPos_ = filledSize_;
    }

    void reset() {
        w_ = 0;
        trace_ = 0;
        int pos = 0;
        for (int i = 0; i <= filledSize_; ++i) {
            for (int j = 0; j < i + 1; ++j) {
                XTX_[pos + j] = 0;
            }
            pos += i + 1;
            XTy_[i] = 0;
        }
        filledSize_ = 0;
    }

    void setFilledSize(int filledSize) {
        filledSize_ = filledSize;
    }

    int filledSize() {
        return filledSize_;
    }

    void addNewCorrelation(const std::vector<float>& xtx, float xty, int shift = 0) {
        assert(xtx.size() >= filledSize_ + shift + 1);

        const int corPos = filledSize_ + shift;

        int pos = corPos * (corPos + 1) / 2;
        for (int i = 0; i <= corPos; ++i) {
            XTX_[pos + i] += xtx[i];
        }
        XTy_[corPos] += xty;
        trace_ += xtx[corPos];
        maxUpdatedPos_ = std::max(maxUpdatedPos_, corPos + 1);
    }

    void addFullCorrelation(const std::vector<float>& x, float y, float w) {
        assert(x.size() >= filledSize_);

        for (int i = 0; i < filledSize_; ++i) {
            XTy_[i] += x[i] * y * w;
        }

        int pos = 0;
        for (int i = 0; i < filledSize_; ++i) {
            for (int j = 0; j < i + 1; ++j) {
                XTX_[pos + j] += x[i] * x[j] * w;
            }
            pos += i + 1;
        }

        w_ += w;
    }

    [[nodiscard]] Eigen::MatrixXd getXTX() const {
        Eigen::MatrixXd res(maxUpdatedPos_, maxUpdatedPos_);

        int basePos = 0;
        for (int i = 0; i < maxUpdatedPos_; ++i) {
            for (int j = 0; j < i + 1; ++j) {
                res(i, j) = XTX_[basePos + j];
                res(j, i) = XTX_[basePos + j];
            }
            basePos += i + 1;
        }

        return res;
    }

    [[nodiscard]] Eigen::MatrixXd getXTy() const {
        Eigen::MatrixXd res(maxUpdatedPos_, 1);

        for (int i = 0; i < maxUpdatedPos_; ++i) {
            res(i, 0) = XTy_[i];
        }

        return res;
    }

    float getWeight() {
        return w_;
    }

    double getTrace() {
        return trace_;
    }

    double fitScore(double l2reg, bool log = false) {
        if (log) std::cout << "fitScoring, " << maxUpdatedPos_ << ", l2=" << l2reg << std::endl;
        if (log) std::cout << "getXTX():\n" << getXTX() << "\nDiagMx:\n" << DiagMx(maxUpdatedPos_, l2reg) << "\n+\n"<< getXTX() + DiagMx(maxUpdatedPos_, l2reg) << std::endl;
        Eigen::MatrixXd XTX = getXTX() + DiagMx(maxUpdatedPos_, l2reg);
        if (log) std::cout << "XTX=\n" << XTX << std::endl;
        Eigen::MatrixXd XTy = getXTy();
        if (log) std::cout << "XTy=\n" << XTy << std::endl;

        Eigen::MatrixXd w = XTX.inverse() * XTy;
        if (log) std::cout << "w=\n" << w << std::endl;

        Eigen::MatrixXd c1(XTy.transpose() * w);
        c1 *= -2;
        assert(c1.rows() == 1 && c1.cols() == 1);
        if (log) std::cout << "c1=\n" << c1 << std::endl;

        Eigen::MatrixXd c2(w.transpose() * XTX * w);
        assert(c2.rows() == 1 && c2.cols() == 1);
        if (log) std::cout << "c2=\n" << c2 << std::endl;

        Eigen::MatrixXd reg = w.transpose() * w * l2reg;
        assert(reg.rows() == 1 && reg.cols() == 1);
        if (log) std::cout << "reg=\n" << reg << std::endl;

        Eigen::MatrixXd res = c1 + c2 + reg;
        if (log) std::cout << "res=\n" << reg << std::endl;

        return res(0, 0);
    }

    BinStat& addSized(const BinStat& s, int size) {
        w_ += s.w_;
        trace_ += s.trace_;

        int pos = 0;
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < i + 1; ++j) {
                XTX_[pos + j] += s.XTX_[pos + j];
            }
            pos += i + 1;
            XTy_[i] += s.XTy_[i];
        }

        return *this;
    }

    BinStat& subtractSized(const BinStat& s, int size) {
        w_ -= s.w_;
        trace_ -= s.trace_;

        int pos = 0;
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < i + 1; ++j) {
                XTX_[pos + j] -= s.XTX_[pos + j];
            }
            pos += i + 1;
            XTy_[i] -= s.XTy_[i];
        }

        return *this;
    }

    BinStat& operator+=(const BinStat& s) {
        return addSized(s, std::min(filledSize_, s.filledSize_));
    }

    BinStat& operator-=(const BinStat& s) {
        return subtractSized(s, std::min(filledSize_, s.filledSize_));
    }

private:
//    friend BinStat operator+(const BinStat& lhs, const BinStat& rhs);
//    friend BinStat operator-(const BinStat& lhs, const BinStat& rhs);

    friend BinStat subtractFull(const BinStat& lhs, const BinStat& rhs);
    friend BinStat addFull(const BinStat& lhs, const BinStat& rhs);

    friend BinStat subtractFilled(const BinStat& lhs, const BinStat& rhs);
    friend BinStat addFilled(const BinStat& lhs, const BinStat& rhs);

public:
    int size_;
    int filledSize_;
    int maxUpdatedPos_;

    std::vector<float> XTX_;
    std::vector<float> XTy_;
    float w_;
    double trace_;
};

inline BinStat addFilled(const BinStat& lhs, const BinStat& rhs) {
    BinStat res(lhs);
    res += rhs;
    return res;
}

inline BinStat subtractFilled(const BinStat& lhs, const BinStat& rhs) {
    BinStat res(lhs);
    res -= rhs;
    return res;
}

inline BinStat subtractFull(const BinStat& lhs, const BinStat& rhs) {
    BinStat res(lhs);
    res.maxUpdatedPos_ = std::max(lhs.maxUpdatedPos_, rhs.maxUpdatedPos_);
    return res.subtractSized(rhs, rhs.maxUpdatedPos_);
}

inline BinStat addFull(const BinStat& lhs, const BinStat& rhs) {
    BinStat res(lhs);
    res.maxUpdatedPos_ = std::max(lhs.maxUpdatedPos_, rhs.maxUpdatedPos_);
    return res.subtractSized(rhs, rhs.maxUpdatedPos_);
}



class HistogramV2 {
public:
    HistogramV2(BinarizedDataSet& bds, GridPtr grid, unsigned int nUsedFeatures, int lastUsedFeatureId);

//    void addFullCorrelation(int bin, Vec x, double y);
    void addNewCorrelation(int bin, const std::vector<float>& xtx, float xty, int shift = 0);
    void prefixSumBins();

    void addBinStat(int bin, const BinStat& stats);

    std::pair<double, double> splitScore(int fId, int condId, double l2reg, double traceReg);

    std::shared_ptr<Eigen::MatrixXd> getW(double l2reg);

    void printEig(double l2reg);
    void printCnt();
    void print();

    HistogramV2& operator+=(const HistogramV2& h);
    HistogramV2& operator-=(const HistogramV2& h);

private:
    static double computeScore(Eigen::MatrixXd XTX, Eigen::MatrixXd XTy, double l2reg, bool log = false);

    static void printEig(Eigen::MatrixXd& M);

    friend HistogramV2 operator-(const HistogramV2& lhs, const HistogramV2& rhs);
    friend HistogramV2 operator+(const HistogramV2& lhs, const HistogramV2& rhs);

private:
    BinarizedDataSet& bds_;
    GridPtr grid_;

    std::vector<BinStat> hist_;

    int lastUsedFeatureId_ = -1;
    unsigned int nUsedFeatures_;

    friend class GreedyLinearObliviousTreeLearnerV2;
};

class LinearObliviousTreeLeafV2;

class GreedyLinearObliviousTreeLearnerV2 final
        : public Optimizer {
public:
    explicit GreedyLinearObliviousTreeLearnerV2(GridPtr grid, int32_t maxDepth = 6, int biasCol = -1,
                                              double l2reg = 0.0, double traceReg = 0.0)
            : grid_(std::move(grid))
            , biasCol_(biasCol)
            , maxDepth_(maxDepth)
            , l2reg_(l2reg)
            , traceReg_(traceReg) {
    }

    GreedyLinearObliviousTreeLearnerV2(const GreedyLinearObliviousTreeLearnerV2& other) = default;

    ModelPtr fit(const DataSet& dataSet, const Target& target) override;

private:
    void cacheDs(const DataSet& ds);
    void resetState();

private:
    GridPtr grid_;
    int32_t maxDepth_ = 6;
    int biasCol_ = -1;
    double l2reg_ = 0.0;
    double traceReg_ = 0.0;

    bool isDsCached_ = false;
    std::vector<Vec> fColumns_;
    std::vector<ConstVecRef<float>> fColumnsRefs_;
    std::vector<std::vector<float>> curX_;

    std::vector<int32_t> leafId_;

    std::set<int> usedFeatures_;
    std::vector<int> usedFeaturesOrdered_;

    // thread      leaf         bin         coordinate
    std::vector<std::vector<std::vector<std::vector<float>>>> h_XTX_;
    std::vector<std::vector<std::vector<float>>> h_XTy_;
    std::vector<std::vector<std::vector<BinStat>>> stats_;

    std::vector<bool> fullUpdate_;
    std::vector<int> samplesLeavesCnt_;

    ConstVecRef<int32_t> binOffsets_;
    int nThreads_;
    int totalBins_;
    int totalCond_;
    int fCount_;
    int nSamples_;
};

class LinearObliviousTreeV2 final
        : public Stub<BinOptimizedModel, LinearObliviousTreeV2>
        , std::enable_shared_from_this<LinearObliviousTreeV2> {
public:

    LinearObliviousTreeV2(const LinearObliviousTreeV2& other, double scale)
            : Stub<BinOptimizedModel, LinearObliviousTreeV2>(other.gridPtr()->origFeaturesCount(), 1) {
        grid_ = other.grid_;
        scale_ = scale;
        leaves_ = other.leaves_;
        splits_ = other.splits_;
    }

    LinearObliviousTreeV2(GridPtr grid, std::vector<std::shared_ptr<LinearObliviousTreeLeafV2>> leaves)
            : Stub<BinOptimizedModel, LinearObliviousTreeV2>(grid->origFeaturesCount(), 1)
            , grid_(std::move(grid))
            , leaves_(std::move(leaves)) {
        scale_ = 1;
    }

    explicit LinearObliviousTreeV2(GridPtr grid)
            : Stub<BinOptimizedModel, LinearObliviousTreeV2>(grid->origFeaturesCount(), 1)
            , grid_(std::move(grid)) {

    }

    Grid grid() const {
        return *grid_.get();
    }

    GridPtr gridPtr() const {
        return grid_;
    }

    // I have now idea what this function should do...
    // For now just adding value(x) to @param to.
    void appendTo(const Vec& x, Vec to) const override;

    void applyToBds(const BinarizedDataSet& ds, Mx to, ApplyType type) const override;

    void applyBinarizedRow(const Buffer<uint8_t>& x, Vec to) const {
        throw std::runtime_error("Unimplemented");
    }

    double value(const Vec& x) override;

    void grad(const Vec& x, Vec to) override;

private:
    friend class GreedyLinearObliviousTreeLearnerV2;

    double value(const ConstVecRef<float>& x) const;

private:
    GridPtr grid_;
    double scale_ = 1;

    std::vector<std::tuple<int32_t, int32_t>> splits_;

    std::vector<std::shared_ptr<LinearObliviousTreeLeafV2>> leaves_;
};
