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

#ifndef NEUTRINO__TEST__DUMMY_QAPPLICATION_H__INCLUDED
#define NEUTRINO__TEST__DUMMY_QAPPLICATION_H__INCLUDED

// Standard:
#include <cstddef>
#include <memory>
#include <string>

// Qt:
#include <QApplication>


namespace neutrino {

class DummyQApplication
{
  public:
	int								argc;
	char**							argv;
	std::unique_ptr<QApplication>	app;

  public:
	// Ctor
	explicit
	DummyQApplication()
	{
		argc = 1;
		argv = new char*[1];
		argv[0] = new char[5];
		std::string ("none").copy (argv[0], 4);
		argv[0][4] = '\0';
		app = std::make_unique<QApplication> (argc, argv);
	}

	// Dtor
	~DummyQApplication()
	{
		delete[] argv[0];
		delete[] argv;
	}

	QApplication*
	operator->()
	{
		return app.get();
	}
};

} // namespace neutrino

#endif

