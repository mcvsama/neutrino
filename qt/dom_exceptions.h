/* vim:ts=4
 *
 * Copyleft 2024  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__QT__DOM_EXCEPTIONS_H__INCLUDED
#define NEUTRINO__QT__DOM_EXCEPTIONS_H__INCLUDED

// Neutrino:
#include <neutrino/exception.h>

// Qt:
#include <QtXml/QDomElement>

// Standard:
#include <cstddef>


namespace neutrino {

class DomException: public Exception
{
  public:
	using Exception::Exception;

  protected:
	static QString
	get_path (QDomElement const& element)
	{
		QString result;
		QDomNode node = element;

		while (!node.isNull() && node.isElement())
		{
			result = "/" + node.toElement().tagName() + result;
			node = node.parentNode();
		}

		return result;
	}
};


/**
 * Throw to indicate that subelement required but missing.
 */
class MissingDomElement: public DomException
{
  public:
	// Ctor
	explicit
	MissingDomElement (QDomElement const& parent, QString const& child_name):
		DomException ("missing subelement <" + child_name + "> in " + get_path (parent))
	{ }
};


/**
 * Throw when an element is malformed.
 */
class MalformedDomElement: public DomException
{
  public:
	// Ctor
	explicit
	MalformedDomElement (QDomElement const& element, QString const& additional_message = QString()):
		DomException ("element '" + element.tagName() + "' is malformed in " + get_path (element) +
					  (additional_message.isEmpty() ? "" : ("; " + additional_message)))
	{ }

	// Ctor
	MalformedDomElement (QString const& message):
		DomException (message)
	{ }
};


/**
 * Throw when an element is not supported in given context.
 */
class UnexpectedDomElement: public DomException
{
  public:
	// Ctor
	explicit
	UnexpectedDomElement (QDomElement const& element, QString const& additional_message = QString()):
		DomException ("element '" + element.tagName() + "' is not supported in " + get_path (element) +
					  (additional_message.isEmpty() ? "" : ("; " + additional_message)))
	{ }

	// Ctor
	UnexpectedDomElement (QString const& message):
		DomException (message)
	{ }
};


/**
 * Throw to indicate that element needs an attribute.
 */
class MissingDomAttribute: public DomException
{
  public:
	// Ctor
	explicit
	MissingDomAttribute (QDomElement const& element, QString const& attribute_name):
		DomException ("element <" + element.tagName() + "> needs attribute '" + attribute_name + "'")
	{ }
};


/**
 * Throw to indicate that an attribute has invalid value.
 */
class BadDomAttribute: public DomException
{
  public:
	// Ctor
	explicit
	BadDomAttribute (QDomElement const& element, QString const& attribute_name, QString const& message = QString()):
		DomException ("invalid value for attribute '" + attribute_name + "' in " + get_path (element) + ": " + message)
	{ }
};



} // namespace neutrino

#endif

