// 1999-06-03 bkoz

// Copyright (C) 1999-2025 Free Software Foundation, Inc.
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

// 21.3.5.4 basic_string::insert

#include <string>
#include <stdexcept>
#include <testsuite_hooks.h>

void test01(void)
{
  typedef std::wstring::size_type csize_type;
  typedef std::wstring::iterator citerator;
  csize_type csz01, csz02;

  const std::wstring str01(L"rodeo beach, marin");
  const std::wstring str02(L"baker beach, san francisco");
  std::wstring str03;

  // wstring& insert(size_type p1, const wstring& str, size_type p2, size_type n)
  // requires:
  //   1) p1 <= size()
  //   2) p2 <= str.size()
  //   3) rlen = min(n, str.size() - p2)
  // throws:
  //   1) out_of_range if p1 > size() || p2 > str.size()
  //   2) length_error if size() >= npos - rlen
  // effects:
  // replaces *this with new wstring of length size() + rlen such that
  // nstr[0]  to nstr[p1] == thisstr[0] to thisstr[p1]
  // nstr[p1 + 1] to nstr[p1 + rlen] == str[p2] to str[p2 + rlen]
  // nstr[p1 + 1 + rlen] to nstr[...] == thisstr[p1 + 1] to thisstr[...]  
  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  try {
    str03.insert(csz01 + 1, str02, 0, 5);
    VERIFY( false );
  }		 
  catch(std::out_of_range& fail) {
    VERIFY( true );
  }
  catch(...) {
    VERIFY( false );
  }

  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  try {
    str03.insert(0, str02, csz02 + 1, 5);
    VERIFY( false );
  }		 
  catch(std::out_of_range& fail) {
    VERIFY( true );
  }
  catch(...) {
    VERIFY( false );
  }

  csz01 = str01.max_size();
  try {
    std::wstring str04(csz01, L'b'); 
    str03 = str04; 
    csz02 = str02.size();
    try {
      str03.insert(0, str02, 0, 5);
      VERIFY( false );
    }		 
    catch(std::length_error& fail) {
      VERIFY( true );
    }
    catch(...) {
      VERIFY( false );
    }
  }
  catch(std::bad_alloc& failure){
    VERIFY( true ); 
  }
  catch(std::exception& failure){
    VERIFY( false );
  }

  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  str03.insert(13, str02, 0, 12); 
  VERIFY( str03 == L"rodeo beach, baker beach,marin" );

  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  str03.insert(0, str02, 0, 12); 
  VERIFY( str03 == L"baker beach,rodeo beach, marin" );

  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  str03.insert(csz01, str02, 0, csz02); 
  VERIFY( str03 == L"rodeo beach, marinbaker beach, san francisco" );

  // wstring& insert(size_type __p, const wstring& wstr);
  // insert(p1, str, 0, npos)
  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  str03.insert(csz01, str02); 
  VERIFY( str03 == L"rodeo beach, marinbaker beach, san francisco" );

  str03 = str01; 
  csz01 = str03.size();
  csz02 = str02.size();
  str03.insert(0, str02); 
  VERIFY( str03 == L"baker beach, san franciscorodeo beach, marin" );

  // wstring& insert(size_type __p, const wchar_t* s, size_type n);
  // insert(p1, wstring(s,n))
  str03 = str02; 
  csz01 = str03.size();
  str03.insert(0, L"-break at the bridge", 20); 
  VERIFY( str03 == L"-break at the bridgebaker beach, san francisco" );

  // wstring& insert(size_type __p, const wchar_t* s);
  // insert(p1, wstring(s))
  str03 = str02; 
  str03.insert(0, L"-break at the bridge"); 
  VERIFY( str03 == L"-break at the bridgebaker beach, san francisco" );

  // wstring& insert(size_type __p, size_type n, wchar_t c)
  // insert(p1, wstring(n,c))
  str03 = str02; 
  csz01 = str03.size();
  str03.insert(csz01, 5, L'z'); 
  VERIFY( str03 == L"baker beach, san franciscozzzzz" );

  // iterator insert(iterator p, wchar_t c)
  // inserts a copy of c before the character referred to by p
  str03 = str02; 
  citerator cit01 = str03.begin();
  str03.insert(cit01, L'u'); 
  VERIFY( str03 == L"ubaker beach, san francisco" );

  // iterator insert(iterator p, size_type n,  wchar_t c)
  // inserts n copies of c before the character referred to by p
  str03 = str02; 
  cit01 = str03.begin();
  str03.insert(cit01, 5, L'u'); 
  VERIFY( str03 == L"uuuuubaker beach, san francisco" );

  // template<inputit>
  //   void 
  //   insert(iterator p, inputit first, inputit, last)
  // ISO-14882: defect #7 part 1 clarifies this member function to be:
  // insert(p - begin(), wstring(first,last))
  str03 = str02; 
  csz01 = str03.size();
  str03.insert(str03.begin(), str01.begin(), str01.end()); 
  VERIFY( str03 == L"rodeo beach, marinbaker beach, san francisco" );

  str03 = str02; 
  csz01 = str03.size();
  str03.insert(str03.end(), str01.begin(), str01.end()); 
  VERIFY( str03 == L"baker beach, san franciscorodeo beach, marin" );
}

int main()
{ 
  __gnu_test::set_memory_limits();
  test01();
  return 0;
}
