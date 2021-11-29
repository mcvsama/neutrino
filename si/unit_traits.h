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

#ifndef NEUTRINO__SI__UNIT_TRAITS_H__INCLUDED
#define NEUTRINO__SI__UNIT_TRAITS_H__INCLUDED

// Standard:
#include <cstddef>
#include <string>
#include <map>

// Local:
#include "concepts.h"
#include "exception.h"
#include "unit.h"


namespace neutrino::si {

/**
 * The main unit traits class.
 */
template<UnitConcept pUnit>
	class UnitTraits
	{
	  public:
		using Unit = pUnit;

	  public:
		/**
		 * Return full name of the unit.
		 */
		[[nodiscard]]
		static std::string
		name()
			{ return "unnamed"; }

		/**
		 * Return short symbol.
		 */
		[[nodiscard]]
		static std::string
		symbol()
			{ return Unit::dynamic_unit().symbol(); }

		/**
		 * Return alternative symbols.
		 */
		[[nodiscard]]
		static std::vector<std::string>
		alternative_symbols()
			{ return { }; }
	};


/**
 * Default implementations of methods.
 */
class DefaultUnitTraits
{
  public:
	[[nodiscard]]
	static std::vector<std::string>
	alternative_symbols()
		{ return { }; }
};

} // namespace neutrino::si

#endif

