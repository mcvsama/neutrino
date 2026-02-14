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

// Local:
#include "test_values.h"

// Neutrino:
#include <neutrino/test/auto_test.h>
#include <neutrino/string.h>

// Lib:
#include <boost/algorithm/string.hpp>

// Standard:
#include <fstream>
#include <iomanip>


namespace neutrino {

TestValuesCompareError::TestValuesCompareError (std::string_view const& message):
	message (message)
{ }


TestValuesCompareError::TestValuesCompareError (std::string_view const& message, std::size_t const line):
	message (std::format ("{}; line={}", message, line)),
	line (line)
{ }


TestValuesCompareError::TestValuesCompareError (std::string_view const& message, std::size_t const line, std::size_t const word, double expected_value, double actual_value):
	message (std::format ("{}; line:word={}:{}; expected={}, actual={}", message, line, word, expected_value, actual_value)),
	line (line),
	word (word),
	expected_value (expected_value),
	actual_value (actual_value)
{ }


TestValues
read_test_values_file (std::filesystem::path const file_name, char const delimiter, std::optional<std::size_t> guaranteed_size)
{
	TestValues result;
	std::ifstream file (file_name);

	while (file.good())
	{
		std::string input_line;
		std::getline (file, input_line);

		if (!input_line.empty())
		{
			if (guaranteed_size)
				result.lines.back().resize (*guaranteed_size, 0.0);

			std::vector<std::string> str_words;
			str_words.reserve (8);
			boost::split (str_words, input_line, [&] (char c) { return c == delimiter; });

			result.lines.push_back ({});
			auto& output_line = result.lines.back();

			for (auto const& w: str_words)
				if (!w.empty())
					output_line.push_back (parse<double> (w));
		}
	}

	return result;
}


void
write_test_values_file (std::filesystem::path const file_name, char const delimiter, TestValues const& test_values)
{
	std::filesystem::create_directories (file_name.parent_path());
	std::ofstream file (file_name);

	if (!file.good())
		throw Exception ("could not open '" + std::string (file_name) + "' for writing");

	for (auto& words: test_values.lines)
	{
		auto const n = words.size();

		for (auto i = 0u; i < n; ++i)
		{
			if (!file.good())
				throw Exception ("failed to write all data to '" + std::string (file_name) + "'");

			file << std::setprecision (18) << words[i];

			if (i < n - 1)
				file << delimiter;
		}

		file << '\n';
	}
}


std::optional<TestValuesCompareError>
compare_test_values (TestValues const& expected, TestValues const& actual, double epsilon)
{
	auto const& expected_lines = expected.lines;
	auto const& actual_lines = actual.lines;

	if (expected_lines.size() != actual_lines.size())
		return TestValuesCompareError { "different data set sizes: expected " + std::to_string (expected_lines.size()) + " lines, actual " + std::to_string (actual_lines.size()) + " lines" };

	for (std::size_t il = 0u; il < expected_lines.size(); ++il)
	{
		auto const& expected_line = expected_lines[il];
		auto const& actual_line = actual_lines[il];

		if (expected_line.size() != actual_line.size())
			return TestValuesCompareError ("wrong number of words in a line; expected " + std::to_string (expected_line.size()) + " words, actual " + std::to_string (actual_line.size()) + " words", il);

		for (std::size_t iw = 0u; iw < expected_line.size(); ++iw)
		{
			auto e = expected_line[iw];
			auto a = actual_line[iw];

			using std::isfinite;

			if (!isfinite (abs (e - a)) || abs (e - a) > epsilon)
				return TestValuesCompareError (std::format ("wrong value (difference={} > epsilon)", e - a), il, iw, e, a);
		}
	}

	return {};
}


void
write_or_compare (TestValues const& test_values, std::filesystem::path const& data_file_path, double epsilon, bool write)
{
	if (write)
		write_test_values_file (data_file_path, ' ', test_values);
	else
	{
		auto const expected_test_values = read_test_values_file (data_file_path, ' ');

		if (auto error = compare_test_values (expected_test_values, test_values, epsilon))
			test_asserts::verify (error->message, false);
	}
}

} // namespace neutrino
