// { dg-do compile { target c++14 } }

// Copyright (C) 2014-2025 Free Software Foundation, Inc.
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

#include <algorithm>
#include <functional>
#include <utility>

static_assert(std::minmax(2, 1) ==
	      std::pair<const int&, const int&>(1, 2), "");
static_assert(std::minmax(2, 1, std::greater<int>()) ==
	      std::pair<const int&, const int&>(2, 1), "");
static_assert(std::minmax({2, 1}) ==
	      std::pair<int, int>(1, 2), "");
static_assert(std::minmax({2, 1}, std::greater<int>())==
	      std::pair<int, int>(2, 1), "");
