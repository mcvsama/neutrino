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
 * --
 * Here be basic, global functions and macros like asserts, debugging helpers, etc.
 */

#ifndef NEUTRINO__EXCEPTION_H__INCLUDED
#define NEUTRINO__EXCEPTION_H__INCLUDED

// Neutrino:
#include <neutrino/backtrace.h>
#include <neutrino/demangle.h>

// Qt:
#include <QtCore/QString>

// Boost:
#include <boost/exception/all.hpp>

// Standard:
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <functional>


namespace neutrino {

class Logger;


class Exception: public std::exception
{
  public:
	/**
	 * Create exception object.
	 * \param	message
	 *			Message for user. Don't capitalize first letter, and don't add dot at the end.
	 *			It should be a simple phrase, that can be embedded into a bigger sentence.
	 */
	explicit
	Exception (const char* message, std::exception_ptr nested_exception = nullptr, bool include_backtrace = true);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (std::string_view const message, std::exception_ptr nested_exception = nullptr, bool include_backtrace = true);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (std::string const& message, std::exception_ptr nested_exception = nullptr, bool include_backtrace = true);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (QString const& message, std::exception_ptr nested_exception = nullptr, bool include_backtrace = true);

	// Dtor
	virtual
	~Exception() noexcept;

	// Ctor
	Exception (Exception const&) = default;

	/**
	 * Return plain exception message.
	 */
	[[nodiscard]]
	const char*
	what() const noexcept;

	/**
	 * Return combined exception message.
	 */
	[[nodiscard]]
	std::string const&
	message() const;

	/**
	 * Return backtrace created when the exception was constructed.
	 */
	[[nodiscard]]
	Backtrace const&
	backtrace() const;

	/**
	 * Return nested exception ptr.
	 */
	[[nodiscard]]
	std::exception_ptr
	nested_exception() const
		{ return _nested_exception; }

	/**
	 * Return true if backtrace is not to be shown to user.
	 */
	[[nodiscard]]
	bool
	backtrace_hidden() const noexcept;

	/**
	 * Execute guarded_code and catch exceptions. If exception is catched,
	 * it's logged and rethrown. If exception is of type neutrino::Exceptions,
	 * it's full message is logged (backtrace, etc). Boost and standard
	 * exceptions are logged just by their typeid().name. Other types
	 * just cause mentioning an exception.
	 */
	static void
	log (Logger const&, std::function<void()> guarded_code);

	/**
	 * Log provided exception.
	 */
	static void
	log (Logger const&, std::exception_ptr const&);

	/**
	 * Similar to log, but doesn't rethrow.
	 * Returns true if exception was thrown and catched.
	 */
	static bool
	catch_and_log (Logger const&, std::function<void()> guarded_code);

	/**
	 * Terminate program after printing message on std::cerr.
	 */
	[[noreturn]]
	static void
	terminate (std::string_view message);

  protected:
	/**
	 * Hides backtrace when put to std::ostream.
	 * Useful for configuration exceptions, where a backtrace
	 * would be rather confusing.
	 */
	void
	hide_backtrace() noexcept;

  private:
	bool				_hide_backtrace	= false;
	std::string			_what;
	std::string			_message;
	Backtrace			_backtrace;
	std::exception_ptr	_nested_exception;
};


/**
 * Does not save backtrace when created.
 */
class FastException: public Exception
{
  public:
	// Ctor
	explicit
	FastException (const char* message, std::exception_ptr nested_exception = nullptr):
		Exception (message, nested_exception, false)
	{ }

	// Ctor
	explicit
	FastException (std::string_view const message, std::exception_ptr nested_exception = nullptr):
		Exception (message, nested_exception, false)
	{ }

	// Ctor
	explicit
	FastException (std::string const& message, std::exception_ptr nested_exception = nullptr):
		Exception (message, nested_exception, false)
	{ }

	// Ctor
	explicit
	FastException (QString const& message, std::exception_ptr nested_exception = nullptr):
		Exception (message, nested_exception, false)
	{ }

	// Ctor
	FastException (FastException const&) = default;
};


inline
Exception::Exception (const char* message, std::exception_ptr const nested_exception, bool include_backtrace):
	Exception (std::string (message), nested_exception, include_backtrace)
{ }


inline
Exception::Exception (std::string_view const message, std::exception_ptr const nested_exception, bool include_backtrace):
	Exception (std::string (message), nested_exception, include_backtrace)
{ }


inline
Exception::Exception (std::string const& message, std::exception_ptr const nested_exception, bool include_backtrace):
	_what (message),
	_message (message),
	_nested_exception (nested_exception)
{
	if (include_backtrace)
		_backtrace = neutrino::backtrace();
}


inline
Exception::Exception (QString const& message, std::exception_ptr const nested_exception, bool include_backtrace):
	Exception (message.toStdString(), nested_exception, include_backtrace)
{ }


inline
Exception::~Exception() noexcept
{ }


inline const char*
Exception::what() const noexcept
{
	return _what.c_str();
}


inline std::string const&
Exception::message() const
{
	return _message;
}


inline Backtrace const&
Exception::backtrace() const
{
	return _backtrace;
}


inline bool
Exception::backtrace_hidden() const noexcept
{
	return _hide_backtrace;
}


inline void
Exception::hide_backtrace() noexcept
{
	_hide_backtrace = true;
}


namespace exception_ops {

std::ostream&
operator<< (std::ostream&, Exception const&);

std::ostream&
operator<< (std::ostream&, std::exception_ptr const&);

} // namespace exception_ops
} // namespace neutrino

#endif
