/* vim:ts=4
 *
 * Copyleft 2012-2024  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__SI__EXCEPTION_H__INCLUDED
#define NEUTRINO__SI__EXCEPTION_H__INCLUDED

// Local:
#include "si_config.h"

// Standard:
#include <cstddef>


namespace neutrino::si {

class DynamicUnit;

using Exception = si_config::Exception;


class UnparsableValue: public Exception
{
  public:
	using Exception::Exception;
};


class UnsupportedUnit: public Exception
{
  public:
	using Exception::Exception;
};


class IncompatibleTypes: public Exception
{
  public:
	using Exception::Exception;

	explicit
	IncompatibleTypes (DynamicUnit const& got, DynamicUnit const& expected, std::optional<bool> include_backtrace = std::nullopt);
};

} // namespace neutrino::si

#endif
