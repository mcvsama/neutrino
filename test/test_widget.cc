/* vim:ts=4
 *
 * Copyleft 2019  Michał Gawron
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
#include "test_widget.h"

// Qt:
#include <QResizeEvent>


namespace neutrino {

TestWidget::TestWidget (QSize size, si::Time loop_period, std::function<void (QPaintDevice&)> loop_body):
	QWidget (nullptr),
	_loop_body (loop_body)
{
	_refresh_timer = new QTimer (this);
	_refresh_timer->setSingleShot (false);
	_refresh_timer->setInterval (loop_period.in<si::Millisecond>());
	QObject::connect (_refresh_timer, &QTimer::timeout, this, &TestWidget::refresh);
	_refresh_timer->start();

	setWindowTitle ("xefis test");
	resize (size);
	update_canvas (size);
}


void
TestWidget::paintEvent (QPaintEvent*)
{
	auto r = rect();
	QPainter painter (this);
	painter.drawImage (r, _canvas, r);
	update();
}


void
TestWidget::resizeEvent (QResizeEvent* resize_event)
{
	update_canvas (resize_event->size());
}


void
TestWidget::refresh()
{
	_loop_body (_canvas);
	update();
}


void
TestWidget::update_canvas (QSize size)
{
	if (_canvas.isNull() || _canvas.size() != size)
	{
		_canvas = QImage (size, QImage::Format_ARGB32_Premultiplied);
		_canvas.fill (Qt::black);
	}
}

} // namespace neutrino

