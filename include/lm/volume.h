/*
	Lightmetrica - Copyright (c) 2019 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
	\addtogroup volume
	@{
*/

/*!
	\brief Volume data.
*/
class Volume : public Component {
public:
	/*!
		\brief Get bound of the volume.
	*/
	virtual Bound bound() const = 0;

	/*!
		\brief Evaluate maximum density.
	*/
	virtual Float maxDensity() const = 0;

	/*!
		\brief Evaluate density.
	*/
	virtual Float evalDensity(Vec3 p) const = 0;

	/*!
		\brief Evaluate albedo.
	*/
	virtual Vec3 evalAlbedo(Vec3 p) const = 0;

	/*!
		\brief Callback function called for ray marching.
	*/
	using RayMarchFunc = std::function<bool(Vec3 p)>;

	/*!
		\brief March the volume along with the ray.
		\param marchStep Ray marching step in world space.

		\rst
		This function performs volume-specific ray marching operations.
		\endrst
	*/
	virtual void march(Ray ray, Float tmin, Float tmax, Float marchStep, const RayMarchFunc& rayMarchFunc) const {
		LM_UNUSED(ray, tmin, tmax, marchStep, rayMarchFunc);
		LM_THROW_UNIMPLEMENTED();
	}
};

/*!
	@}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
