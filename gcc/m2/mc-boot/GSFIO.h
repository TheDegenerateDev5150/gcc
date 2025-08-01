/* do not edit automatically generated by mc from SFIO.  */
/* SFIO.def provides a String interface to the opening routines of FIO.

Copyright (C) 2001-2025 Free Software Foundation, Inc.
Contributed by Gaius Mulley <gaius.mulley@southwales.ac.uk>.

This file is part of GNU Modula-2.

GNU Modula-2 is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GNU Modula-2 is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


#if !defined (_SFIO_H)
#   define _SFIO_H

#include "config.h"
#include "system.h"
#   ifdef __cplusplus
extern "C" {
#   endif
#include <stdbool.h>
#   if !defined (PROC_D)
#      define PROC_D
       typedef void (*PROC_t) (void);
       typedef struct { PROC_t proc; } PROC;
#   endif

#   include "GDynamicStrings.h"
#   include "GFIO.h"

#   if defined (_SFIO_C)
#      define EXTERN
#   else
#      define EXTERN extern
#   endif


/*
   Exists - returns TRUE if a file named, fname exists for reading.
*/

EXTERN bool SFIO_Exists (DynamicStrings_String fname);

/*
   OpenToRead - attempts to open a file, fname, for reading and
                it returns this file.
                The success of this operation can be checked by
                calling IsNoError.
*/

EXTERN FIO_File SFIO_OpenToRead (DynamicStrings_String fname);

/*
   OpenToWrite - attempts to open a file, fname, for write and
                 it returns this file.
                 The success of this operation can be checked by
                 calling IsNoError.
*/

EXTERN FIO_File SFIO_OpenToWrite (DynamicStrings_String fname);

/*
   OpenForRandom - attempts to open a file, fname, for random access
                   read or write and it returns this file.
                   The success of this operation can be checked by
                   calling IsNoError.
                   towrite, determines whether the file should be
                   opened for writing or reading.
                   if towrite is TRUE or whether the previous file should
                   be left alone, allowing this descriptor to seek
                   and modify an existing file.
*/

EXTERN FIO_File SFIO_OpenForRandom (DynamicStrings_String fname, bool towrite, bool newfile);

/*
   WriteS - writes a string, s, to, file. It returns the String, s.
*/

EXTERN DynamicStrings_String SFIO_WriteS (FIO_File file, DynamicStrings_String s);

/*
   ReadS - reads a string, s, from, file. It returns the String, s.
           It stops reading the string at the end of line or end of file.
           It consumes the newline at the end of line but does not place
           this into the returned string.
*/

EXTERN DynamicStrings_String SFIO_ReadS (FIO_File file);

/*
   GetFileName - return a new string containing the name of the file.
                 The string should be killed by the caller.
*/

EXTERN DynamicStrings_String SFIO_GetFileName (FIO_File file);
#   ifdef __cplusplus
}
#   endif

#   undef EXTERN
#endif
