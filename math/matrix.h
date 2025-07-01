/* vim:ts=4
 *
 * Copyleft 2012…2017  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__MATH__MATRIX_H__INCLUDED
#define NEUTRINO__MATH__MATRIX_H__INCLUDED

// Neutrino:
#include <neutrino/math/concepts.h>
#include <neutrino/math/utility.h>

// Standard:
#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>


namespace neutrino::math {

// Forward
template<Scalar pScalar, CoordinateSystem pTargetSpace, CoordinateSystem pSourceSpace>
	class Quaternion;


class BasicMatrix
{
  protected:
	static std::out_of_range
	make_out_of_range_exception (std::size_t column, std::size_t row)
		{ return std::out_of_range ("element [" + std::to_string (column) + ", " + std::to_string (row) + "] is out of bounds in the Matrix"); }
};


/**
 * A matrix. Should be self explanatory.
 *
 * \param	pScalar
 *			Algebraic value type, probably a double or something.
 * \param	pTargetSpace
 *			A target CoordinateSystem type tag.
 *			Matrix is supposed to transform frame of reference from pSourceSpace to pTargetSpace. This can be useful when using matrices as transformations for
 *			vector spaces. If used, Matrices with different type tags become incompatible, but certain operations are still defined, for example:
 *			  struct TargetSpace;
 *			  struct IntermediateSpace;
 *			  struct SourceSpace;
 *			  M<..., TargetSpace, SourceSpace> x = M<..., TargetSpace, IntermediateSpace>{} * M<..., IntermediateSpace, SourceSpace>{};
 * \param	pSourceSpace
 *			Source CoordinateSystem types.
 */
template<Scalar pScalar, std::size_t pColumns, std::size_t pRows, CoordinateSystem pTargetSpace = void, CoordinateSystem pSourceSpace = pTargetSpace>
	class Matrix: public BasicMatrix
	{
	  public:
		static constexpr std::size_t kColumns	= pColumns;
		static constexpr std::size_t kRows		= pRows;

		using Scalar			= pScalar;
		using SquaredScalar	= std::remove_cvref_t<decltype (std::declval<Scalar>() * std::declval<Scalar>())>;
		using InverseScalar		= decltype (1.0 / std::declval<pScalar>());
		using ColumnVector		= Matrix<Scalar, 1, pRows, pTargetSpace, void>;
		using InverseMatrix		= Matrix<InverseScalar, pColumns, pRows, pSourceSpace, pTargetSpace>;
		using TransposedMatrix	= Matrix<Scalar, pRows, pColumns, pSourceSpace, pTargetSpace>;
		using TargetSpace		= pTargetSpace;
		using SourceSpace		= pSourceSpace;

		template<std::size_t NewColumns, std::size_t NewRows>
			using Resized		= Matrix<Scalar, NewColumns, NewRows, pTargetSpace, pSourceSpace>;

		template<class NewScalar>
			using Retyped		= Matrix<NewScalar, pColumns, pRows, pTargetSpace, pSourceSpace>;

	  public:
		[[nodiscard]]
		static consteval bool
		is_scalar()
			{ return kColumns == 1 && kRows == 1; }

		[[nodiscard]]
		static consteval bool
		is_vector()
			{ return kColumns == 1; }

		[[nodiscard]]
		static consteval bool
		is_square()
			{ return kColumns == kRows; }

		template<class ...Ts>
			[[nodiscard]]
			static consteval bool
			is_scalar_pack()
				{ return (sizeof...(Ts) == kRows * kColumns) && (std::is_convertible_v<std::remove_cvref_t<Ts>, Scalar> && ...); }

		template<class ...Ts>
			[[nodiscard]]
			static consteval bool
			is_column_vector_pack()
				{ return (sizeof...(Ts) == kColumns) && (std::is_same_v<std::remove_cvref_t<Ts>, ColumnVector> && ...); }

	  public:
		// Ctor. Same as using ZeroInitializer
		constexpr
		Matrix() noexcept;

		// Ctor. Copy constructor.
		constexpr
		Matrix (Matrix const&) noexcept;

		// Ctor. Alias for default constructor (initialized to zero).
		constexpr
		Matrix (ZeroInitializer) noexcept;

		// Ctor. Initializes to identity matrix.
		constexpr
		Matrix (UnitInitializer) noexcept;

		// Ctor. Initializes to identity matrix.
		constexpr
		Matrix (IdentityInitializer) noexcept;

		// Ctor. Doesn't initialize matrix at all if Scalar is a fundamental type; otherwise equivalent to ZeroInitializer.
		constexpr
		Matrix (UninitializedInitializer) noexcept;

		// Ctor. Initializes from scalar. Only for 1x1 matrices.
		template<class T>
			requires (is_scalar() && std::is_convertible_v<std::remove_cvref_t<T>, Scalar>)
			constexpr
			Matrix (T scalar) noexcept:
				_data ({ scalar })
			{ }

		/**
		 * Ctor. Initializes from sequence of scalars: {
		 *	 R0C0, R0C1, R0C2,
		 *	 R1C0, R1C1, R1C2,
		 *	 ...
		 * }
		 */
		template<class Iterator>
			requires (std::is_convertible_v<decltype (*std::declval<Iterator>()), Scalar>)
			constexpr
			Matrix (Iterator begin, Iterator end)
				{ std::copy (begin, end, _data.begin()); }

		// Ctor. Initializes from std::array of scalars.
		explicit constexpr
		Matrix (std::array<Scalar, kColumns * kRows> const& values) noexcept;

		// Ctor. Initializes columns from std::array of vectors.
		explicit constexpr
		Matrix (std::array<ColumnVector, kColumns> const& vectors) noexcept;

		explicit constexpr
		Matrix (Quaternion<Scalar, TargetSpace, SourceSpace> const&) noexcept;

		// Ctor. Initializes from scalars list.
		template<class ...Ts>
			requires (is_scalar_pack<Ts...>() || is_column_vector_pack<Ts...>())
			constexpr
			Matrix (Ts&& ...values) noexcept
			{
				if constexpr ((std::is_convertible_v<std::remove_cvref_t<Ts>, Scalar> && ...))
				{
					static_assert (sizeof...(Ts) == kRows * kColumns, "Invalid number of scalar arguments");

					recursive_initialize_from_scalars (0, std::forward<Ts> (values)...);
				}
				else if constexpr ((std::is_same_v<std::remove_cvref_t<Ts>, ColumnVector> && ...))
				{
					static_assert (sizeof...(Ts) == kColumns, "Invalid number of vector arguments");

					recursive_initialize_from_vectors (0, std::forward<Ts> (values)...);
				}
				else
					static_assert (false, "Impossible code path");
			}

		// Ctor method
		static constexpr Matrix
		equal_diagonal (Scalar diagonal_value)
			requires (is_square());

		// Copy operator
		constexpr Matrix&
		operator= (Matrix const&)
			noexcept (std::is_copy_assignable_v<Scalar>);

		// Move operator
		constexpr Matrix&
		operator= (Matrix&&)
			noexcept (std::is_move_assignable_v<Scalar>) = default;

		/**
		 * Equality operator
		 */
		[[nodiscard]]
		constexpr bool
		operator== (Matrix const& other) const
			noexcept (noexcept (Scalar{} == Scalar{}))
		{ return _data == other._data; }

		/**
		 * Difference operator
		 */
		[[nodiscard]]
		constexpr bool
		operator!= (Matrix const& other) const
			noexcept (noexcept (Scalar{} != Scalar{}))
		{ return _data != other._data; }

		/**
		 * Array of data.
		 */
		[[nodiscard]]
		constexpr std::array<Scalar, kColumns * kRows>&
		components() noexcept
			{ return _data; }

		/**
		 * Array of data.
		 */
		[[nodiscard]]
		constexpr std::array<Scalar, kColumns * kRows> const&
		components() const noexcept
			{ return _data; }

		/**
		 * If size is 1x1, then it should be convertible to scalar.
		 */
		[[nodiscard]]
		constexpr Scalar
		scalar() const noexcept
			requires (is_scalar())
		{ return at (0, 0); }

		/**
		 * Safe element accessor. Throws std::out_of_range when accessing elements outside matrix.
		 */
		[[nodiscard]]
		constexpr Scalar&
		at (std::size_t column, std::size_t row);

		/**
		 * Safe element accessor. Throws std::out_of_range when accessing elements outside matrix.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		at (std::size_t column, std::size_t row) const
			{ return const_cast<Matrix&> (*this).at (column, row); }

		/**
		 * Fast element accessor. Doesn't perform range checks.
		 */
		[[nodiscard]]
		constexpr Scalar&
		operator[] (std::size_t column, std::size_t row) noexcept
			{ return _data[row * kColumns + column]; }

		/**
		 * Fast element accessor. Doesn't perform range checks.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		operator[] (std::size_t column, std::size_t row) const noexcept
			{ return _data[row * kColumns + column]; }

		/**
		 * Vector access operator.
		 * Accesses only the first column of the matrix.
		 */
		[[nodiscard]]
		constexpr Scalar&
		operator[] (std::size_t index) noexcept
			requires (is_vector())
		{ return _data[index]; }

		/**
		 * Vector access operator - const version.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		operator[] (std::size_t index) const noexcept
			requires (is_vector())
		{ return _data[index]; }

		/**
		 * Return first value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar&
		x() noexcept
			requires (is_vector() && kRows >= 1)
		{ return _data[0]; }

		/**
		 * Return first value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		x() const noexcept
			requires (is_vector() && kRows >= 1)
		{ return _data[0]; }

		/**
		 * Return second value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar&
		y() noexcept
			requires (is_vector() && kRows >= 2)
		{ return _data[1]; }

		/**
		 * Return second value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		y() const noexcept
			requires (is_vector() && kRows >= 2)
		{ return _data[1]; }

		/**
		 * Return third value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar&
		z() noexcept
			requires (is_vector() && kRows >= 3)
		{ return _data[2]; }

		/**
		 * Return third value if this matrix is a vector.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		z() const noexcept
			requires (is_vector() && kRows >= 3)
		{ return _data[2]; }

		/**
		 * Structured bindings support.
		 */
		template<std::size_t I>
			constexpr Scalar
			get() const noexcept
				requires (kColumns == 1 || kRows == 1)
			{ return _data[I]; }

		/**
		 * Return length of this vector.
		 */
		[[nodiscard]]
		constexpr Scalar
		norm() const noexcept
			requires (is_vector())
		{
			using std::sqrt;

			SquaredScalar sum (0);

			for (auto v: _data)
				sum += v * v;

			return sqrt (sum);
		}

		constexpr void
		normalize()
			requires (is_vector())
		{ *this = *this / (1 / Scalar (1)) * norm(); }

		/**
		 * Return normalized version of this vector.
		 */
		[[nodiscard]]
		constexpr auto
		normalized() const
			requires (is_vector())
		{ return Scalar (1) * *this / norm(); }

		/**
		 * Return given column as a vector.
		 */
		[[nodiscard]]
		constexpr ColumnVector
		column (std::size_t index) const noexcept;

		/**
		 * Return number of columns.
		 */
		[[nodiscard]]
		consteval std::size_t
		n_columns() const
			{ return kColumns; }

		/**
		 * Return number of rows.
		 */
		[[nodiscard]]
		consteval std::size_t
		n_rows() const
			{ return kRows; }

		/**
		 * Return inverted matrix.
		 * If matrix is not invertible, it will contain infinities or NaNs.
		 */
		[[nodiscard]]
		constexpr InverseMatrix
		inverted() const;

		/**
		 * Return transposed matrix.
		 */
		[[nodiscard]]
		constexpr TransposedMatrix
		transposed() const noexcept;

		/**
		 * Alias for transposed().
		 */
		[[nodiscard]]
		constexpr TransposedMatrix
		operator~() const noexcept
			{ return transposed(); }

		/**
		 * Returns negated version of the matrix.
		 */
		[[nodiscard]]
		constexpr Matrix
		operator-() const noexcept
			{ return -1.0 * *this; }

		/**
		 * Returns unchanged matrix.
		 */
		[[nodiscard]]
		constexpr Matrix const&
		operator+() const noexcept
			{ return *this; }

		/**
		 * Add another matrix to this one.
		 */
		constexpr Matrix&
		operator+= (Matrix const&)
			noexcept (noexcept (Scalar{} + Scalar{}));

		/**
		 * Subtract another matrix from this one.
		 */
		constexpr Matrix&
		operator-= (Matrix const&)
			noexcept (noexcept (Scalar{} - Scalar{}));

		/**
		 * Multiply this matrix by a scalar.
		 */
		template<class OtherScalar>
			requires (sizeof (Scalar{} *= OtherScalar{}) > 0) // Using sizeof() for SFINAE to ensure the expression is valid.
			constexpr Matrix&
			operator*= (OtherScalar const& scalar)
				noexcept (noexcept (Scalar{} *= OtherScalar{}))
			{
				for (auto& d: _data)
					d *= scalar;

				return *this;
			}

		/**
		 * Multiply this matrix by another matrix.
		 */
		template<class OtherScalar>
			requires (sizeof (Scalar{} *= OtherScalar{}) > 0) // Using sizeof() for SFINAE to ensure the expression is valid.
			constexpr Matrix&
			operator*= (Retyped<OtherScalar> const& other)
				noexcept (noexcept (Scalar{} *= OtherScalar{}))
			{
				static_assert (is_square(), "Matrix needs to be square");

				// Use global operator*():
				return *this = *this * other;
			}

		/**
		 * Return a new Matrix with all values of this transformed by given transform function.
		 */
		template<class TransformFunction>
			[[nodiscard]]
			constexpr auto
			transformed (TransformFunction&& transform_function) const
			{
				Retyped<std::invoke_result_t<TransformFunction, Scalar>> result;

				for (std::size_t i = 0; i < _data.size(); ++i)
					result[i] = transform_function (_data[i]);

				return result;
			}

		/**
		 * Put another matrix inside this one.
		 */
		template<class OtherMatrix>
			constexpr void
			put (OtherMatrix const& other, std::size_t at_column, std::size_t at_row)
			{
				if (other.n_columns() + at_column > n_columns() ||
					other.n_rows() + at_row > n_rows())
				{
					throw make_out_of_range_exception (at_column, at_row);
				}

				auto target_r = at_row;

				for (std::size_t src_r = 0; src_r < other.n_rows(); ++src_r, ++target_r)
				{
					auto target_c = at_column;

					for (std::size_t src_c = 0; src_c < other.n_columns(); ++src_c, ++target_c)
						(*this)[target_c, target_r] = other[src_c, src_r];
				}
			}

	  private:
		template<class ...Ts>
			constexpr void
			recursive_initialize_from_scalars (std::size_t position, Scalar const& scalar, Ts&& ...rest) noexcept
			{
				_data[position] = scalar;

				if constexpr (sizeof...(rest) > 0)
					recursive_initialize_from_scalars (position + 1, std::forward<Ts> (rest)...);
			}

		template<class ...Ts>
			constexpr void
			recursive_initialize_from_vectors (std::size_t position, ColumnVector const& vector, Ts&& ...rest) noexcept
			{
				for (std::size_t r = 0; r < kRows; ++r)
					at (position, r) = vector[r];

				if constexpr (sizeof...(rest) > 0)
					recursive_initialize_from_vectors (position + 1, std::forward<Ts> (rest)...);
			}

	  private:
		alignas(8) std::array<Scalar, kColumns * kRows> _data;
	};


template<Scalar S, std::size_t N, CoordinateSystem TS = void, CoordinateSystem SS = void>
	using Vector = Matrix<S, 1, N, TS, SS>;


template<Scalar S, std::size_t N, CoordinateSystem TS = void, CoordinateSystem SS = TS>
	using SquareMatrix = Matrix<S, N, N, TS, SS>;


/*
 * Implementation
 */


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix() noexcept:
		Matrix (zero)
	{ }


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (Matrix const& other) noexcept:
		_data (other._data)
	{ }


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (ZeroInitializer) noexcept
	{
		_data.fill (Scalar { 0 });
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (UnitInitializer) noexcept:
		Matrix (zero)
	{
		static_assert (is_square(), "Matrix has to be square");

		for (std::size_t i = 0; i < kColumns; ++i)
			(*this)[i, i] = S (1);
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (IdentityInitializer) noexcept:
		Matrix (unit)
	{ }


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (UninitializedInitializer) noexcept
	{ }


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (std::array<Scalar, kColumns * kRows> const& initial_values) noexcept:
		_data (initial_values)
	{ }


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (std::array<ColumnVector, kColumns> const& initial_vectors) noexcept
	{
		for (std::size_t r = 0; r < kRows; ++r)
			for (std::size_t c = 0; c < kColumns; ++c)
				(*this)[c, r] = initial_vectors[c][r];
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr Matrix<S, C, R, TS, SS>
	Matrix<S, C, R, TS, SS>::equal_diagonal (S const diagonal_value)
		requires (is_square())
	{
		Matrix m (zero);

		for (std::size_t d = 0; d < kColumns; ++d)
			m[d, d] = diagonal_value;

		return m;
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Matrix<S, C, R, TS, SS>::Matrix (Quaternion<Scalar, TargetSpace, SourceSpace> const& quaternion) noexcept
	{
		static_assert (kColumns == 3 && kRows == 3, "Only square 3x3 matrix can be created froma Quaternion");

		auto const w = quaternion.w();
		auto const x = quaternion.x();
		auto const y = quaternion.y();
		auto const z = quaternion.z();

		auto const ww = w * w;
		auto const wx = w * x;
		auto const wy = w * y;
		auto const wz = w * z;

		auto const xx = x * x;
		auto const xy = x * y;
		auto const xz = x * z;

		auto const yy = y * y;
		auto const yz = y * z;

		auto const zz = z * z;

		_data = {
			2.0 * (ww + xx) - 1.0,	2.0 * (xy - wz),		2.0 * (xz + wy),
			2.0 * (xy + wz),		2.0 * (ww + yy) - 1.0,	2.0 * (yz - wx),
			2.0 * (xz - wy),		2.0 * (yz + wx),		2.0 * (ww + zz) - 1.0,
		};
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Matrix<S, C, R, TS, SS>::at (std::size_t column, std::size_t row) -> Scalar&
	{
		if (column >= kColumns || row >= kRows)
			throw make_out_of_range_exception (column, row);

		return _data[row * kColumns + column];
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Matrix<S, C, R, TS, SS>::column (std::size_t index) const noexcept -> ColumnVector
	{
		ColumnVector result;

		for (std::size_t r = 0; r < kRows; ++r)
			result[r] = at (index, r);

		return result;
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Matrix<S, C, R, TS, SS>::inverted() const -> InverseMatrix
	{
		static_assert (is_square(), "Matrix needs to be square");

		if constexpr (is_scalar())
			return { 1.0 / (*this)[0, 0] };
		else if constexpr (kColumns == 2)
		{
			auto const& self = *this;
			auto const det = (self[0, 0] * self[1, 1] - self[1, 0] * self[0, 1]);
			auto const scaler = 1.0 / det;

			return InverseMatrix {
				scaler * +self[1, 1], scaler * -self[1, 0],
				scaler * -self[0, 1], scaler * +self[0, 0],
			};
		}
		else if constexpr (kColumns == 3)
		{
			auto const& self = *this;
			auto const a = self[0, 0];
			auto const b = self[1, 0];
			auto const c = self[2, 0];
			auto const d = self[0, 1];
			auto const e = self[1, 1];
			auto const f = self[2, 1];
			auto const g = self[0, 2];
			auto const h = self[1, 2];
			auto const i = self[2, 2];
			auto const kA = +(e * i - f * h);
			auto const kB = -(d * i - f * g);
			auto const kC = +(d * h - e * g);
			auto const kD = -(b * i - c * h);
			auto const kE = +(a * i - c * g);
			auto const kF = -(a * h - b * g);
			auto const kG = +(b * f - c * e);
			auto const kH = -(a * f - c * d);
			auto const kI = +(a * e - b * d);
			auto const det = a * kA + b * kB + c * kC;
			auto const scaler = 1.0 / det;

			return InverseMatrix {
				scaler * kA, scaler * kD, scaler * kG,
				scaler * kB, scaler * kE, scaler * kH,
				scaler * kC, scaler * kF, scaler * kI,
			};
		}
		else
		{
			// Gauss-Jordan inversion.

			auto m = *this / Scalar (1.0);
			auto E = Matrix<double, kColumns, kRows, SourceSpace, TargetSpace> (math::identity);

			auto const divide_row = [](auto& matrix, std::size_t row, auto value) {
				auto const inv_value = 1.0 / value;

				for (std::size_t c = 0; c < kColumns; ++c)
					matrix[c, row] *= inv_value;
			};

			auto const substract_row = [](auto& matrix, std::size_t row, auto value, std::size_t mult_row) {
				for (std::size_t c = 0; c < kColumns; ++c)
					matrix[c, row] -= value * matrix[c, mult_row];
			};

			for (std::size_t step = 0; step < kColumns; ++step)
			{
				{
					auto const v = m[step, step];
					divide_row (m, step, v);
					divide_row (E, step, v);
				}

				for (std::size_t r = 0; r < kRows; ++r)
				{
					if (r != step)
					{
						auto const v = m[step, r];
						substract_row (m, r, v, step);
						substract_row (E, r, v, step);
					}
				}
			}

			return E * InverseScalar (1.0);
		}
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Matrix<S, C, R, TS, SS>::transposed() const noexcept -> TransposedMatrix
	{
		if constexpr (kRows == 1 || kColumns == 1)
		{
			// If it's a vector or transposed vector, the underlying std::array<>
			// will be exactly the same after transposition, so just copy the array.
			return TransposedMatrix (_data);
		}
		else
		{
			Matrix<Scalar, kRows, kColumns, SS, TS> result;

			for (std::size_t r = 0; r < kRows; ++r)
				for (std::size_t c = 0; c < kColumns; ++c)
					result[r, c] = (*this)[c, r];

			return result;
		}
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr Matrix<S, C, R, TS, SS>&
	Matrix<S, C, R, TS, SS>::operator= (Matrix const& other)
		noexcept (std::is_copy_assignable_v<Scalar>)
	{
		std::copy (other._data.begin(), other._data.end(), _data.begin());
		return *this;
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr Matrix<S, C, R, TS, SS>&
	Matrix<S, C, R, TS, SS>::operator+= (Matrix const& other)
		noexcept (noexcept (Scalar{} + Scalar{}))
	{
		std::transform (_data.begin(), _data.end(), other._data.begin(), _data.begin(), std::plus<Scalar>());
		return *this;
	}


template<Scalar S, std::size_t C, std::size_t R, CoordinateSystem TS, CoordinateSystem SS>
	constexpr Matrix<S, C, R, TS, SS>&
	Matrix<S, C, R, TS, SS>::operator-= (Matrix const& other)
		noexcept (noexcept (Scalar{} - Scalar{}))
	{
		std::transform (_data.begin(), _data.end(), other._data.begin(), _data.begin(), std::minus<Scalar>());
		return *this;
	}


/*
 * Global functions
 */


template<CoordinateSystem NewTargetSpace, CoordinateSystem NewSourceSpace, CoordinateSystem OldTargetSpace, CoordinateSystem OldSourceSpace, Scalar S, std::size_t Columns, std::size_t Rows>
	[[nodiscard]]
	constexpr auto&
	coordinate_system_cast (Matrix<S, Columns, Rows, OldTargetSpace, OldSourceSpace>& matrix)
	{
		using Scalar = typename std::remove_cvref_t<decltype (matrix)>::Scalar;
		return reinterpret_cast<Matrix<Scalar, Columns, Rows, NewTargetSpace, NewSourceSpace>&> (matrix);
	}


template<CoordinateSystem NewTargetSpace, CoordinateSystem NewSourceSpace, CoordinateSystem OldTargetSpace, CoordinateSystem OldSourceSpace, Scalar S, std::size_t Columns, std::size_t Rows>
	[[nodiscard]]
	constexpr auto const&
	coordinate_system_cast (Matrix<S, Columns, Rows, OldTargetSpace, OldSourceSpace> const& matrix)
	{
		using Scalar = typename std::remove_cvref_t<decltype (matrix)>::Scalar;
		return reinterpret_cast<Matrix<Scalar, Columns, Rows, NewTargetSpace, NewSourceSpace> const&> (matrix);
	}

} // namespace neutrino::math


// Local:
#include "matrix_operations.h"

#endif

