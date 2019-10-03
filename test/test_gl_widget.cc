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
#include "test_gl_widget.h"

// Qt:
#include <QResizeEvent>


namespace neutrino {

TestGLWidget::TestGLWidget (QSize size, si::Time loop_period, std::function<void (QOpenGLPaintDevice&)> loop_body):
	QWindow (static_cast<QWindow*> (nullptr)),
	_loop_body (loop_body)
{
	setSurfaceType (QWindow::OpenGLSurface);

	_refresh_timer = new QTimer (this);
	_refresh_timer->setSingleShot (false);
	_refresh_timer->setInterval (loop_period.in<si::Millisecond>());
	QObject::connect (_refresh_timer, &QTimer::timeout, this, &TestGLWidget::refresh);
	_refresh_timer->start();

	setTitle ("xefis test");
	resize (size);
	create();
}


void
TestGLWidget::refresh()
{
	if (isExposed())
	{
		if (!_open_gl_context)
		{
			auto format = requestedFormat();
			format.setDepthBufferSize (24);
			format.setProfile (QSurfaceFormat::CoreProfile);
			// OpenGL antialiasing:
			format.setSamples (8);

			_open_gl_context = std::make_unique<QOpenGLContext> (this);
			_open_gl_context->setFormat (format);
			_open_gl_context->create();
		}

		if (_open_gl_context->makeCurrent (this))
		{
			if (!_open_gl_device)
				_open_gl_device = std::make_unique<QOpenGLPaintDevice>();

			_open_gl_device->setSize (size() * devicePixelRatio());
			_open_gl_device->setDevicePixelRatio (devicePixelRatio());

			// Paint black background, reset z-buffer and stencil buffer:
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			_loop_body (*_open_gl_device);
			_open_gl_context->swapBuffers (this);
		}
		else
			std::cerr << "Could not make OpenGL context current.\n";
	}
}

} // namespace neutrino

