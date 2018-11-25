/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/film.h>
#include <lm/logger.h>
#include <lm/parallel.h>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Film_Progress final : public Film {
private:
    int w_;
    int h_;
    std::vector<std::atomic<Vec3>> data_;
    std::vector<Vec3> dataTemp_;  // Temporary buffer for external reference
    time_point<high_resolution_clock> lastUpdated_;
    ReportProgressFunc reportProgress_;

private:
    void makeTemp() {
        dataTemp_.clear();
        for (const auto& v : data_) {
            dataTemp_.push_back(v);
        }
    }

    void atomicUpdate(std::atomic<Vec3>& dst, Vec3 src) {
        auto expected = dst.load();
        while (!dst.compare_exchange_weak(expected, src));
    }

public:
    virtual bool construct(const Json& prop) override {
        w_ = prop["w"];
        h_ = prop["h"];
        for (int i = 0; i < w_*h_; i++) {
            data_.emplace_back();
        }
        return true;
    }

    virtual FilmSize size() const override {
        return { w_, h_ };
    }

    virtual void setPixel(int x, int y, Vec3 v) override {
        atomicUpdate(data_[y*w_ + x], v);
        if (parallel::mainThread()) {
            const auto now = high_resolution_clock::now();
            const auto elapsed = duration_cast<milliseconds>(now - lastUpdated_);
            if (elapsed > 1s) {
                makeTemp();
                reportProgress_({ w_, h_, &dataTemp_[0].x });
                lastUpdated_ = now;
            }
        }
    }

    virtual bool save(const std::string& outpath) const override {
        LM_UNUSED(outpath);
        return true;
    }

    virtual FilmBuffer buffer() override {
        makeTemp();
        return FilmBuffer{ w_, h_, &dataTemp_[0].x };
    }

    virtual void regReporter(const ReportProgressFunc& reportProgress) override {
        reportProgress_ = reportProgress;
    }
};

LM_COMP_REG_IMPL(Film_Progress, "film::progress");

LM_NAMESPACE_END(LM_NAMESPACE)
