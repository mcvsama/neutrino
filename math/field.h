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

#ifndef NEUTRINO__MATH__FIELD_H__INCLUDED
#define NEUTRINO__MATH__FIELD_H__INCLUDED

// Neutrino:
#include <neutrino/map.h>
#include <neutrino/numeric.h>
#include <neutrino/sequence_utils.h>
#include <neutrino/synchronized.h>
#include <neutrino/types.h>

// Standard:
#include <cstddef>
#include <initializer_list>
#include <set>
#include <tuple>
#include <vector>
#include <type_traits>


namespace neutrino::math {

/**
 * N-dimensional lookup table with interpolation/extrapolation support.
 *
 * Template parameters are argument types followed by the result value type.
 * Example: Field<X, Y, Z, V> describes a 3-dimensional field mapping
 * (X, Y, Z) -> V.
 */
template<class Argument0, class ...RemainingArgumentsAndValue>
	class Field
	{
		static_assert (sizeof...(RemainingArgumentsAndValue) >= 1, "Field<> needs at least two parameters, argument and value types");

		static constexpr bool kLinearExtrapolate = false;

	  public:
		static constexpr std::size_t kNumArguments	= sizeof...(RemainingArgumentsAndValue);

		using ArgumentsAndValue		= std::tuple<Argument0, RemainingArgumentsAndValue...>;
		using Arguments				= TupleSlice<0, std::tuple_size_v<ArgumentsAndValue> - 1, ArgumentsAndValue>;
		using Value					= std::remove_cvref_t<std::tuple_element_t<kNumArguments, ArgumentsAndValue>>;
		using DataMap				= MultiDimensionalMap<Arguments, Value>;

		/**
		 * A point in the field subspace. Arguments and value.
		 */
		template<class Tuple>
			struct SubspacePoint
			{
			  public:
				Tuple	arguments;
				Value	value;

			  public:
				explicit
				SubspacePoint (Tuple const& arguments, Value const& value):
					arguments (arguments),
					value (value)
				{ }
			};

		using Point = SubspacePoint<Arguments>;

		template<std::size_t N>
			using NthArgument =
				std::remove_cvref_t<std::tuple_element_t<N, Arguments>>;

		template<class ...Args>
			using ArgumentType =
				std::conditional_t<sizeof...(Args) == 0,
								   NthArgument<sizeof...(Args)>,
								   std::optional<NthArgument<sizeof...(Args)>>>;

		template<class ...Args>
			using ValueType =
				std::conditional_t<sizeof...(Args) == 0,
								   Value,
								   std::optional<Value>>;

		template<class ...Args>
			using PointType =
				std::conditional_t<sizeof...(Args) == 0,
								   Point,
								   std::optional<Point>>;

		template<class ...Args>
			using DomainType =
				std::conditional_t<sizeof...(Args) == 0,
								   Range<NthArgument<sizeof...(Args)>>,
								   std::optional<Range<NthArgument<sizeof...(Args)>>>>;

		template<class ...Args>
			using CodomainType =
				std::conditional_t<sizeof...(Args) == 0,
								   Range<Value>,
								   std::optional<Range<Value>>>;

		template<class Map, class ...Args>
			using MinMaxSubspacePoint =
				std::conditional_t<sizeof...(Args) == 0,
								   SubspacePoint<RecursiveMapKeysTuple<Map>>,
								   std::optional<SubspacePoint<RecursiveMapKeysTuple<Map>>>>;

		/**
		 * Thrown when coefficients map passed to the Field
		 * doesn't have any points defined.
		 */
		class EmptyDomainException: public Exception
		{
		  public:
			explicit
			EmptyDomainException():
				Exception ("field domain must not be empty")
			{ }
		};

	  public:
		/**
		 * Create a data-table from argument-value points given in the map.
		 */
		explicit
		Field (DataMap const&);

		/**
		 * Create a data-table from argument-value points given in the map.
		 */
		explicit
		Field (DataMap&&);

		/**
		 * Create a data-table from a list of key-value pairs.
		 */
		explicit
		Field (std::initializer_list<typename DataMap::value_type>&&);

		// Copy ctor
		Field (Field const&) = default;

		// Move ctor
		Field (Field&&) noexcept;

		// Copy operator
		Field&
		operator= (Field const&) = default;

		// Move operator
		Field&
		operator= (Field&&) noexcept;

		/**
		 * Return number of dimensions of this field.
		 */
		[[nodiscard]]
		static constexpr std::size_t
		dimensions() noexcept;

		/**
		 * Return value for given arguments.
		 * Result will be defined only if arguments are within bounds of data-table arguments.
		 * The result will be interpolated.
		 * At an exact grid knot, only that slice contributes; between knots, all contributing slices
		 * must contain the remaining arguments in their domains.
		 */
		template<class ...Args>
			[[nodiscard]]
			std::optional<Value>
			value (Args&&...) const;

		/**
		 * Return value for given arguments.
		 * If arguments are outside bounds of data-table arguments, result will be extrapolated;
		 * otherwise result will be the same as from calling value().
		 */
		template<class ...Args>
			[[nodiscard]]
			Value
			extrapolated_value (Args&&...) const;

		/**
		 * Alias for extrapolated_value().
		 */
		template<class ...Args>
			[[nodiscard]]
			Value
			operator() (Args&&...) const;

		/**
		 * Alias for extrapolated_value().
		 */
		template<class ...Args>
			[[nodiscard]]
			Value
			operator[] (Args&&...) const;

		/**
		 * Return the minimum of the next unfixed argument.
		 * Interpolate bounds between grid knots; at an exact knot, use only that slice.
		 * With fixed arguments, return std::nullopt if any contributing slice cannot provide a bound.
		 */
		template<class ...Args>
			[[nodiscard]]
			ArgumentType<Args...>
			min_argument (Args&&...) const;

		/**
		 * Return the maximum of the next unfixed argument.
		 * Same notes as for min_argument() apply.
		 */
		template<class ...Args>
			[[nodiscard]]
			ArgumentType<Args...>
			max_argument (Args&&...) const;

		/**
		 * Return the point of minimum value.
		 * For fixed arguments, search the interpolated field within the common domain of the contributing slices.
		 * Its extrema occur at breakpoints of the remaining grids.
		 *
		 * Fewer fixed arguments leave more dimensions to search.
		 * With no arguments, search the whole field and return Point. With fixed arguments, return std::optional<Point>;
		 * the result is empty when the requested slice has no common domain.
		 */
		template<class ...Args>
			[[nodiscard]]
			PointType<Args...>
			min_value_point (Args&&...) const;

		/**
		 * Return the point of maximum value.
		 *
		 * Same notes as for min_value_point() apply.
		 */
		template<class ...Args>
			[[nodiscard]]
			PointType<Args...>
			max_value_point (Args&&...) const;

		/**
		 * Return the minimum value for given subspace.
		 */
		template<class ...Args>
			[[nodiscard]]
			ValueType<Args...>
			min_value (Args&&...) const;

		/**
		 * Return the maximum value for given subspace.
		 */
		template<class ...Args>
			[[nodiscard]]
			ValueType<Args...>
			max_value (Args&&...) const;

		/**
		 * Return the range between min_argument(args...) and max_argument(args...).
		 * Fixed leading arguments select the subspace; undefined bounds give std::nullopt.
		 */
		template<class ...Args>
			[[nodiscard]]
			DomainType<Args...>
			domain (Args&&...) const;

		/**
		 * Return the range between min_value(args...) and max_value(args...).
		 * Fixed leading arguments select the subspace; undefined extrema give std::nullopt.
		 */
		template<class ...Args>
			[[nodiscard]]
			CodomainType<Args...>
			codomain (Args&&...) const;

		/**
		 * Return reference to underlying map of values used by this Field.
		 */
		[[nodiscard]]
		DataMap const&
		data_map() const;

	  private:
		/**
		 * Search all submaps and throw EmptyDomainException if any of the maps
		 * is empty.
		 */
		template<class Map>
			static void
			validate (Map const&);

		/**
		 * Helper for min_argument() and max_argument().
		 */
		template<class Map, class GetIter>
			[[nodiscard]]
			typename Map::key_type
			compute_minmax_argument (Map const&, GetIter&&) const;

		/**
		 * Helper for min_argument() and max_argument().
		 */
		template<class Map, class GetIter, class X, class ...Xs>
			[[nodiscard]]
			std::optional<std::tuple_element_t<sizeof...(Xs) + 1u, RecursiveMapKeysTuple<Map>>>
			compute_minmax_argument (Map const&, GetIter&&, X&&, Xs&&...) const;

		/**
		 * Helper for min_value_point() and max_value_point().
		 */
		template<class Map, class IsBetter>
			[[nodiscard]]
			MinMaxSubspacePoint<Map>
			compute_minmax_value (Map const&, IsBetter&&) const;

		/**
		 * Helper for min_value_point() and max_value_point().
		 */
		template<class Map, class IsBetter, class X, class ...Xs>
			[[nodiscard]]
			MinMaxSubspacePoint<Map, X, Xs...>
			compute_minmax_value (Map const&, IsBetter&&, X&&, Xs&&...) const;

		/**
		 * Search the union of grid breakpoints within the common domain of the contributing maps.
		 * Evaluate the original field at each candidate, keeping all free arguments at the same point
		 * across the interpolated slices. A multilinear function attains its extrema at cell vertices.
		 */
		template<class Map, class IsBetter, class Evaluate, class ...Xs>
			[[nodiscard]]
			std::optional<SubspacePoint<RecursiveMapKeysTuple<Map>>>
			search_minmax_value (std::vector<Map const*> const&, IsBetter const&, Evaluate const&, Xs const&...) const;

		/**
		 * Find neighboring knots, selecting a single slice at exact knots and constant-extrapolation edges.
		 * The first result still indicates whether the argument is within the map's domain.
		 */
		template<class Map, class X>
			[[nodiscard]]
			static std::tuple<bool, typename Map::const_iterator, typename Map::const_iterator>
			interpolation_points (Map const&, X const&);

		/**
		 * Return interpolated value for given map.
		 * Argument must be within data domain for that map.
		 */
		template<class Map, class X, class ...Xs>
			[[nodiscard]]
			std::optional<Value>
			compute_value (Map&&, bool extrapolate, X&&, Xs&&...) const;

	  private:
		DataMap _data_map;
	};


template<class A, class ...R>
	inline
	Field<A, R...>::Field (DataMap const& map):
		_data_map (map)
	{
		validate (_data_map);
	}


template<class A, class ...R>
	inline
	Field<A, R...>::Field (DataMap&& map):
		_data_map (std::move (map))
	{
		validate (_data_map);
	}


template<class A, class ...R>
	inline
	Field<A, R...>::Field (std::initializer_list<typename DataMap::value_type>&& pairs):
		_data_map (std::move (pairs))
	{
		validate (_data_map);
	}


template<class A, class ...R>
	inline
	Field<A, R...>::Field (Field&& other) noexcept:
		_data_map (std::move (other._data_map))
	{ }


template<class A, class ...R>
	inline Field<A, R...>&
	Field<A, R...>::operator= (Field&& other) noexcept
	{
		_data_map = std::move (other._data_map);
		return *this;
	}


template<class A, class ...R>
	constexpr std::size_t
	Field<A, R...>::dimensions() noexcept
	{
		return std::tuple_size_v<Arguments>;
	}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::value (Args&& ...args) const
			-> std::optional<Value>
		{
			static_assert (sizeof...(args) == kNumArguments, "not enough arguments");

			return compute_value (_data_map, false, std::forward<Args> (args)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::extrapolated_value (Args&& ...args) const
			-> Value
		{
			static_assert (sizeof...(args) == kNumArguments, "not enough arguments");

			return *compute_value (_data_map, true, std::forward<Args> (args)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::operator() (Args&& ...args) const
			-> Value
		{
			return extrapolated_value (std::forward<Args> (args)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::operator[] (Args&& ...args) const
			-> Value
		{
			return extrapolated_value (std::forward<Args> (args)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::min_argument (Args&& ...xs) const
			-> ArgumentType<Args...>
		{
			static_assert (sizeof...(Args) <= kNumArguments - 1, "number of arguments must be between 0 and dimensions()-1");

			auto const get_iter = [](auto&& map) { return map.begin(); };

			if constexpr (sizeof...(Args) == 0)
				return compute_minmax_argument (_data_map, get_iter);
			else
				return compute_minmax_argument (_data_map, get_iter, std::forward<Args> (xs)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::max_argument (Args&& ...xs) const
			-> ArgumentType<Args...>
		{
			static_assert (sizeof...(Args) <= kNumArguments - 1, "number of arguments must be between 0 and dimensions()-1");

			auto const get_iter = [](auto&& map) { return map.rbegin(); };

			if constexpr (sizeof...(Args) == 0)
				return compute_minmax_argument (_data_map, get_iter);
			else
				return compute_minmax_argument (_data_map, get_iter, std::forward<Args> (xs)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::min_value_point (Args&& ...xs) const
			-> PointType<Args...>
		{
			static_assert (sizeof...(Args) <= kNumArguments - 1, "number of arguments must be between 0 and dimensions()-1");

			auto const is_better = [](auto&& a, auto&& b) { return a < b; };

			return compute_minmax_value (_data_map, is_better, std::forward<Args> (xs)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::max_value_point (Args&& ...xs) const
			-> PointType<Args...>
		{
			static_assert (sizeof...(Args) <= kNumArguments - 1, "number of arguments must be between 0 and dimensions()-1");

			auto const is_better = [](auto&& a, auto&& b) { return a > b; };

			return compute_minmax_value (_data_map, is_better, std::forward<Args> (xs)...);
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::min_value (Args&& ...xs) const
			-> ValueType<Args...>
		{
			auto const point = min_value_point (xs...);

			if constexpr (sizeof...(Args) == 0)
				return point.value;
			else
			{
				if (point)
					return point->value;
				else
					return std::nullopt;
			}
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::max_value (Args&& ...xs) const
			-> ValueType<Args...>
		{
			auto const point = max_value_point (xs...);

			if constexpr (sizeof...(Args) == 0)
				return point.value;
			else
			{
				if (point)
					return point->value;
				else
					return std::nullopt;
			}
		}


template<class A, class ...R>
	template<class ...Args>
		inline auto
		Field<A, R...>::domain (Args&& ...xs) const
			-> DomainType<Args...>
		{
			if constexpr (sizeof...(Args) == 0)
				return { min_argument(), max_argument() };
			else
			{
				auto const min = min_argument (std::forward<Args> (xs)...);

				if (!min)
					return std::nullopt;

				auto const max = max_argument (std::forward<Args> (xs)...);

				if (!max)
					return std::nullopt;

				return Range { *min, *max };
			}
		}


template<class A, class ...R>
	template<class ...Args>
		[[nodiscard]]
		inline auto
		Field<A, R...>::codomain (Args&& ...args) const
			-> CodomainType<Args...>
		{
			if constexpr (sizeof...(Args) == 0)
				return { min_value(), max_value() };
			else
			{
				auto const min_y = min_value (std::forward<Args> (args)...);

				if (!min_y)
					return std::nullopt;

				auto const max_y = max_value (std::forward<Args> (args)...);

				if (!max_y)
					return std::nullopt;

				return Range { *min_y, *max_y };
			}
		}


template<class A, class ...R>
	inline auto
	Field<A, R...>::data_map() const
		-> DataMap const&
	{
		return _data_map;
	}


template<class A, class ...R>
	template<class Map>
		inline void
		Field<A, R...>::validate (Map const& map)
		{
			if (map.empty())
				throw EmptyDomainException();

			if constexpr (is_specialization_v<typename Map::mapped_type, std::map>)
			{
				for (auto const& pair: map)
					validate (pair.second);
			}
		}


template<class A, class ...R>
	template<class Map, class GetIter>
		inline auto
		Field<A, R...>::compute_minmax_argument (Map const& map, GetIter&& get_iter) const
			-> typename Map::key_type
		{
			return get_iter (map)->first;
		}


template<class A, class ...R>
	template<class Map, class GetIter, class X, class ...Xs>
		inline auto
		Field<A, R...>::compute_minmax_argument (Map const& map, GetIter&& get_iter, X&& x_unconverted, Xs&& ...xs) const
			-> std::optional<std::tuple_element_t<sizeof...(Xs) + 1u, RecursiveMapKeysTuple<Map>>>
		{
			using MapKey = typename Map::key_type;

			// "x" is a key in the "map" argument.

			auto const x = static_cast<MapKey> (x_unconverted);
			auto const [inside_domain, ia, ib] = interpolation_points (map, x);

			if (!inside_domain)
				return std::nullopt;

			if (ia == ib)
				return compute_minmax_argument (ia->second, get_iter, xs...);

			Range const xrange { ia->first, ib->first };

			if constexpr (sizeof...(Xs) == 0)
			{
				Range const yrange { compute_minmax_argument (ia->second, get_iter), compute_minmax_argument (ib->second, get_iter) };
				return renormalize (x, xrange, yrange);
			}
			else
			{
				auto const bound_a = compute_minmax_argument (ia->second, get_iter, xs...);

				if (!bound_a)
					return std::nullopt;

				auto const bound_b = compute_minmax_argument (ib->second, get_iter, xs...);

				if (!bound_b)
					return std::nullopt;

				Range const yrange { *bound_a, *bound_b };
				return renormalize (x, xrange, yrange);
			}
		}


template<class A, class ...R>
	template<class Map, class IsBetter>
		inline auto
		Field<A, R...>::compute_minmax_value (Map const& map, IsBetter&& is_better) const
			-> MinMaxSubspacePoint<Map>
		{
			// If map values are also maps, recurse:
			if constexpr (is_specialization_v<typename Map::mapped_type, std::map>)
			{
				auto best = compute_minmax_value (map.begin()->second, is_better);
				auto best_argument = map.begin()->first;

				for (auto it = std::next (map.begin()); it != map.end(); ++it)
				{
					auto const better = compute_minmax_value (it->second, is_better);

					if (is_better (better.value, best.value))
					{
						best = better;
						best_argument = it->first;
					}
				}

				return SubspacePoint (std::tuple_cat (std::make_tuple (best_argument), best.arguments), best.value);
			}
			// If map values are scalars:
			else
			{
				auto best = SubspacePoint (std::make_tuple (map.begin()->first), map.begin()->second);

				for (auto const& i: map)
					if (is_better (i.second, best.value))
						best = SubspacePoint (std::make_tuple (i.first), i.second);

				return best;
			}
		}


template<class A, class ...R>
	template<class Map, class IsBetter, class X, class ...Xs>
		inline auto
		Field<A, R...>::compute_minmax_value (Map const& map, IsBetter&& is_better, X&& x_unconverted, Xs&& ...xs) const
			-> MinMaxSubspacePoint<Map, X, Xs...>
		{
			auto const evaluate = [&] (auto const& ...args) {
				return compute_value (map, false, args...);
			};
			return search_minmax_value (std::vector<Map const*> { &map }, is_better, evaluate, x_unconverted, xs...);
		}


template<class A, class ...R>
	template<class Map, class IsBetter, class Evaluate, class ...Xs>
		inline auto
		Field<A, R...>::search_minmax_value (std::vector<Map const*> const& maps, IsBetter const& is_better, Evaluate const& evaluate, Xs const& ...xs) const
			-> std::optional<SubspacePoint<RecursiveMapKeysTuple<Map>>>
		{
			using MapKey = typename Map::key_type;
			using MapValue = typename Map::mapped_type;
			using ResultPoint = SubspacePoint<RecursiveMapKeysTuple<Map>>;

			std::optional<ResultPoint> best;

			auto const consider_key = [&] (MapKey const& key, auto const& ...remaining_args) {
				if constexpr (is_specialization_v<MapValue, std::map>)
				{
					std::vector<MapValue const*> submaps;

					for (auto const* map: maps)
					{
						auto const [inside_domain, ia, ib] = interpolation_points (*map, key);

						if (!inside_domain)
							return;

						submaps.push_back (&ia->second);

						if (ia != ib)
							submaps.push_back (&ib->second);
					}

					auto const evaluate_at_key = [&] (auto const& ...args) { return evaluate (key, args...); };
					auto const candidate = search_minmax_value (submaps, is_better, evaluate_at_key, remaining_args...);

					if (candidate && (!best || is_better (candidate->value, best->value)))
						best = ResultPoint (std::tuple_cat (std::make_tuple (key), candidate->arguments), candidate->value);
				}
				else
				{
					for (auto const* map: maps)
						if (key < map->begin()->first || map->rbegin()->first < key)
							return;

					auto const value = evaluate (key);

					if (value && (!best || is_better (*value, best->value)))
						best = ResultPoint (std::make_tuple (key), *value);
				}
			};

			if constexpr (sizeof...(Xs) > 0)
			{
				std::apply ([&] (auto const& x, auto const& ...rest) {
					consider_key (static_cast<MapKey> (x), rest...);
				}, std::forward_as_tuple (xs...));
			}
			else
			{
				std::set<MapKey> keys;

				for (auto const* map: maps)
					for (auto const& pair: *map)
						keys.insert (pair.first);

				for (auto const& key: keys)
					consider_key (key);
			}

			return best;
		}


template<class A, class ...R>
	template<class Map, class X>
		inline auto
		Field<A, R...>::interpolation_points (Map const& map, X const& x)
			-> std::tuple<bool, typename Map::const_iterator, typename Map::const_iterator>
		{
			auto const get_first = [](auto const& pair) { return pair.first; };
			auto [inside_domain, ia, ib] = adjacent_find_for_extrapolation (map.begin(), map.end(), x, get_first);

			if (ia != ib)
			{
				if (x == ia->first || (!kLinearExtrapolate && x < ia->first))
					ib = ia;
				else if (x == ib->first || (!kLinearExtrapolate && ib->first < x))
					ia = ib;
			}

			return { inside_domain, ia, ib };
		}


template<class A, class ...R>
	template<class Map, class X, class ...Xs>
		inline auto
		Field<A, R...>::compute_value (Map&& map, bool extrapolate, X&& x, Xs&& ...xs) const
			-> std::optional<Value>
		{
			auto const [inside_domain, ia, ib] = interpolation_points (map, x);

			if (!inside_domain && !extrapolate)
				return std::nullopt;

			auto const value_at = [&] (auto const& point) -> std::optional<Value> {
				if constexpr (sizeof...(xs) > 0)
					return compute_value (point.second, extrapolate, std::forward<Xs> (xs)...);
				else
					return point.second;
			};

			auto const value_a = value_at (*ia);

			if (ia == ib)
				return value_a;

			if (!value_a)
				return std::nullopt;

			auto const value_b = value_at (*ib);

			if (!value_b)
				return std::nullopt;

			return renormalize (x, Range { ia->first, ib->first }, Range { *value_a, *value_b });
		}

} // namespace neutrino::math

#endif
