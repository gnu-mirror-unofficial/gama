/*
  GNU Gama -- adjustment of geodetic networks
  Copyright (C) 1999, 2006  Ales Cepek <cepek@gnu.org>

  This file is part of the GNU Gama C++ library.

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GNU Gama.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GNU_Gama_gnu_gama_gnugama_GaMa_OLS_gso_h
#define GNU_Gama_gnu_gama_gnugama_GaMa_OLS_gso_h

#include <matvec/gso.h>
#include <gnu_gama/adj/adj_basefull.h>
#include <cmath>

namespace GNU_gama {

  template <typename Float, typename Index, typename Exc>
  class AdjGSO : public AdjBaseFull<Float, Index, Exc> {

  public:

    AdjGSO() = default;
   ~AdjGSO() override = default;

    AdjGSO (const AdjGSO&) = delete;
    AdjGSO& operator=(const AdjGSO&) = delete;
    AdjGSO (const AdjGSO&&) = delete;
    AdjGSO& operator=(const AdjGSO&&) = delete;

    AdjGSO(const Mat<Float, Index, Exc>& A, const Vec<Float, Index, Exc>& b)
      : AdjBaseFull<Float, Index, Exc>(A, b) {}

    void reset(const Mat<Float, Index, Exc>& A,
               const Vec<Float, Index, Exc>& b) override
    {
      AdjBaseFull<Float, Index, Exc>::reset(A, b);
    }

    Index defect() override { return gso.defect(); }
    bool  lindep(Index i) override { return gso.lindep(i); }

    Float q_xx(Index i, Index j) override;
    Float q_bb(Index i, Index j) override;
    Float q_bx(Index i, Index j) override;

    void min_x() override { gso.min_x(); }
    void min_x(Index n, Index x[]) override { gso.min_x(n, x); }

    void solve() override;

  private:

    Mat<Float, Index, Exc> A_;
    GSO<Float, Index, Exc> gso;

    void init_gso_();
  };

  // ...................................................................


  template <typename Float, typename Index, typename Exc>
  void AdjGSO<Float, Index, Exc>::solve()
  {
    if (this->is_solved) return;

    const Index M = this->pA->rows();
    const Index N = this->pA->cols();

    A_.reset(M+N, N+1);

    const Mat<Float, Index, Exc>& A1 = *this->pA;
    const Vec<Float, Index, Exc>& b1 = *this->pb;

    for (Index i=1; i<=M; i++)
      {
        A_(i, N+1) = -b1(i);
        for (Index j=1; j<=N; j++) A_(i, j) = A1(i, j);
      }

    for (Index i=1; i<=N; i++)
      for (Index j=1; j<=N+1; j++)
        A_(M+i, j) = (i==j) ? 1 : 0;

    gso.reset(A_, M, N);
    gso.gso1();
    gso.gso2();

    this->x.reset(N);
    for (Index i=1; i<=N; i++)
      this->x(i) = A_(M+i, N+1);

    this->r.reset(M);
    for (Index j=1; j<=M; j++)
      this->r(j) = A_(j, N+1);

    this->is_solved = true;
  }


  template <typename Float, typename Index, typename Exc>
  Float AdjGSO<Float, Index, Exc>::q_xx(Index i, Index j)
  {
    if(!this->is_solved) solve();
    const Index M = this->pA->rows();
    const Index N = this->pA->cols();
    i += M;
    j += M;
    Float s = 0;
    for (Index k=1; k<=N; k++)
      s += A_(i,k)*A_(j,k);              // cov x_i x_j
    return s;
  }


  template <typename Float, typename Index, typename Exc>
  Float AdjGSO<Float, Index, Exc>::q_bb(Index i, Index j)
  {
    if(!this->is_solved) solve();

    const Index N = this->pA->cols();
    Float s = 0;
    for (Index k=1; k<=N; k++)
      s += A_(i,k)*A_(j,k);              // cov b_i b_j
    return s;
  }


  template <typename Float, typename Index, typename Exc>
  Float AdjGSO<Float, Index, Exc>::q_bx(Index i, Index j)
  {
    if(!this->is_solved) solve();

    const Index M = this->pA->rows();
    const Index N = this->pA->cols();
    j += M;
    Float s = 0;
    for (Index k=1; k<=N; k++)
      s += A_(i,k)*A_(j,k);              // cov b_i x_j
    return s;
  }


}   // GNU_gama

#endif
