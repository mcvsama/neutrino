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

// Local:
#include "qutils.h"

// Neutrino:
#include <neutrino/stdexcept.h>

// Qt:
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>

// Standard:
#include <cstddef>


namespace neutrino {

extern float
default_line_height (QWidget* widget)
{
	QFont font = QApplication::font();
	float dpi = 72.0f;

	if (widget)
		dpi = widget->physicalDpiY();
	else if (auto* primary_screen = QApplication::primaryScreen())
		dpi = primary_screen->physicalDotsPerInch();

	return font.pointSize() * pixels_per_point (si::PixelDensity (dpi));
}


extern void
setup_appereance (QTreeWidgetItem& item)
{
	QSize s = item.sizeHint (0);

	if (!item.treeWidget())
		throw InvalidArgument ("setup_appereance (QTreeWidgetItem&) requires item to be inserted into a tree");

	s.setHeight (1.75 * default_line_height (item.treeWidget()));
	item.setSizeHint (0, s);
}


void
break_ownership (QObject& parent, QObject& child)
{
	auto child_still_exists = std::make_shared<bool> (true);

	QObject::connect (&parent, &QObject::destroyed, [&child, child_still_exists] (QObject*) {
		if (*child_still_exists)
			child.setParent (nullptr);
	});

	QObject::connect (&child, &QObject::destroyed, [child_still_exists] (QObject*) {
		*child_still_exists = false;
	});
}

} // namespace neutrino

