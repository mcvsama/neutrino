/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__MATH__NORMAL_DISTRIBUTION_H__INCLUDED
#define NEUTRINO__MATH__NORMAL_DISTRIBUTION_H__INCLUDED

// Neutrino:
#include <neutrino/math/normal_variable.h>
#include <neutrino/si/utils.h>

// Standard:
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>


namespace neutrino::math {

/**
 * A wrapper for std::normal_distribution that allows using non-fundamental types as values.
 * A zero standard deviation produces the mean exactly, without advancing the generator.
 * Parameters must be finite and the standard deviation must be nonnegative.
 */
template<class pValue>
	class NormalDistribution
	{
	  public:
		using Value = pValue;

	  public:
		/**
		 * Throw std::invalid_argument for invalid parameters, or if converting a positive-deviation
		 * configuration to double would overflow or round its deviation to zero.
		 */
		NormalDistribution (Value mean, Value stddev);

		// Ctor
		NormalDistribution (NormalVariable<Value> const&);

		// Ctor
		NormalDistribution (NormalDistribution const&) = default;

		// Copy operator
		NormalDistribution&
		operator= (NormalDistribution const&) = default;

		void
		reset()
			{ _dist.reset(); }

		Value
		mean() const noexcept
			{ return _mean; }

		Value
		stddev() const noexcept
			{ return _stddev; }

		template<class Generator>
			Value
			operator() (Generator& g);

	  private:
		/**
		 * Validate parameters before constructing std::normal_distribution.
		 * Use a valid dummy distribution for the constant case.
		 */
		static std::normal_distribution<>
		make_distribution (Value const& mean, Value const& stddev);

	  private:
		Value						_mean;
		Value						_stddev;
		std::normal_distribution<>	_dist;
	};


template<class V>
	inline
	NormalDistribution<V>::NormalDistribution (Value mean, Value stddev):
		_mean (mean),
		_stddev (stddev),
		_dist (make_distribution (mean, stddev))
	{ }


template<class V>
	inline
	NormalDistribution<V>::NormalDistribution (NormalVariable<Value> const& var):
		NormalDistribution (var.mean(), var.stddev())
	{ }


template<class V>
	template<class Generator>
		inline auto
		NormalDistribution<V>::operator() (Generator& g) -> Value
		{
			if (_stddev == Value())
				return _mean;

			return Value (1) * _dist (g);
		}


template<class V>
	inline std::normal_distribution<>
	NormalDistribution<V>::make_distribution (Value const& mean, Value const& stddev)
	{
		auto const raw_mean = si::value (mean);
		auto const raw_stddev = si::value (stddev);

		if (!std::isfinite (raw_mean) || !std::isfinite (raw_stddev) || raw_stddev < 0)
			throw std::invalid_argument ("normal distribution requires finite parameters and a nonnegative standard deviation");

		if (raw_stddev == 0)
			return std::normal_distribution<> {};

		auto const max = std::numeric_limits<double>::max();

		if (raw_mean < -max || raw_mean > max || raw_stddev > max)
			throw std::invalid_argument ("normal distribution parameters do not fit in double");

		auto const converted_stddev = static_cast<double> (raw_stddev);

		if (converted_stddev == 0)
			throw std::invalid_argument ("normal distribution standard deviation underflows in double");

		return std::normal_distribution<> (static_cast<double> (raw_mean), converted_stddev);
	}

} // namespace neutrino::math

#endif
