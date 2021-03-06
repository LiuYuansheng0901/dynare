/*
 * Copyright (C) 2010-2017 Dynare Team
 *
 * This file is part of Dynare.
 *
 * Dynare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dynare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dynare.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MATRIX_HH
#define _MATRIX_HH

#include <algorithm>
#include <iostream>
#include <iomanip>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include <vector>

#include "Vector.hh"

/*
  This header defines three matrix classes, which implement a "matrix concept"
  (much like the concepts of the Standard Template Library or of Boost
  Library). The first class is a matrix owning the data space for its
  elements, and the other two are matrix "views" of another matrix, i.e. a
  contiguous submatrix. This design philosophy is close to the design of the
  GNU Scientific Library, but here using the syntactic power of C++ class and
  templates, while achieving very high efficiency.

  These classes can be used with various templated functions, including
  wrappers around the BLAS primitives.

  The expressions required to be valid for a class M implementing the "matrix concept" are:
  - M.getRows(): return number of rows
  - M.getCols(): return number of columns
  - M.getLd(): return the leading dimension (here the offset between two columns in the data space, since we are in column-major order)
  - M.getData(): return the pointer to the data space
  - M(i,j): get an element of the matrix

  The expressions required to be valid for a class M implementing the "mutable matrix concept" are (in addition to those of "matrix concept"):
  - M = X: assignment operator
  - M(i,j) = d: assign an element of the matrix
  - M.setAll(d): set all the elements of the matrix
*/

//! A full matrix, implements the "mutable matrix concept"
/*! Owns the data space for the elements */
class Matrix
{
private:
  //! Stored in column-major order, as in Fortran and MATLAB
  double *data;
  const size_t rows, cols;
public:
  Matrix(size_t rows_arg, size_t cols_arg);
  Matrix(size_t size_arg);
  Matrix(const Matrix &arg);
  virtual
  ~Matrix();
  inline size_t
  getRows() const
  {
    return rows;
  }
  inline size_t
  getCols() const
  {
    return cols;
  }
  inline size_t
  getLd() const
  {
    return rows;
  }
  inline double *
  getData()
  {
    return data;
  }
  inline const double *
  getData() const
  {
    return data;
  }
  inline void
  setAll(double val)
  {
    std::fill_n(data, rows*cols, val);
  }
  inline double &
  operator()(size_t i, size_t j)
  {
    return data[i+j*rows];
  }
  inline const double &
  operator()(size_t i, size_t j) const
  {
    return data[i+j*rows];
  }
  //! Assignment operator, only works for matrices of same dimension
  template<class Mat>
  Matrix &
  operator=(const Mat &arg)
  {
    assert(rows == arg.getRows() && cols == arg.getCols());
    for (size_t j = 0; j < cols; j++)
      memcpy(data + j*rows, arg.getData() + j*arg.getLd(), rows*sizeof(double));
    return *this;
  }
  //! The copy assignment operator, which is not generated by the template assignment operator
  /*! See C++ standard, §12.8.9, in the footnote */
  Matrix &operator=(const Matrix &arg);
};

//! A contiguous submatrix of another matrix, implements the "mutable matrix concept"
/*! Does not own the data space for the elements, so depends on another matrix */
class MatrixView
{
private:
  double *const data;
  const size_t rows, cols, ld;
public:
  MatrixView(double *data_arg, size_t rows_arg, size_t cols_arg, size_t ld_arg);
  template<class Mat>
  MatrixView(Mat &arg, size_t row_offset, size_t col_offset,
             size_t rows_arg, size_t cols_arg) :
    data(arg.getData() + row_offset + col_offset*arg.getLd()), rows(rows_arg), cols(cols_arg), ld(arg.getLd())
  {
    assert(row_offset < arg.getRows()
           && row_offset + rows_arg <= arg.getRows()
           && col_offset < arg.getCols()
           && col_offset + cols_arg <= arg.getCols());
  }
  virtual ~MatrixView()
  {
  };
  inline size_t
  getRows() const
  {
    return rows;
  }
  inline size_t
  getCols() const
  {
    return cols;
  }
  inline size_t
  getLd() const
  {
    return ld;
  }
  inline double *
  getData()
  {
    return data;
  }
  inline const double *
  getData() const
  {
    return data;
  }
  inline void
  setAll(double val)
  {
    for (double *p = data; p < data + cols*ld; p += ld)
      std::fill_n(p, rows, val);
  }
  inline double &
  operator()(size_t i, size_t j)
  {
    return data[i+j*ld];
  }
  inline const double &
  operator()(size_t i, size_t j) const
  {
    return data[i+j*ld];
  }
  //! Assignment operator, only works for matrices of same dimension
  template<class Mat>
  MatrixView &
  operator=(const Mat &arg)
  {
    assert(rows == arg.getRows() && cols == arg.getCols());
    for (size_t j = 0; j < cols; j++)
      memcpy(data + j*ld, arg.getData() + j*arg.getLd(), rows*sizeof(double));
    return *this;
  }
  //! The copy assignment operator, which is not generated by the template assignment operator
  /*! See C++ standard, §12.8.9, in the footnote */
  MatrixView &operator=(const MatrixView &arg);
};

//! Like MatrixView, but cannot be modified (implements the "matrix concept")
class MatrixConstView
{
private:
  const double *const data;
  const size_t rows, cols, ld;
public:
  MatrixConstView(const double *data_arg, size_t rows_arg, size_t cols_arg, size_t ld_arg);
  template<class Mat>
  MatrixConstView(const Mat &arg, size_t row_offset, size_t col_offset,
                  size_t rows_arg, size_t cols_arg) :
    data(arg.getData() + row_offset + col_offset*arg.getLd()), rows(rows_arg), cols(cols_arg), ld(arg.getLd())
  {
    assert(row_offset < arg.getRows()
           && row_offset + rows_arg <= arg.getRows()
           && col_offset < arg.getCols()
           && col_offset + cols_arg <= arg.getCols());
  }
  virtual ~MatrixConstView()
  {
  };
  inline size_t
  getRows() const
  {
    return rows;
  }
  inline size_t
  getCols() const
  {
    return cols;
  }
  inline size_t
  getLd() const
  {
    return ld;
  }
  inline const double *
  getData() const
  {
    return data;
  }
  inline const double &
  operator()(size_t i, size_t j) const
  {
    return data[i+j*ld];
  }
};

std::ostream &operator<<(std::ostream &out, const Matrix &M);
std::ostream &operator<<(std::ostream &out, const MatrixView &M);
std::ostream &operator<<(std::ostream &out, const MatrixConstView &M);

namespace mat
{
  //define nullVec (const vector<int>(0)) for assign and order by vector
  // It is used as a proxy for the ":" matlab operator:
  // i.e. zero sized int vector, nullVec, is interpreted as if one supplied ":"
  const std::vector<size_t> nullVec(0);

  template<class Mat>
  void
  print(std::ostream &out, const Mat &M)
  {
    for (size_t i = 0; i < M.getRows(); i++)
      {
        for (size_t j = 0; j < M.getCols(); j++)
          out << std::setw(13) << std::right << M(i, j) << " ";
        out << std::endl;
      }
  }

  template<class Mat>
  inline VectorView
  get_col(Mat &M, size_t j)
  {
    return VectorView(M.getData()+j*M.getLd(), M.getRows(), 1);
  }

  template<class Mat>
  inline VectorView
  get_row(Mat &M, size_t i)
  {
    return VectorView(M.getData()+i, M.getCols(), M.getLd());
  }

  template<class Mat>
  inline VectorConstView
  get_col(const Mat &M, size_t j)
  {
    return VectorConstView(M.getData()+j*M.getLd(), M.getRows(), 1);
  }

  template<class Mat>
  inline VectorConstView
  get_row(const Mat &M, size_t i)
  {
    return VectorConstView(M.getData()+i, M.getCols(), M.getLd());
  }

  template<class Mat1, class Mat2>
  inline void
  col_copy(const Mat1 &src, size_t col_src, Mat2 &dest, size_t col_dest)
  {
    assert(src.getRows() == dest.getRows()
           && col_src < src.getCols() && col_dest < dest.getCols());
    memcpy(dest.getData() + col_dest*dest.getLd(),
           const_cast<double *>(src.getData()) + col_src*src.getLd(),
           src.getRows()*sizeof(double));
  }

  template<class Mat1, class Mat2>
  inline void
  col_copy(const Mat1 &src, size_t col_src, size_t row_offset_src, size_t row_nb,
           Mat2 &dest, size_t col_dest, size_t row_offset_dest)
  {
    assert(col_src < src.getCols() && col_dest < dest.getCols()
           && row_offset_src < src.getRows() && row_offset_src+row_nb <= src.getRows()
           && row_offset_dest < dest.getRows() && row_offset_dest+row_nb <= dest.getRows());
    memcpy(dest.getData() + row_offset_dest + col_dest*dest.getLd(),
           src.getData() + row_offset_src + col_src*src.getLd(),
           row_nb*sizeof(double));
  }

  template<class Mat1, class Mat2>
  inline void
  row_copy(const Mat1 &src, size_t row_src, Mat2 &dest, size_t row_dest)
  {
    assert(src.getCols() == dest.getCols()
           && row_src < src.getRows() && row_dest < dest.getRows());
    const double *p1 = src.getData() + row_src;
    double *p2 = dest.getData() + row_dest;
    while (p1 < src.getData() + src.getCols() * src.getLd())
      {
        *p2 = *p1;
        p1 += src.getLd();
        p2 += dest.getLd();
      }
  }

  template<class Mat>
  inline void
  col_set(Mat &M, size_t col, size_t row_offset, size_t row_nb, double val)
  {
    assert(col < M.getCols());
    assert(row_offset < M.getRows() && row_offset + row_nb <= M.getRows());
    std::fill_n(M.getData() + M.getLd()*col + row_offset, row_nb, val);
  }

  //! Copy under the diagonal the elements above the diagonal
  template<class Mat>
  inline void
  copy_upper_to_lower(Mat &M)
  {
    size_t d = std::min(M.getCols(), M.getRows());
    for (size_t i = 0; i < d; i++)
      for (size_t j = 0; j < i; j++)
        M(i, j) = M(j, i);
  }

  //! Copy above the diagonal the elements under the diagonal
  template<class Mat>
  inline void
  copy_lower_to_upper(Mat &M)
  {
    size_t d = std::min(M.getCols(), M.getRows());
    for (size_t i = 0; i < d; i++)
      for (size_t j = 0; j < i; j++)
        M(j, i) = M(i, j);
  }

  //! Fill the matrix with the identity matrix
  template<class Mat>
  inline void
  set_identity(Mat &M)
  {
    M.setAll(0.0);
    size_t d = std::min(M.getCols(), M.getRows());
    for (size_t i = 0; i < d; i++)
      M(i, i) = 1.0;
  }

  //! In-place transpose of a square matrix
  template<class Mat>
  inline void
  transpose(Mat &M)
  {
    assert(M.getRows() == M.getCols());
    for (size_t i = 0; i < M.getRows(); i++)
      for (size_t j = 0; j < i; j++)
        std::swap(M(i, j), M(j, i));
  }

  //! Computes M1 = M2' (even for rectangular matrices)
  template<class Mat1, class Mat2>
  inline void
  transpose(Mat1 &M1, const Mat2 &M2)
  {
    assert(M1.getRows() == M2.getCols() && M1.getCols() == M2.getRows());
    for (size_t i = 0; i < M1.getRows(); i++)
      for (size_t j = 0; j < M1.getCols(); j++)
        M1(i, j) = M2(j, i);
  }

  //! Computes m1 = m1 + m2
  template<class Mat1, class Mat2>
  void
  add(Mat1 &m1, const Mat2 &m2)
  {
    assert(m1.getRows() == m2.getRows() && m1.getCols() == m2.getCols());
    double *p1 = m1.getData();
    const double *p2 = m2.getData();
    while (p1 < m1.getData() + m1.getCols() * m1.getLd())
      {
        double *pp1 = p1;
        const double *pp2 = p2;
        while (pp1 < p1 + m1.getRows())
          *pp1++ += *pp2++;

        p1 += m1.getLd();
        p2 += m2.getLd();
      }
  }

  //! Computes m1 = m1 + number
  template<class Mat1>
  void
  add(Mat1 &m1, double d)
  {
    double *p1 = m1.getData();

    while (p1 < m1.getData() + m1.getCols() * m1.getLd())
      {
        double *pp1 = p1;
        while (pp1 < p1 + m1.getRows())
          *pp1++ += d;

        p1 += m1.getLd();
      }
  }

  //! Computes m1 = m1 - m2
  template<class Mat1, class Mat2>
  void
  sub(Mat1 &m1, const Mat2 &m2)
  {
    assert(m1.getRows() == m2.getRows() && m1.getCols() == m2.getCols());
    double *p1 = m1.getData();
    const double *p2 = m2.getData();
    while (p1 < m1.getData() + m1.getCols() * m1.getLd())
      {
        double *pp1 = p1;
        const double *pp2 = p2;
        while (pp1 < p1 + m1.getRows())
          *pp1++ -= *pp2++;

        p1 += m1.getLd();
        p2 += m2.getLd();
      }
  }

  //! Computes m1 = m1 - number
  template<class Mat1>
  void
  sub(Mat1 &m1, double d)
  {
    add(m1, -1.0*d);
  }

  //! Does m = -m
  template<class Mat>
  void
  negate(Mat &m)
  {
    double *p = m.getData();
    while (p < m.getData() + m.getCols() * m.getLd())
      {
        double *pp = p;
        while (pp < p + m.getRows())
          {
            *pp = -*pp;
            pp++;
          }

        p += m.getLd();
      }
  }

  // Computes the infinite norm of a matrix
  template<class Mat>
  double
  nrminf(const Mat &m)
  {
    double nrm = 0;
    const double *p = m.getData();
    while (p < m.getData() + m.getCols() * m.getLd())
      {
        const double *pp = p;
        while (pp < p + m.getRows())
          {
            if (fabs(*pp) > nrm)
              nrm = fabs(*pp);
            pp++;
          }

        p += m.getLd();
      }
    return nrm;
  }

  // emulates Matlab command A(:,b)=B(:,d) where b,d are size_t vectors or nullVec as a proxy for ":")
  // i.e. zero sized vector (or mat::nullVec) is interpreted as if one supplied ":" in matlab
  template<class Mat1, class Mat2>
  void
  reorderColumnsByVectors(Mat1 &a, const std::vector<size_t> &vToCols,
                          const Mat2 &b, const std::vector<size_t> &vcols)
  {
    size_t ncols = 0, toncols = 0;
    const std::vector<size_t> *vpToCols = 0, *vpCols = 0;
    std::vector<size_t> tmpvpToCols(0), tmpvpCols(0);
    assert(b.getRows() ==  a.getRows());

    if (vToCols.size() == 0  && vcols.size() == 0)
      a = b;
    else
      {
        if (vToCols.size() == 0)
          {
            toncols = a.getCols();
            tmpvpToCols.reserve(toncols);
            for (size_t i = 0; i < toncols; ++i)
              tmpvpToCols[i] = i;
            vpToCols = (const std::vector<size_t> *)&tmpvpToCols;
          }
        else
          {
            for (size_t i = 0; i < vToCols.size(); ++i)
              {
                assert(vToCols[i] < a.getCols()); //Negative or too large indices
                toncols++;
              }
            assert(toncols <= a.getCols()); // check wrong dimensions for assignment by vector
            vpToCols = &vToCols;
          }

        if (vcols.size() == 0)
          {
            ncols = b.getCols();
            tmpvpCols.reserve(ncols);
            for (size_t i = 0; i < ncols; ++i)
              tmpvpCols[i] = i;
            vpCols = (const std::vector<size_t> *)&tmpvpCols;
          }
        else
          {
            for (size_t i = 0; i < vcols.size(); ++i)
              {
                assert(vcols[i] < b.getCols()); //Negative or too large indices
                ncols++;
              }
            assert(ncols <= b.getCols()); // check wrong dimensions for assignment by vector
            vpCols = &vcols;
          }

        assert(toncols == ncols && ncols > 0);
        for (size_t j = 0; j < ncols; ++j)
          col_copy(b,  (*vpCols)[j], a,  (*vpToCols)[j]);
      }
  }

  // emulates Matlab command A(a,:)=B(c,:) where a,c are size_t vectors or nullVec as a proxy for ":")
  // i.e. zero sized vector (or mat::nullVec) is interpreted as if one supplied ":" in matlab
  template<class Mat1, class Mat2>
  void
  reorderRowsByVectors(Mat1 &a, const std::vector<size_t> &vToRows,
                       const Mat2 &b, const std::vector<size_t> &vrows)
  {
    size_t nrows = 0, tonrows = 0;
    const std::vector<size_t>  *vpToRows = 0, *vpRows = 0;
    std::vector<size_t>  tmpvpToRows(0), tmpvpRows(0);

    //assert(b.getRows() >=  a.getRows() && b.getCols() ==  a.getCols());
    assert(b.getCols() ==  a.getCols());
    if (vToRows.size() == 0  && vrows.size() == 0)
      a = b;
    else
      {
        if (vToRows.size() == 0)
          {
            tonrows = a.getRows();
            tmpvpToRows.reserve(tonrows);
            for (size_t i = 0; i < tonrows; ++i)
              tmpvpToRows[i] = i;
            vpToRows = (const std::vector<size_t> *)&tmpvpToRows;
          }
        else
          {
            for (size_t i = 0; i < vToRows.size(); ++i)
              {
                assert(vToRows[i] < a.getRows()); //Negative or too large indices
                tonrows++;
              }
            assert(tonrows <= a.getRows()); // check wrong dimensions for assignment by vector
            vpToRows = &vToRows;
          }

        if (vrows.size() == 0)
          {
            nrows = b.getRows();
            tmpvpRows.reserve(nrows);
            for (size_t i = 0; i < nrows; ++i)
              tmpvpRows[i] = i;
            vpRows = (const std::vector<size_t> *)&tmpvpRows;
          }
        else
          {
            for (size_t i = 0; i < vrows.size(); ++i)
              {
                assert(vrows[i] < b.getRows()); //Negative or too large indices
                nrows++;
              }
            assert(nrows <= b.getRows()); // check wrong dimensions for assignment by vector
            vpRows = &vrows;
          }

        assert(tonrows == nrows && nrows > 0);
        for (size_t i = 0; i < nrows; ++i)
          row_copy(b, (*vpRows)[i], a, (*vpToRows)[i]);
      }
  }

  // emulates Matlab command A(a,b)=B(c,d) where a,b,c,d are size_t vectors or nullVec as a proxy for ":")
  // i.e. zero sized vector (or mat::nullVec) is interpreted as if one supplied ":" in matlab
  template<class Mat1, class Mat2>
  void
  assignByVectors(Mat1 &a, const std::vector<size_t> &vToRows, const std::vector<size_t> &vToCols,
                  const Mat2 &b, const std::vector<size_t> &vrows, const std::vector<size_t> &vcols)
  {
    size_t nrows = 0, ncols = 0, tonrows = 0, toncols = 0;
    const std::vector<size_t> *vpToCols = 0, *vpToRows = 0, *vpRows = 0, *vpCols = 0;
    std::vector<size_t> tmpvpToCols(0), tmpvpToRows(0), tmpvpRows(0), tmpvpCols(0);

    if (vToRows.size() == 0 && vToCols.size() == 0 && vrows.size() == 0 && vcols.size() == 0)
      a = b;
    else if (vToRows.size() == 0 && vrows.size() == 0)                                                                                                                                                                                                                                                                                                                                                                                                    // just reorder columns
      reorderColumnsByVectors(a, vToCols, b, vcols);
    else if (vToCols.size() == 0 && vcols.size() == 0)                                                                                                                                                                                                                                                                                                                                                                                                    // just reorder rows
      reorderRowsByVectors(a, vToRows, b, vrows);
    else
      {
        if (vToRows.size() == 0)
          {
            tonrows = a.getRows();
            tmpvpToRows.reserve(tonrows);
            for (size_t i = 0; i < tonrows; ++i)
              tmpvpToRows[i] = i;
            vpToRows = (const std::vector<size_t> *)&tmpvpToRows;
          }
        else
          {
            for (size_t i = 0; i < vToRows.size(); ++i)
              {
                assert(vToRows[i] < a.getRows()); //Negative or too large indices
                tonrows++;
              }
            assert(tonrows <= a.getRows()); // check wrong dimensions for assignment by vector
            vpToRows = &vToRows;
          }

        if (vToCols.size() == 0)
          {
            toncols = a.getCols();
            tmpvpToCols.reserve(toncols);
            for (size_t i = 0; i < toncols; ++i)
              tmpvpToCols[i] = i;
            vpToCols = (const std::vector<size_t> *)&tmpvpToCols;
          }
        else
          {
            for (size_t i = 0; i < vToCols.size(); ++i)
              {
                assert(vToCols[i] < a.getCols()); //Negative or too large indices
                toncols++;
              }
            assert(toncols <= a.getCols()); // check wrong dimensions for assignment by vector
            vpToCols = &vToCols;
          }

        if (vrows.size() == 0)
          {
            nrows = b.getRows();
            tmpvpRows.reserve(nrows);
            for (size_t i = 0; i < nrows; ++i)
              tmpvpRows[i] = i;
            vpRows = (const std::vector<size_t> *)&tmpvpRows;
          }
        else
          {
            for (size_t i = 0; i < vrows.size(); ++i)
              {
                assert(vrows[i] < b.getRows()); //Negative or too large indices
                nrows++;
              }
            assert(nrows <= b.getRows()); // check wrong dimensions for assignment by vector
            vpRows = &vrows;
          }

        if (vcols.size() == 0)
          {
            ncols = b.getCols();
            tmpvpCols.reserve(ncols);
            for (size_t i = 0; i < ncols; ++i)
              tmpvpCols[i] = i;
            vpCols = (const std::vector<size_t> *)&tmpvpCols;
          }
        else
          {
            for (size_t i = 0; i < vcols.size(); ++i)
              {
                assert(vcols[i] < b.getCols()); //Negative or too large indices
                ncols++;
              }
            assert(ncols <= b.getCols()); // check wrong dimensions for assignment by vector
            vpCols = &vcols;
          }

        assert(tonrows == nrows && toncols == ncols && nrows * ncols > 0);
        for (size_t i = 0; i < nrows; ++i)
          for (size_t j = 0; j < ncols; ++j)
            a((*vpToRows)[i], (*vpToCols)[j]) = b((*vpRows)[i], (*vpCols)[j]);
      }
  }

  //emulates Matlab repmat: Mat2 = multv*multh tiled [Mat1]
  template<class Mat1, class Mat2 >
  void
  repmat(Mat1 &a, size_t multv, size_t multh, Mat2 &repMat)   // vertical and horisontal replicators
  {
    assert(repMat.getRows() == multv * a.getRows() && repMat.getCols() == multh * a.getCols());
    for (size_t i = 0; i < multv; ++i)
      for (size_t j = 0; j < multh; ++j)
        for (size_t k = 0; k < a.getCols(); ++k)
          col_copy(a, k, 0, a.getRows(), repMat, a.getCols() * j + k, a.getRows() * i);
  };

  template<class Mat1, class Mat2>
  bool
  isDiff(const Mat1 &m1, const Mat2 &m2, const double tol = 0.0)
  {
    assert(m2.getRows() == m1.getRows() && m2.getCols() == m1.getCols());
    const double *p1 = m1.getData();
    const double *p2 = m2.getData();
    while (p1 < m1.getData() + m1.getCols() * m1.getLd())
      {
        const double *pp1 = p1;
        const double *pp2 = p2;
        while (pp1 < p1 + m1.getRows())
          if (fabs(*pp1++ - *pp2++) > tol)
            return true;

        p1 += m1.getLd();
        p2 += m2.getLd();
      }
    return false;
  }

  //traverse the upper triangle only along diagonals where higher changes occur
  template<class Mat1, class Mat2>
  bool
  isDiffSym(const Mat1 &m1, const Mat2 &m2, const double tol = 0.0)
  {
    assert(m2.getRows() == m1.getRows() && m2.getCols() == m1.getCols()
           && m2.getRows() == m1.getCols() && m2.getCols() == m1.getRows());
    for (size_t i = 0; i < m1.getCols(); i++)
      for (size_t j = 0; i + j < m1.getCols(); j++)
        if (fabs(m1(j, j + i) - m2(j, j + i)) > tol)
          return true;
    return false;
  }

} // End of namespace

#endif
