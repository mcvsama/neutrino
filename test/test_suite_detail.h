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

#ifndef NEUTRINO__TEST__TEST_SUITE_DETAIL_H__INCLUDED
#define NEUTRINO__TEST__TEST_SUITE_DETAIL_H__INCLUDED


namespace neutrino::test_suite_detail {

template<class Callback>
	concept DescribeLeafCallback =
		std::constructible_from<TestSuite::DescribeCallback1, std::remove_cvref_t<Callback>> ||
		std::constructible_from<TestSuite::DescribeCallback2, std::remove_cvref_t<Callback>>;


template<class Tuple, class Indices>
	struct DescribePathArguments;


template<class Tuple, std::size_t ...Indices>
	struct DescribePathArguments<Tuple, std::index_sequence<Indices...>>:
		public std::bool_constant<(std::convertible_to<std::tuple_element_t<Indices, Tuple>, std::string_view> && ...)>
	{ };


template<class ...Args>
	concept MultiDescribeArguments =
		sizeof...(Args) >= 3 &&
		DescribeLeafCallback<std::tuple_element_t<sizeof...(Args) - 1, std::tuple<std::remove_cvref_t<Args>...>>> &&
		DescribePathArguments<std::tuple<std::remove_cvref_t<Args>...>, std::make_index_sequence<sizeof...(Args) - 1>>::value;


template<class Tuple, std::size_t ...Indices>
	[[nodiscard]]
	inline auto
	make_describe_path (Tuple const& arguments, std::index_sequence<Indices...>)
	{
		return std::array<std::string, sizeof...(Indices)> {
			std::string (std::string_view (std::get<Indices> (arguments)))...
		};
	}


template<std::size_t n, class Callback>
	inline void
	describe_with_path (std::array<std::string, n> path, std::size_t const index, Callback callback)
	{
		auto const tested_thing = path[index];

		if (index + 1 == n)
			describe (tested_thing, std::move (callback));
		else
		{
			describe (tested_thing, [path = std::move (path), index, callback = std::move (callback)]() mutable {
				describe_with_path (std::move (path), index + 1, std::move (callback));
			});
		}
	}

} // namespace neutrino::test_suite_detail

#endif
