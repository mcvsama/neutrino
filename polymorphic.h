/* vim:ts=4
 *
 * Copyleft 2026  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__POLYMORPHIC_H__INCLUDED
#define NEUTRINO__POLYMORPHIC_H__INCLUDED


namespace neutrino {

class Polymorphic
{
  protected:
	/**
	 * Make this constructor protected to prevent creating bare Polymorphic.
	 * It wouldn't make sense and would indicate a logic bug in the code.
	 */
	explicit
	Polymorphic() = default;

  public:
	// Dtor
	virtual
	~Polymorphic() = default;
};

} // namespace neutrino

#endif
