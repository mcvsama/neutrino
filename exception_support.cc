/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
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
#include "exception_support.h"
#include "exception.h"

// Boost:
#include <boost/format.hpp>

// Standard:
#include <any>
#include <cstddef>
#include <exception>
#include <format>
#include <functional>
#include <future>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>


namespace neutrino {

[[nodiscard]]
std::string
describe_exception (std::exception_ptr const eptr)
{
	if (!eptr)
		return "<no exception>";
	else
	{
		auto const get_nested_messages = [](auto const& exception) {
			try {
				std::rethrow_if_nested (exception);
				return std::string();
			}
			catch (...)
			{
				return describe_exception (std::current_exception());
			}
		};

		auto const compile_message = [&get_nested_messages] (std::string_view const prepended_message, auto const& exception) {
			std::string result;

			if (!prepended_message.empty())
				result = std::format ("{}: ", prepended_message);

			result += exception.what();

			if (auto const nested_messages = get_nested_messages (exception);
				!nested_messages.empty())
			{
				result += std::format (": {}", nested_messages);
			}

			return result;
		};

		try {
			std::rethrow_exception (eptr);
		}
		catch (Exception const& e)
		{
			return compile_message ("", e);
		}
		// boost::format exceptions:
		catch (boost::io::too_few_args& e)
		{
			return compile_message ("format: too few args", e);
		}
		catch (boost::io::too_many_args& e)
		{
			return compile_message ("format: too many args", e);
		}
		catch (boost::io::bad_format_string& e)
		{
			return compile_message ("format: ill formed", e);
		}
		// std::logic_error derivatives:
		catch (std::invalid_argument& e)
		{
			return compile_message ("std: invalid argument: ", e);
		}
		catch (std::domain_error& e)
		{
			return compile_message ("std: domain error: ", e);
		}
		catch (std::length_error& e)
		{
			return compile_message ("std: length error: ", e);
		}
		catch (std::out_of_range& e)
		{
			return compile_message ("std: out of range: ", e);
		}
		catch (std::future_error& e)
		{
			return compile_message ("std: future error: ", e);
		}
		catch (std::logic_error& e)
		{
			return compile_message ("std: logic error: ", e);
		}
		// std::runtime_error derivatives:
		catch (std::range_error& e)
		{
			return compile_message ("std: range error: ", e);
		}
		catch (std::overflow_error& e)
		{
			return compile_message ("std: overflow error: ", e);
		}
		catch (std::underflow_error& e)
		{
			return compile_message ("std: underflow error: ", e);
		}
		catch (std::regex_error& e)
		{
			return compile_message ("std: regex error: ", e);
		}
		catch (std::ios_base::failure& e)
		{
			return compile_message ("std: ios_base failure: ", e);
		}
		catch (std::system_error& e)
		{
			return compile_message ("std: system error: ", e);
		}
		catch (std::runtime_error& e)
		{
			return compile_message ("std: runtime error: ", e);
		}
		// Generic:
		catch (std::bad_optional_access& e)
		{
			return compile_message ("std: bad optional access: ", e);
		}
		catch (std::bad_typeid& e)
		{
			return compile_message ("std: bad typeid(): ", e);
		}
		catch (std::bad_any_cast& e)
		{
			return compile_message ("std: bad any_cast(): ", e);
		}
		catch (std::bad_cast& e)
		{
			return compile_message ("std: bad cast: ", e);
		}
		catch (std::bad_weak_ptr& e)
		{
			return compile_message ("std: bad weak_ptr<>: ", e);
		}
		catch (std::bad_function_call& e)
		{
			return compile_message ("std: bad function call: ", e);
		}
		catch (std::bad_array_new_length& e)
		{
			return compile_message ("std: bad array new length: ", e);
		}
		catch (std::bad_alloc& e)
		{
			return compile_message ("std: bad alloc: ", e);
		}
		catch (std::bad_exception& e)
		{
			return compile_message ("std: bad exception: ", e);
		}
		catch (std::bad_variant_access& e)
		{
			return compile_message ("std: bad variant access: ", e);
		}
		// Most generic:
		catch (std::exception& e)
		{
			return compile_message ("standard exception: ", e);
		}
		catch (...)
		{
			return "<unknown exception>";
		}
	}
}

} // namespace neutrino
