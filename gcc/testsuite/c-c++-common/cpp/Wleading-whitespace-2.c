/* { dg-do compile { target { c || c++11 } } } */
/* { dg-options "-Wleading-whitespace=spaces" } */

    int i1; /* 4 spaces ok for */
		       int i2; /* 2 tabs 7 spaces not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
        int i3; /* 8 spaces ok */
	        int i4; /* tab 8 spaces not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
    	int i5; /* 4 spaces tab not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
 	int i6; /* space tab not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
	 int i7; /* tab vtab not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
	 int i8; /* tab form-feed not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
     int i9; /* 4 spaces vtab not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
   int i10; /* 2 spaces form-feed not ok */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-1 } */
	        	
/* Just whitespace on a line is something for -Wtrailing-whitespace.  */
int \
	i11, \
    i12, \
		        i13, \
        i14, \
 	i15, \
		i16, \
		i17;
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-6 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-5 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-5 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-5 } */
          const char *p = R"*|*(		
    a
	b
	    c
          d
 	e
		 f
  g
)*|*";
/* This is a comment with leading whitespace non-issues and issues
		a
      b
	    c
	        d
 	e
	 f
	 g
*/
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-8 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
/* { dg-warning "whitespace other than spaces in leading whitespace" "" { target *-*-* } .-7 } */
