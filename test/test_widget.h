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

#ifndef NEUTRINO__TEST__TEST_WIDGET_H__INCLUDED
#define NEUTRINO__TEST__TEST_WIDGET_H__INCLUDED

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/si/si.h>

// Qt:
#include <QApplication>
#include <QPainter>
#include <QPaintDevice>
#include <QTimer>
#include <QWidget>


namespace neutrino {

/**
 * Calls specified painting function in a loop with specified frequency.
 * The painted images are animated in a widget.
 */
class TestWidget: public QWidget
{
  public:
	// Ctor
	explicit
	TestWidget (QSize size, si::Time loop_period, std::function<void (QPaintDevice&)> loop_body);

  private:
	// QWidget API
	void
	paintEvent (QPaintEvent*) override;

	// QWidget API
	void
	resizeEvent (QResizeEvent* resize_event) override;

	void
	refresh();

	void
	update_canvas (QSize size);

  private:
	std::function<void (QPaintDevice&)>	_loop_body;
	QImage								_canvas;
	QTimer*							    _refresh_timer;
};

} // namespace neutrino

#endif

