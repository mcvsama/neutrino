/* vim:ts=4
 *
 * Copyleft 2012…2017  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__SI__LONLAT_RADIUS_H__INCLUDED
#define NEUTRINO__SI__LONLAT_RADIUS_H__INCLUDED

// Standard:
#include <cstddef>


namespace neutrino::si {

using namespace literals;
using quantities::Angle;
using quantities::Length;


/**
 * Polar coordinates on Earth.
 */
template<class LengthType = Length>
	class LonLatRadius: public LonLat
	{
	  public:
		using Length = LengthType;

	  public:
		// Ctor
		constexpr
		LonLatRadius() noexcept = default;

		// Ctor
		explicit constexpr
		LonLatRadius (LonLat const&, Length radius);

		// Ctor
		explicit constexpr
		LonLatRadius (Angle longitude, Angle latitude, Length radius);

		[[nodiscard]]
		constexpr Length&
		radius() noexcept
			{ return _radius; }

		[[nodiscard]]
		constexpr Length
		radius() const noexcept
			{ return _radius; }

	  private:
		Length _radius { 0 };
	};


template<class L>
	constexpr
	LonLatRadius<L>::LonLatRadius (LonLat const& lonlat, Length radius):
		LonLat (lonlat),
		_radius (radius)
	{ }


template<class L>
	constexpr
	LonLatRadius<L>::LonLatRadius (Angle longitude, Angle latitude, Length radius):
		LonLat (longitude, latitude),
		_radius (radius)
	{ }

} // namespace neutrino::si

#endif
