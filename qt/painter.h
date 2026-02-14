/* vim:ts=4
 *
 * Copyleft 2024  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__QT__PAINTER_H__INCLUDED
#define NEUTRINO__QT__PAINTER_H__INCLUDED

// Neutrino:
#include <neutrino/si/si.h>

// Standard:
#include <cstddef>


namespace neutrino {

/**
 * Change arc degrees to values used by QPainter when painting arcs and pies.
 */
constexpr float
angle_for_qpainter (si::Angle const angle)
{
	return 16 * angle.in<si::Degree>();
}

} // namespace neutrino

namespace neutrino::painter_literals {

/**
 * Change arc degrees to values used by QPainter when painting arcs and pies.
 */
constexpr float
operator""_qarcdeg (long double const angle)
{
	return angle_for_qpainter (1_deg * angle);
}


/**
 * Change arc degrees to values used by QPainter when painting arcs and pies.
 */
constexpr float
operator""_qarcdeg (unsigned long long const angle)
{
	return angle_for_qpainter (1_deg * angle);
}

} // namespace neutrino::painter_literals

#endif
