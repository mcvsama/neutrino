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

#ifndef NEUTRINO__QT__QPALETTE_H__INCLUDED
#define NEUTRINO__QT__QPALETTE_H__INCLUDED

// Qt:
#include <QEvent>
#include <QPalette>
#include <QWidget>

// Standard:
#include <cstddef>


namespace neutrino {

[[nodiscard]]
inline bool
is_light_theme (QPalette const& palette)
{
	return palette.color (QPalette::Active, QPalette::Base).lightness() >= 128;
}


[[nodiscard]]
inline bool
is_theme_change (QEvent const* event)
{
	if (!event)
		return false;

	switch (event->type())
	{
		case QEvent::PaletteChange:
		case QEvent::ApplicationPaletteChange:
		case QEvent::StyleChange:
		case QEvent::ThemeChange:
			return true;

		default:
			return false;
	}
}

} // namespace neutrino

#endif
