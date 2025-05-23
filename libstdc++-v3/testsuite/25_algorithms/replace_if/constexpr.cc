// Copyright (C) 2019-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// { dg-do compile { target c++20 } }

#include <algorithm>
#include <array>

constexpr bool
test()
{
  std::array<int, 12> ar0{{0, 1, 2, 3, 4, 5, 6, 6, 8, 9, 9, 11}};

  std::replace_if(ar0.begin(), ar0.end(), [](int i){ return i % 2 == 1; }, 42);

  std::array<int, 12> ar1{{0, 42, 2, 42, 4, 42, 6, 6, 8, 42, 42, 42}};

  return ar0 == ar1;
}

static_assert(test());
