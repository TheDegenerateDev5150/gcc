// { dg-options "-Wno-deprecated" }

// Copyright (C) 2002-2025 Free Software Foundation, Inc.
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

// hash_map (SGI extension)

#include <cstdlib>
#include <string>
#include <hash_map>
#include <testsuite_hooks.h>

namespace __gnu_cxx 
{
  using std::string;

  inline size_t hash_string(const char* s)
  {
    unsigned long h; 
    for (h=0; *s; ++s) {
      h = 5*h + *s;
    }
    return size_t(h);
  }

  template<class T> struct hash<T *>
  {
    size_t operator()(const T *const & s) const
      { return reinterpret_cast<size_t>(s); }
  };    
  
  template<> struct hash<string>
  {
    size_t operator()(const string &s) const { return hash_string(s.c_str()); }
  };

  template<> struct hash<const string>
  {
    size_t operator()(const string &s) const { return hash_string(s.c_str()); }
  };

  template<class T1, class T2> struct hash<pair<T1,T2> >
  {
    hash<T1> __fh;
    hash<T2> __sh;
    size_t operator()(const pair<T1,T2> &p) const { 
      return __fh(p.first) ^ __sh(p.second);
    }
  };
}

void test01()
{
  const int Size = 5;

  using std::string;
  using std::pair;
  using std::vector;
  using __gnu_cxx::hash_map;

  for (int i = 0; i < 10; i++)
  {
    hash_map<string, int> a;
    hash_map<string, int> b;
    
    vector<pair<string, int> > contents (Size);
    for (int j = 0; j < Size; j++)
    {
      string s;
      for (int k = 0; k < 10; k++)
      {
        s += 'a' + (rand() % 26);
      }
      contents[j] = make_pair(s,j);
    }
    for (int j = 0; j < Size; j++)
    {
      a[contents[j].first] = contents[j].second;
      int k = Size - 1 - j;
      b[contents[k].first] = contents[k].second;
    }
    VERIFY( a == b );
  }
}

int main()
{
  test01();
  return 0;
}
