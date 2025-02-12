// { dg-do compile { target c++11 } }

// Copyright (C) 2010-2025 Free Software Foundation, Inc.
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

// 18.9.3 Initializer list range access [support.initlist.range]

#include <initializer_list>

void
test01()
{
  std::begin({1, 2, 3});
  std::end({1, 2, 3});
}

void
test02()
{
  static constexpr std::initializer_list<int> il{1};
  static_assert( std::begin(il) == il.begin() );
  static_assert( std::end(il) == il.end() );
  static_assert( noexcept(std::begin(il)) );
  static_assert( noexcept(std::end(il)) );
}
