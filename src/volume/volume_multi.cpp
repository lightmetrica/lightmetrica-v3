/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/volume.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: volume::multi

    Multiple volumes.

    :param volumes_alb: Array of references to volume albedos
    :param volumes_den: Array of references to volume densities
\endrst
*/
class Volume_Multi : public Volume {
private:
    Bound bound_;           // Boundary including all volumes
    std::vector<Volume*> volumes_den_;
    std::vector<Volume*> volumes_alb_;
    unsigned int size_;     // Size of volume arrays
    Float max_scalar_ = 0;  // Sum of maxScalar() of all Volumes in volumes_den_

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(bound_, max_scalar_, size_, volumes_den_, volumes_alb_);
    }

public:
    virtual void construct(const Json& prop) override {
        // Currently no straightforward way to use json::comp_ref<Volume>() on an array
        // Store all the ref strings
        auto vol_ref_alb = json::value< std::vector<std::string> >(prop,"volumes_alb");
        auto vol_ref_den = json::value< std::vector<std::string> >(prop,"volumes_den");
        if(!vol_ref_alb.size() || !vol_ref_den.size() || vol_ref_den.size()!=vol_ref_alb.size())
            LM_THROW_EXCEPTION(Error::InvalidArgument,
                "volumes_alb and/or volumes_den have an invalid size. They need to be of same size.");

        size_ = static_cast<unsigned int>(vol_ref_alb.size());
        // Load all components
        for( unsigned int i = 0; i < size_; i++ ){
            volumes_alb_.push_back( comp::get<Volume>(vol_ref_alb[i]) );
            volumes_den_.push_back( comp::get<Volume>(vol_ref_den[i]) );

            if( !volumes_alb_.back()->has_color() )
                LM_THROW_EXCEPTION(Error::InvalidArgument, "volumes_alb[{}] has no albedo/color", i);
            if( !volumes_den_.back()->has_scalar() )
                LM_THROW_EXCEPTION(Error::InvalidArgument, "volumes_den[{}] has no density", i);
        }

        // Computes the bounding box of all volumes
        Vec3 min = Vec3(Inf);
        Vec3 max = Vec3(-Inf);
        for(auto* v : volumes_den_) {
            const Bound b = v->bound();

            min.x = std::min(min.x,b.min.x);
            min.y = std::min(min.y,b.min.y);
            min.z = std::min(min.z,b.min.z);

            max.x = std::max(max.x,b.max.x);
            max.y = std::max(max.y,b.max.y);
            max.z = std::max(max.z,b.max.z);

            max_scalar_ += v->max_scalar();
        }
        bound_.min = min;
        bound_.max = max;

        LM_DEBUG("min bound: {}, {}, {}", bound_.min.x, bound_.min.y, bound_.min.z);
        LM_DEBUG("max bound: {}, {}, {}", bound_.max.x, bound_.max.y, bound_.max.z);
    }

    virtual Bound bound() const override {
        return bound_;
    }

    // This Volume requires to have color and scalar
    virtual bool has_scalar() const override {
        return true;
    }

    virtual Float max_scalar() const override {
        return max_scalar_;
    }

    bool isInBound(const Vec3& p, const Bound& b) const {
        auto isBetween=[](float a, float low, float high)->bool{
            return (low<=a && high>=a);
        };

        return isBetween(p.x,b.min.x,b.max.x) &&
               isBetween(p.y,b.min.y,b.max.y) &&
               isBetween(p.z,b.min.z,b.max.z);
    }
    
    // Computes the sum over all Volumes of eval_scalar
    virtual Float eval_scalar(Vec3 p) const override {
        Float sum = 0._f;
        for(auto* v : volumes_den_)
        {
            if(!isInBound(p,v->bound()))
                continue;
            sum += v->eval_scalar(p);
        }
       return sum;
    }

    // This Volume requires to have color and scalar
    virtual bool has_color() const override {
        return true;
    }

    // Accumulate contribution of the volumes (bounds check)
    virtual Vec3 eval_color(Vec3 p) const override {
        Float sum = 0;
        Vec3 resulting_color(0._f);
        
        for(unsigned int i = 0; i < size_; i++){
            if( !isInBound(p,volumes_den_[i]->bound()) )
                continue;
            //accumulate separately scalar and scalar times color
            Float sc = volumes_den_[i]->eval_scalar(p);
            resulting_color+= sc * volumes_alb_[i]->eval_color(p);
            sum+=sc;
        }
        //perform the scalar ratio
        return resulting_color/sum;
    }
};

LM_COMP_REG_IMPL(Volume_Multi, "volume::multi");

LM_NAMESPACE_END(LM_NAMESPACE)
