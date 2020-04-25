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
.. function:: volume::multigaussian

    Multiple Gaussian volumes.

    :param volumes_alb: Array of references to volume albedos
    :param volumes_den: Array of references to volume densities
\endrst
*/
class Volume_Multi : public Volume {
private:
    Bound bound_; // Boundary including all volumes
    std::vector< Volume* > volumes_den_;
    std::vector< Volume* > volumes_alb_;
    unsigned int size_; // Size of volume arrays
    Float maxScalar_ = 0; // Sum of maxScalar() of all Volumes in volumes_den_
    Rng* rng; // Currently no way to get current thread's rng, fallback to this
public:
	LM_SERIALIZE_IMPL(ar) {
		ar(bound_);
	}

public:
	virtual void construct(const Json& prop) override {
        // Currently no straightforward way to use json::comp_ref<Volume>() on an array
        // Store all the ref strings
        auto volRefAlb = json::value< std::vector<std::string> >(prop,"volumes_alb");
        auto volRefDen = json::value< std::vector<std::string> >(prop,"volumes_den");
        if(!volRefAlb.size() || !volRefDen.size() || volRefDen.size()!=volRefAlb.size())
            LM_THROW_EXCEPTION(Error::InvalidArgument,
                "volumes_alb and/or volumes_den have an invalid size. They need to be of same size.");
            
        size_ = volRefAlb.size();
        // Load all components
        for( int i = 0; i < size_; i++ ){
            volumes_alb_.push_back( comp::get<Volume>(volRefAlb[i]) );
            volumes_den_.push_back( comp::get<Volume>(volRefDen[i]) );

            if( !volumes_alb_.back()->has_color() ) 
                LM_THROW_EXCEPTION(Error::InvalidArgument, "volumes_alb[{}] has no albedo/color",i);
            if( !volumes_den_.back()->has_scalar() )
                LM_THROW_EXCEPTION(Error::InvalidArgument, "volumes_den[{}] has no density",i);
        }
        
        // Computes the bounding box of all volumes
        Vec3 min = Vec3(Inf);
        Vec3 max = Vec3(-Inf)
        for( auto* v : volumes_den_ ){
            const Bound b = v->bound();

            min.x = std::min(min.x,b.min.x);
            min.y = std::min(min.y,b.min.y);
            min.z = std::min(min.z,b.min.z);

            max.x = std::max(max.x,b.max.x);
            max.y = std::max(max.y,b.max.y);
            max.z = std::max(max.z,b.max.z);
            
            maxScalar_ += v->max_scalar();
        }
        bound_.min = min;
        bound_.max = max;
    
        LM_DEBUG("min bound: {}, {}, {}", bound_.min.x, bound_.min.y, bound_.min.z);
        LM_DEBUG("max bound: {}, {}, {}", bound_.max.x, bound_.max.y, bound_.max.z);

        rng = new Rng(math::rng_seed());
	}
	
	virtual Bound bound() const override {
		return bound_;
	}

// This Volume requires to have color and scalar
	virtual bool has_scalar() const override {
		return true;
	}

	virtual Float max_scalar() const override {
		return maxScalar_;
	}

// Computes the sum over all Volumes of eval_scalar
	virtual Float eval_scalar(Vec3 p) const override {
        Float sum = 0._f;
        for( auto* v : volumes_den_ )// Compiler should optimize these
            sum += v->eval_scalar(p);
       return sum;
	}

// This Volume requires to have color and scalar
	virtual bool has_color() const override {
		return true;
	}

// Compute the color by randomly selecting within the different gaussians
// depending on scalar value at position 
	virtual Vec3 eval_color(Vec3 p) const override {
        std::vector<Float> scalars;
        Float sum = 0;
        scalars.reserve(size_);

        // Perform a prefix sum of scalar evaluations
        for( auto* v : volumes_den_ ){
            sum += v->eval_scalar(p);
            scalars.push_back(sum);
        }        
        // Normalize the prefix sum to use as probabilities ranges
        Float s = scalars.back();
        for( auto& sc : scalars )
            sc /= s;
            
        // Computes which volume provides the color stochastically
        // Ensures the while loop exits since rng->u() give float<=1
        scalars[ scalars.size() - 1 ] = 1.0_f + Eps;
        Float r = rng->u();
        int current = 0; 
        while( r >= scalars[current] )
            current++;
        return volumes_alb_[current]->eval_color(p);
		
	}
};

LM_COMP_REG_IMPL(Volume_Multi, "volume::multi");

LM_NAMESPACE_END(LM_NAMESPACE)
