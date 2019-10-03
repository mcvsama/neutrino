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

#ifndef NEUTRINO__TEST__TEST_GL_WIDGET_H__INCLUDED
#define NEUTRINO__TEST__TEST_GL_WIDGET_H__INCLUDED

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/si/si.h>

// Qt:
#include <QApplication>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QOpenGLWidget>
#include <QPainter>
#include <QTimer>
#include <QWindow>


namespace neutrino {

/**
 * Calls specified painting function in a loop with specified frequency.
 * The painted images are animated in a widget.
 */
class TestGLWidget: public QWindow
{
  public:
	// Ctor
	explicit
	TestGLWidget (QSize size, si::Time loop_period, std::function<void (QOpenGLPaintDevice&)> loop_body);

  private:
	void
	refresh();

  private:
	std::unique_ptr<QOpenGLContext>				_open_gl_context;
	std::unique_ptr<QOpenGLPaintDevice>			_open_gl_device;
	std::function<void (QOpenGLPaintDevice&)>	_loop_body;
	QTimer*										_refresh_timer;
};

} // namespace neutrino

#endif

