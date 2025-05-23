// Copyright (C) 2005-2025 Free Software Foundation, Inc.
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

// TR1 2.2.2 Template class shared_ptr [tr.util.smartptr.shared]

// { dg-add-options using-deprecated }
// { dg-warning "auto_ptr. is deprecated" "" { target c++11 } 0 }

#include <tr1/memory>
#include <testsuite_hooks.h>

struct A
{
  A() { ++ctor_count; }
  virtual ~A() { ++dtor_count; }
  static long ctor_count;
  static long dtor_count;
};
long A::ctor_count = 0;
long A::dtor_count = 0;

struct B : A
{
  B() { ++ctor_count; }
  virtual ~B() { ++dtor_count; }
  static long ctor_count;
  static long dtor_count;
};
long B::ctor_count = 0;
long B::dtor_count = 0;


struct reset_count_struct
{
  ~reset_count_struct()
  {
    A::ctor_count = 0;
    A::dtor_count = 0;
    B::ctor_count = 0;
    B::dtor_count = 0;
  }
};


// 2.2.3.3 shared_ptr assignment [tr.util.smartptr.shared.assign]

// Assignment from auto_ptr<Y>
int
test01()
{
  reset_count_struct __attribute__((unused)) reset;

  std::tr1::shared_ptr<A> a(new A);
  std::auto_ptr<B> b(new B);
  a = b;
  VERIFY( a.get() != 0 );
  VERIFY( b.get() == 0 );
  VERIFY( A::ctor_count == 2 );
  VERIFY( A::dtor_count == 1 );
  VERIFY( B::ctor_count == 1 );
  VERIFY( B::dtor_count == 0 );

  return 0;
}

int
main()
{
  test01();
  return 0;
}
