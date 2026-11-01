/* vim:ts=4
 *
 * Copyleft 2026  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__MATH__NORM_H__INCLUDED
#define NEUTRINO__MATH__NORM_H__INCLUDED

// Neutrino:
#include <neutrino/math/concepts.h>
#include <neutrino/si/utils.h>

// Standard:
#include <array>
#include <cmath>
#include <cstddef>


namespace neutrino::math::detail {

/**
 * Compute a Euclidean norm without squaring the original components.
 * Scale before accumulating with hypot() to avoid repeated rounding of subnormal partial norms.
 * Quantities retain their stored unit and numerical type; no unit conversion is needed.
 */
template<Scalar S, std::size_t Size>
	[[nodiscard]]
	constexpr S
	stable_norm (std::array<S, Size> const& components) noexcept
	{
		using Value = si::decay_quantity_t<S>;
		Value scale { 0 };

		for (auto const& component: components)
		{
			auto const magnitude = std::abs (si::value (component));

			// Like binary hypot(), infinity takes precedence over NaN, regardless of order.
			if (std::isinf (magnitude))
				return S (magnitude);

			if (std::isnan (magnitude) || magnitude > scale)
				scale = magnitude;
		}

		if (scale == Value (0) || std::isnan (scale))
			return S (scale);

		Value norm { 0 };

		for (auto const& component: components)
			norm = std::hypot (norm, si::value (component) / scale);

		return S (scale * norm);
	}

} // namespace neutrino::math::detail

#endif
