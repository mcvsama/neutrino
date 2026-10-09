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
#include "../si.h"

// Neutrino:
#include <neutrino/format.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>
#include <neutrino/string.h>

// Standard:
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <limits>


namespace neutrino::test {
namespace {

using namespace test_asserts;
using namespace si::units;
using namespace si::literals;


static consteval void
ensure_consteval_constructible()
{
	[[maybe_unused]] Dimensionless d;
	[[maybe_unused]] si::Quantity<Dimensionless> q;
}


template<si::ValueConcept Value>
	static consteval bool
	ensure_free_quantity_operations()
	{
		constexpr si::Quantity<Kilometer, Value> distance { Value (1.5) };
		constexpr auto meters = si::to<Meter> (distance);
		constexpr auto base = si::to_base_unit (distance);

		static_assert (std::same_as<decltype (distance.value()), Value>);
		static_assert (std::same_as<decltype (distance.template value<Meter>()), Value>);
		static_assert (distance.value() == Value (1.5));
		static_assert (distance.template value<Meter>() == Value (1500));
		static_assert (noexcept (distance.template value<Meter>()));
		static_assert (std::same_as<decltype (si::value (distance)), Value>);
		static_assert (std::same_as<decltype (si::value<Meter> (distance)), Value>);
		static_assert (si::value<Meter> (distance) == Value (1500));
		static_assert (noexcept (si::value<Meter> (distance)));
		static_assert (std::same_as<decltype (meters), const si::Quantity<Meter, Value>>);
		static_assert (std::same_as<decltype (base), const si::Quantity<Meter, Value>>);
		static_assert (si::value (distance) == Value (1.5));
		static_assert (si::value (meters) == Value (1500));
		static_assert (si::value (base) == Value (1500));
		static_assert (si::value<Kilometer> (meters) == Value (1.5));
		static_assert (noexcept (si::value (distance)));
		static_assert (noexcept (si::to<Meter> (distance)));
		static_assert (noexcept (si::to_base_unit (distance)));

		static_assert (std::same_as<decltype (si::value (Value (1.5))), Value>);
		static_assert (std::same_as<decltype (si::value<Meter> (Value (1.5))), Value>);
		static_assert (si::value<Meter> (Value (1.5)) == Value (1.5));
		static_assert (noexcept (si::value<Meter> (Value (1.5))));
		static_assert (si::value (si::to_base_unit (Value (1.5))) == Value (1.5));

		return true;
	}


template<class Q, class U>
	concept CanConvertTo = requires (Q const q) { si::to<U> (q); };


template<class Q, class U>
	concept CanGetValueIn = requires (Q const q) { q.template value<U>(); };


template<class Q, class U>
	concept CanGetFreeValueIn = requires (Q const q) { si::value<U> (q); };


static_assert (ensure_free_quantity_operations<float>());
static_assert (ensure_free_quantity_operations<double>());
static_assert (ensure_free_quantity_operations<long double>());
static_assert (std::same_as<decltype (si::value<Meter> (42)), int>);
static_assert (si::value<Meter> (42) == 42);
static_assert (si::value<Millisecond> (1.5_s) == 1500.0);
static_assert (si::value (si::to_base_unit (42)) == 42);
static_assert (CanConvertTo<si::Quantity<Meter>, Kilometer>);
static_assert (!CanConvertTo<si::Quantity<Meter>, Second>);
static_assert (!CanConvertTo<si::Quantity<Hertz>, RadianPerSecond>);
static_assert (CanGetValueIn<si::Quantity<Second, float>, Millisecond>);
static_assert (!CanGetValueIn<si::Quantity<Meter>, Second>);
static_assert (!CanGetValueIn<si::Quantity<Hertz>, RadianPerSecond>);
static_assert (CanGetFreeValueIn<si::Quantity<Second, float>, Millisecond>);
static_assert (!CanGetFreeValueIn<si::Quantity<Meter>, Second>);
static_assert (!CanGetFreeValueIn<si::Quantity<Hertz>, RadianPerSecond>);


TestRegistration tests ([]{
	describe ("Neutrino", "SI", []{
		it ("expression validity assertions", []{
			si::Quantity<Meter> one_meter;

			verify_compilation (5.0 * Meter());
			verify_compilation (Meter() * 5.0);

			verify_compilation (5.0 * one_meter);
			verify_compilation (one_meter * 5.0);

			si::Quantity<Meter> length;

			verify_compilation (length += one_meter);

			verify_compilation (std::numeric_limits<si::quantities::Current>::max());
			verify_compilation (si::quantities::Mass (1.0) * si::quantities::Mass (2.0));

			verify_compilation (one_meter.value<Foot>());
		});

		it ("dynamic numerical values", []{
			si::Quantity<Celsius, float> const temperature { 20.0f };
			si::Quantity<MeterPerSecond, float> const speed { 15.0f };
			auto const kelvin = Kelvin::dynamic_unit();
			auto const meter = Meter::dynamic_unit();
			auto const kilometer = Kilometer::dynamic_unit();

			static_assert (std::same_as<decltype (si::value (temperature, kelvin)), float>);
			static_assert (std::same_as<decltype (si::value (speed, "fps")), float>);
			static_assert (std::same_as<decltype (si::convert_value (kilometer, 1.5f, meter)), float>);
			verify_equal_with_epsilon ("dynamic temperature conversion includes the offset", si::value (temperature, kelvin), 293.15f, 1e-4f);
			verify_equal_with_epsilon ("string unit conversion includes the offset", si::value (temperature, "K"), 293.15f, 1e-4f);
			verify_equal_with_epsilon ("string unit conversion scales the value", si::value (speed, "fps"), 49.2125f, 1e-4f);
			verify_equal ("convert_value scales a numerical value", si::convert_value (kilometer, 1.5f, meter), 1500.0f);
			verify_equal ("scalar values ignore dynamic units", si::value (42, kelvin), 42);
			verify_equal ("scalar values ignore unit strings without parsing them", si::value (42, "unknown-unit"), 42);

			verify_throws<si::IncompatibleTypes> ("dynamic values reject incompatible dimensions", [temperature] {
				static_cast<void> (si::value (temperature, Meter::dynamic_unit()));
			});
			verify_throws<si::IncompatibleTypes> ("string unit values reject incompatible dimensions", [temperature] {
				static_cast<void> (si::value (temperature, "m"));
			});
			verify_throws<si::UnsupportedUnit> ("quantity values reject unknown unit strings", [temperature] {
				static_cast<void> (si::value (temperature, "unknownunit"));
			});
			verify_throws<si::IncompatibleTypes> ("convert_value rejects incompatible dimensions", [temperature] {
				static_cast<void> (si::convert_value (Celsius::dynamic_unit(), temperature.value(), Meter::dynamic_unit()));
			});
		});

		it ("explicit unit conversions", []{
			si::Quantity<Kilometer, float> const distance { 1.5f };
			si::Quantity<Hertz, float> const frequency { 1.0f };
			auto const meters = si::to_with_angle_exponent_change<Meter> (distance);
			si::Quantity<Meter, float> const converted = si::convert (distance);
			auto const angular_velocity = si::to_with_angle_exponent_change<RadianPerSecond> (frequency);

			static_assert (std::same_as<decltype (meters), const si::Quantity<Meter, float>>);
			static_assert (std::same_as<decltype (angular_velocity), const si::Quantity<RadianPerSecond, float>>);
			verify_equal ("to_with_angle_exponent_change converts between differently scaled units", meters.value(), 1500.0f);
			verify_equal ("convert returns a converter for ordinary unit conversions", converted.value(), 1500.0f);
			verify_equal_with_epsilon ("to_with_angle_exponent_change applies the cycles-to-radians factor", angular_velocity.value(), static_cast<float> (2.0 * M_PI), 1e-6f);
			verify_equal_with_epsilon ("to_with_angle_exponent_change reverses the radians-to-cycles conversion", si::to_with_angle_exponent_change<Hertz> (angular_velocity).value(), 1.0f, 1e-6f);
		});

		it ("comparison operators", []{
			si::Quantity<Meter> m1 { 10.0 };
			si::Quantity<Meter> m2 { 10.1 };

			verify ("10.0 m < 10.1 m", m1 < m2);
			verify ("10.0 m <= 10.1 m", m1 <= m2);
			verify ("10.0 m <= 10.0 m", m1 <= m1);

			verify ("10.1 m > 10.0 m", m2 > m1);
			verify ("10.1 m >= 10.0 m", m2 >= m1);
			verify ("10.0 m >= 10.0 m", m1 >= m1);

			m1 += 1.0 * Meter();
			m2 -= 0.1 * Meter();

			verify_equal_with_epsilon ("10.0 m + 1 m = 11 m", m1, 11.0 * Meter(), 1e-8 * Meter());
			verify_equal_with_epsilon ("10.1 m - 0.1 m = 10 m", m2, 10.0 * Meter(), 1e-8 * Meter());
			verify ("10.0 m == 10.0 m", m1 == m1);
			verify ("10.0 m != 10.1 m", m1 != m2);
		});

		it ("basic arithmetic operations", []{
			// Adding quantities for differently scaled units:
			auto len1 = 10.0 * Meter() + 1.0 * Foot();
			auto len2 = 10.0 * Meter() - 1.0 * Foot();
			auto area1 = 10.0 * Meter() * (1.0 * Foot());
			auto area2 = 10.0 * Meter() / (1.0 * Foot());
			auto volume = 1.0 * Kilometer() * (1.0 * Kilometer()) * (1.0 * Meter());

			verify_equal_with_epsilon ("10 m + 1 ft", len1, 10.304800609 * Meter(), 1e-9 * Meter());
			verify_equal_with_epsilon ("10 m - 1 ft", len2, 9.695199391 * Meter(), 1e-9 * Meter());
			verify_equal_with_epsilon ("10 m * 1 ft", area1, 3.048006096012 * SquareMeter(), 1e-9 * SquareMeter());
			verify_equal_with_epsilon ("10 m / 1 ft", area2, 32.808333333333, 1e-9);
			verify_equal_with_epsilon ("1 km * 1 km * 1 m", volume, 1'000'000.0 * CubicMeter(), 1e-9 * CubicMeter());
		});

		it ("lerp", []{
			verify_equal_with_epsilon ("si::lerp() interpolates floating-point values", si::lerp (10.0, 20.0, 0.25), 12.5, 1e-9);
			verify_equal_with_epsilon ("si::lerp() interpolates SI quantities preserving unit", si::lerp (10.0 * Kilometer(), 20.0 * Kilometer(), 0.25), 12.5 * Kilometer(), 1e-9 * Kilometer());
			verify_equal_with_epsilon ("si::lerp() interpolates power quantities", si::lerp (10.0 * Watt(), 20.0 * Watt(), 0.25), 12.5 * Watt(), 1e-9 * Watt());
		});

		it ("multiplication, division and counting quantities", []{
			using SquareMeter = decltype (1_m * 1_m)::Unit;
			using SquareKilometer = decltype (1_km * 1_km)::Unit;
			using InvMeter = decltype (1 / 1_m)::Unit;
			using InvKilometer = decltype (1 / 1_km)::Unit;

			verify_equal_with_epsilon ("1 m * 1 km = 1000.0 [value<SquareMeter>()]", (1_m * 1_km).value<SquareMeter>(), 1000.0, 1e-9);
			verify_equal_with_epsilon ("1 m * 1 km = 1000 m² [static_cast<> to SquareMeter]", static_cast<si::Quantity<SquareMeter>> (1_m * 1_km), 1000_m2, 1e-9_m2);
			verify_equal_with_epsilon ("1 m * 1 km = 1.0 [value()]", (1_m * 1_km).value(), 1.0, 1e-9);
			verify_equal_with_epsilon ("1 m * 1 km = 1000.0 [to_base_unit().value()]", (1_m * 1_km).to_base_unit().value(), 1000.0, 1e-9);
			verify_equal_with_epsilon ("1 m * 1 km = 0.001 [value<SquareKilometer>()]", (1_m * 1_km).value<SquareKilometer>(), 0.001, 1e-9);
			verify_equal_with_epsilon ("1 m * 1 km = 0.001 [static_cast<> to SquareKilometer]", static_cast<si::Quantity<SquareKilometer>> (1_m * 1_km).value(), 0.001, 1e-9);
			verify_equal_with_epsilon ("2 km * 1 km / 5 = 400'000 [value<T>()]", (2_km * 1_km / 5_m).value<si::Meter>(), 400'000.0, 1e-9);
			verify_equal_with_epsilon ("2 km * 1 km / 5 m = 400'000 m", 2_km * 1_km / 5_m, 400'000_m, 1e-9_m);
			verify_equal_with_epsilon ("5 / 1_m == 5000 / km [value<InvKilometer>()]", (5 / 1_m).value<InvKilometer>(), 5000.0, 1e-9);
			verify_equal_with_epsilon ("5 / 1_m == 5000 / km [static_cast<> to InvKilometer]", static_cast<si::Quantity<InvKilometer>> (5 / 1_m), 5000.0 * InvKilometer(), 1e-9 * InvMeter());
			verify_equal_with_epsilon ("1_m / 5_m == 0.2", 1_m / 5_m, 0.2, 1e-9);
			verify_equal_with_epsilon ("1_km / 5_m == 200.0", 1_km / 5_m, 200.0, 1e-9);
			verify_equal_with_epsilon ("1 / 1_in * 5_in == 5.0", 1 / 1_in * 5_in, 5.0, 1e-9);
		});

		it ("Temperature", []{
			si::Quantity<Kelvin> t1 { 273.15 - 40.0 };
			si::Quantity<Celsius> t2 { -40.0 };
			si::Quantity<Fahrenheit> t3 { -40.0 };

			using si::quantities::Temperature;

			verify_equal_with_epsilon ("temperature quantities are equal (t1, t2)", Temperature (t1), Temperature (t2), 1e-9 * Kelvin());
			verify_equal_with_epsilon ("temperature quantities are equal (t2, t3)", Temperature (t2), Temperature (t3), 1e-9 * Kelvin());
			verify_equal_with_epsilon ("temperature quantities are equal (t3, t1)", Temperature (t3), Temperature (t1), 1e-9 * Kelvin());
		});

		it ("Angle", []{
			si::Quantity<Radian> a1 { 1.0 };
			si::Quantity<Degree> a2 { 57.295'779'513 };
			si::Quantity<RadianPerSecond> s1 { 1.0 };
			si::Quantity<Hertz> s2 { 1.0 / (2.0 * M_PI) };
			si::Quantity<RadianPerSecond> s3 { s2.value<Hertz>() * (2.0 * M_PI) };
			si::Quantity<RadianPerSecond> s4 = si::convert (1.0_Hz / (2.0 * M_PI));

			verify_equal_with_epsilon ("radians/degrees equality test", a1, a2, 1e-9 * Radian());
			verify_equal_with_epsilon ("radians per second/hertz conversion test 1", s1, s3, 1e-9 * RadianPerSecond());
			verify_equal_with_epsilon ("radians per second/hertz conversion test 2", s3, s4, 1e-9 * RadianPerSecond());
		});

		it ("Velocity", []{
			si::Quantity<MeterPerSecond> s1 { 1.0 };
			si::Quantity<FootPerMinute> s2 { 196.85 };
			si::Quantity<FootPerSecond> s3 { 3.280833333333333 };
			si::Quantity<Knot> s4 { 1.9438444924406046432 };
			si::Quantity<KilometerPerHour> s5 { 3.6 };

			verify_equal_with_epsilon ("velocity quantities are equal (s1, s2)", s1, s2, 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("velocity quantities are equal (s2, s3)", s2, s3, 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("velocity quantities are equal (s3, s4)", s3, s4, 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("velocity quantities are equal (s4, s5)", s4, s5, 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("velocity quantities are equal (s5, s1)", s5, s1, 1e-9 * MeterPerSecond());
		});

		it ("parsing", []{
			si::DynamicUnit unit;

			unit = si::parse_unit (" m^2 ");
			verify ("parsed unit is SquareMeter", unit == SquareMeter::dynamic_unit());

			unit = si::parse_unit ("m^2 kg s^-2 A^0 K^-1 mol^0 cd^0 rad^0");
			verify ("parsed unit is JoulePerKelvin", unit == JoulePerKelvin::dynamic_unit());

			verify ("m^2 km is parsed correctly", si::parse_unit ("m^2   km") == si::DynamicUnit (3, 0, 0, 0, 0, 0, 0, 0, si::DynamicRatio (1000, 1)));
			verify ("m km^2 is parsed correctly", si::parse_unit ("m  km^2  ") == si::DynamicUnit (3, 0, 0, 0, 0, 0, 0, 0, si::DynamicRatio (1000000, 1)));
			verify ("m^-1 km^2 is parsed correctly", si::parse_unit (" m^-1 km^2  ") == si::DynamicUnit (1, 0, 0, 0, 0, 0, 0, 0, si::DynamicRatio (1000000, 1)));

			using si::quantities::Velocity;

			// No exceptions expected:
			{
				si::Quantity<FootPerMinute> v1;
				parse_to ("1 fpm", v1);
			}

			// No exceptions expected - quantity is convertible and should be converted:
			{
				si::Quantity<MeterPerSecond> v2;
				parse_to ("1 fpm", v2);
			}

			// Test type incompatibility:
			try {
				Velocity v;
				parse_to ("1 kg", v);
				verify ("exception IncompatibleTypes is thrown on incompatible types", false);
			}
			catch (si::IncompatibleTypes&)
			{ }

			// Make sure that the parse_to() function can convert values and units,
			// if they're convertible (same SI exponents vector):
			si::Quantity<MeterPerSecond> v1;
			si::Quantity<FootPerSecond> v2;
			parse_to ("15 m s^-1", v1);
			parse_to ("15 m s^-1", v2);
			verify_equal_with_epsilon ("v1 is 15 m/s", v1, 15.0 * MeterPerSecond(), 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("v1 == v2", v1, v2, 1e-9 * MeterPerSecond());
			parse_to ("49.2125 ft s^-1", v1);
			parse_to ("49.2125 ft s^-1", v2);
			verify_equal_with_epsilon ("v1 is 15 m/s", v1, 15.0 * MeterPerSecond(), 1e-9 * MeterPerSecond());
			verify_equal_with_epsilon ("v1 == v2", v1, v2, 1e-9 * MeterPerSecond());

			// Test non-base symbols:
			parse_to ("100 fpm", v1);
			verify_equal_with_epsilon ("v1 is 100 fpm", v1, 100.0 * FootPerMinute(), 1e-9 * MeterPerSecond());

			// Test generic conversion:
			verify_equal_with_epsilon ("15 m/s converts correctly to 'fps'", si::value (15.0 * MeterPerSecond(), "fps"), 49.2125, 1e-9);

			// Test division character:
			verify ("m/s == m / s", si::parse_unit ("m/s") == si::parse_unit ("m / s"));
			verify ("m s^-1 == m/s", si::parse_unit ("m s^-1") == si::parse_unit ("m/s"));
			verify ("m s^-2 kg^-3 == m / s^2 / kg^3", si::parse_unit ("m s^-2 kg^-3") == si::parse_unit ("m / s^2 / kg^3"));
			verify ("/s == s^-1", si::parse_unit (" / s") == si::parse_unit ("s^-1"));
			verify ("m / s kg == m s^-1 kg", si::parse_unit ("m / s kg") == si::parse_unit ("m s^-1 kg"));
		});

		it ("dBm formatting", []{
			verify_equal ("1 mW converts to 0 dBm", si::dBm_value (1_mW), 0.0);
			verify_equal ("1 W converts to 30 dBm", si::dBm_value (1_W), 30.0);
			verify ("0 W converts to -inf dBm", std::isinf (si::dBm_value (0_W)) && si::dBm_value (0_W) < 0.0);
			verify ("negative power converts to nan dBm", std::isnan (si::dBm_value (-1_mW)));

			verify_equal ("1 mW formats as 0 dBm", si::format_dBm (1_mW), std::string ("0 dBm"));
			verify_equal ("1 W formats as 30 dBm", si::format_dBm (1_W), std::string ("30 dBm"));
			verify_equal ("0 W formats as -inf dBm", si::format_dBm (0_W), std::string ("-inf dBm"));
			verify_equal ("negative dBm stays compact in format_unit()", format_unit (-121.0, 3, "dBm"), std::string ("-121 dBm"));
			verify_equal ("small current can be clamped to zero at a requested display threshold", format_unit (0.4_uA, 3, 1_uA), std::string ("0 µA"));
		});
	});
});

} // namespace
} // namespace neutrino::test
