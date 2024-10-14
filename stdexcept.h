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

#ifndef NEUTRINO__STDEXCEPT_H__INCLUDED
#define NEUTRINO__STDEXCEPT_H__INCLUDED

// Neutrino:
#include <neutrino/exception.h>

// Standard:
#include <cstddef>
#include <exception>


namespace neutrino {

class WrongDestructionOrder: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * General configuration error.
 */
class BadConfiguration: public Exception
{
  public:
	// Ctor
	explicit
	BadConfiguration (const char* message):
		Exception (message, false)
	{ }

	// Ctor
	explicit
	BadConfiguration (std::string const& message):
		Exception (message, false)
	{ }

	// Ctor
	explicit
	BadConfiguration (QString const& message):
		Exception (message, false)
	{ }
};


/**
 * General IO error.
 */
class IOError: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * Invalid format during conversion (eg. forbidden character in numeric string).
 */
class InvalidFormat: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * Invalid call (a function should not be called by user, etc).
 */
class InvalidCall: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * Invalid argument passed to a function.
 */
class InvalidArgument: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * Precondition failed when calling a function.
 */
class PreconditionFailed: public Exception
{
  public:
	using Exception::Exception;
};


/**
 * Thrown by parse() functions.
 */
class ParseException: public Exception
{
  public:
	using Exception::Exception;
};

} // namespace neutrino

#endif

