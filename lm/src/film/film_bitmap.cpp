/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/film.h>
#include <lm/logger.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Film_Bitmap final : public Film {
private:
    int w_;
    int h_;
    std::vector<Vec3> data_;

public:
    virtual bool construct(const Json& prop) override {
        w_ = prop["w"];
        h_ = prop["h"];
        data_.assign(w_ * h_, Vec3(0_f));
        return true;
    }

    virtual FilmSize size() const override {
        return { w_, h_ };
    }

    virtual void setPixel(int x, int y, Vec3 v) override {
        data_[y*w_ + x] = v;
    }

    virtual bool save(const std::string& outpath) const override {
        LM_INFO("Saving image [file='{}']", outpath);
        FILE *f;
        errno_t err;
        if ((err = fopen_s(&f, outpath.c_str(), "wb")) != 0) {
            LM_ERROR("Failed to open [file='{}',errorno='{}']", outpath, err);
            return false;
        }
        fprintf(f, "PF\n%d %d\n-1\n", w_, h_);
        std::vector<float> d(w_*h_*3);
        for (int y = 0; y < h_; y++) {
            for (int x = 0; x < w_; x++) {
                for (int i = 0; i < 3; i++) {
                    d[3*(y*w_+x)+i] = float(data_[(h_-1-y)*w_+(w_-1-x)][i]);
                }
            }
        }
        fwrite(d.data(), 4, d.size(), f);
        fclose(f);
        return true;
    }

    virtual FilmBuffer buffer() override {
        return FilmBuffer{ w_, h_, &data_[0].x };
    }
};

LM_COMP_REG_IMPL(Film_Bitmap, "film::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
