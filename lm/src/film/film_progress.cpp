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

// Wrapper for std::atomic.
// Extending std::vector with std::atomic generates a compilation error
// because std::atomic is neither copy nor move constructable.
// cf. https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
template <typename T>
struct AtomicWrapper {
    std::atomic<T> v_;
    AtomicWrapper() = default;
    AtomicWrapper(T& v) : v_(v) {}
    AtomicWrapper(const std::atomic<T>& v) : v_(v.load()) {}
    AtomicWrapper(const AtomicWrapper& o) : v_(o.v_.load()) {}
    AtomicWrapper& operator=(const AtomicWrapper& o) { v_.store(o.v_.load()); return *this; }
    void update(const T& src) {
        auto expected = v_.load();
        while (!v_.compare_exchange_weak(expected, src));
    }
};

class Film_Progress final : public Film {
private:
    int w_;
    int h_;
    std::vector<AtomicWrapper<Vec3>> data_;
    std::vector<Vec3> dataTemp_;  // Temporary buffer for external reference
    time_point<high_resolution_clock> lastUpdated_;
    ReportProgressFunc reportProgress_;

private:
    void makeTemp() {
        dataTemp_.clear();
        for (const auto& v : data_) {
            dataTemp_.push_back(v.v_);
        }
    }

public:
    virtual bool construct(const Json& prop) override {
        w_ = prop["w"];
        h_ = prop["h"];
        data_.assign(w_*h_, {});
        return true;
    }

    virtual FilmSize size() const override {
        return { w_, h_ };
    }

    virtual void setPixel(int x, int y, Vec3 v) override {
        data_[y*w_ + x].update(v);
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
