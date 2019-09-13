/* vim:ts=4
 *
 * Copyleft 2019  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__TEST__TEST_VALUES_H__INCLUDED
#define NEUTRINO__TEST__TEST_VALUES_H__INCLUDED

// Standard:
#include <cstddef>
#include <filesystem>
#include <optional>


namespace neutrino {

/**
 * Represents table of double values, multiple lines, multiple values in each line.
 */
class TestValues
{
  public:
	using Words	= std::vector<double>;
	using Lines	= std::vector<Words>;

  public:
	Lines lines;

  public:
	template<class ...Args>
		void
		add_line (Args&& ...args);

  private:
	template<class Value, class ...Args>
		void
		add_words_to (Words& words, Value const& value, Args&& ...args);
};


class TestValuesCompareError
{
  public:
	std::string	message;
	std::size_t	line			{ 0u };
	std::size_t	word			{ 0u };
	double		expected_value	{ 0.0 };
	double		actual_value	{ 0.0 };

  public:
	// Ctor
	explicit
	TestValuesCompareError (std::string_view const& message);

	// Ctor
	explicit
	TestValuesCompareError (std::string_view const& message, std::size_t const line);

	// Ctor
	explicit
	TestValuesCompareError (std::string_view const& message, std::size_t const line, std::size_t const word, double expected_value, double actual_value);
};


template<class ...Args>
	inline void
	TestValues::add_line (Args&& ...args)
	{
		lines.push_back ({});
		lines.back().reserve (8);
		add_words_to (lines.back(), std::forward<Args> (args)...);
	}


template<class Value, class ...Args>
	inline void
	TestValues::add_words_to (Words& words, Value const& value, Args&& ...args)
	{
		words.push_back (value / Value (1));

		if constexpr (sizeof...(args) > 0)
			add_words_to (words, std::forward<Args> (args)...);
	}


/*
 * Global functions
 */


/**
 * Read delimiter-separated values from the file.
 *
 * \param	guaranteed_size
 *			If provided, functions will make sure that resulting vector of values in each line will have at least guaranteed_size elements.
 */
[[nodiscard]]
TestValues
read_test_values_file (std::filesystem::path const file_name, char delimiter, std::optional<std::size_t> guaranteed_size = {});

/**
 * Write given TestValues to a file.
 */
void
write_test_values_file (std::filesystem::path const file_name, char delimiter, TestValues const& test_values);

/**
 * Compare two sets of TestValues with "epsilon" accuracy.
 *
 * \param	epsilon
 *			Maximum allowed difference between each corresponding value in compared data sets.
 */
[[nodiscard]]
std::optional<TestValuesCompareError>
compare_test_values (TestValues const& expected, TestValues const& actual, double epsilon);

/**
 * Writes test values to the given file or compares it with the file, depending on the "write" argument.
 *
 * \param	epsilon
 *			Maximum allowed difference between each corresponding value in compared data sets.
 */
void
write_or_compare (TestValues const& test_values, std::filesystem::path const& data_file_path, double epsilon, bool write);

} // namespace neutrino

#endif

