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

#ifndef NEUTRINO__QT__QSIGNALS_BLOCKER_H__INCLUDED
#define NEUTRINO__QT__QSIGNALS_BLOCKER_H__INCLUDED

// Neutrino:
#include <neutrino/responsibility.h>

// Qt:
#include <QtWidgets/QWidget>

// Standard:
#include <cstddef>


namespace neutrino {

/**
 * RAII wrapper for QWidget::blockSignals().
 */
class QSignalsBlocker
{
  public:
	// Ctor
	// \param	widget
	//			May be nullptr.
	explicit
	QSignalsBlocker (QWidget* widget);

	// Ctor
	~QSignalsBlocker();

  private:
	QWidget*	_widget;
	bool		_signals_blocked	{ false };
};


inline
QSignalsBlocker::QSignalsBlocker (QWidget* widget):
	_widget (widget)
{
	if (_widget)
		_signals_blocked = _widget->blockSignals (true);
}


inline
QSignalsBlocker::~QSignalsBlocker()
{
	if (_widget)
		_widget->blockSignals (_signals_blocked);
}

} // namespace xf

#endif

