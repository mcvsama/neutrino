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
#include <atomic>
#include <cstdio>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <functional>


namespace neutrino {

class Logger;

inline std::atomic<bool> enable_backtraces = false;


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
	Exception (const char* message, std::optional<bool> include_backtrace = std::nullopt);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (std::string_view const message, std::optional<bool> include_backtrace = std::nullopt);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (std::string const& message, std::optional<bool> include_backtrace = std::nullopt);

	/**
	 * Convenience function.
	 */
	explicit
	Exception (QString const& message, std::optional<bool> include_backtrace = std::nullopt);

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
	std::optional<Backtrace> const&
	backtrace() const;

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

  private:
	std::string					_what;
	std::string					_message;
	std::optional<Backtrace>	_backtrace;
};


inline
Exception::Exception (const char* message, std::optional<bool> include_backtrace):
	Exception (std::string (message), include_backtrace)
{ }


inline
Exception::Exception (std::string_view const message, std::optional<bool> include_backtrace):
	Exception (std::string (message), include_backtrace)
{ }


inline
Exception::Exception (std::string const& message, std::optional<bool> include_backtrace):
	_what (message),
	_message (message)
{
	if (include_backtrace.value_or (enable_backtraces.load()))
		_backtrace = neutrino::backtrace();
}


inline
Exception::Exception (QString const& message, std::optional<bool> include_backtrace):
	Exception (message.toStdString(), include_backtrace)
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


inline std::optional<Backtrace> const&
Exception::backtrace() const
{
	return _backtrace;
}


namespace exception_ops {

std::ostream&
operator<< (std::ostream&, Exception const&);

std::ostream&
operator<< (std::ostream&, std::exception_ptr const&);

} // namespace exception_ops
} // namespace neutrino

#endif
