/* vim:ts=4
 *
 * Copyleft 2025  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__QT__QFONTMETRICS_H__INCLUDED
#define NEUTRINO__QT__QFONTMETRICS_H__INCLUDED

// Qt:
#include <QFontMetrics>

// Standard:
#include <cstddef>


namespace neutrino {

/**
 * In Qt6 there's a bug in QFontMetrics where .height() returns wrong (too small) values.
 * The reason is that it returns ascent() + descent() and descent() is buggy - it returns
 * negative values (despite being documented as returning a distance).
 *
 * This is a workaround.
 */
inline int
line_height (QFontMetrics const& metrics)
{
	return metrics.ascent() + std::abs (metrics.descent());
}


/**
 * Overload for QFontMetricsF.
 */
inline float
line_height (QFontMetricsF const& metrics)
{
	return metrics.ascent() + std::abs (metrics.descent());
}

} // namespace neutrino

#endif

