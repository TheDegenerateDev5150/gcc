/* PR119692 "C++ 'typeinfo', 'vtable' vs. OpenACC, OpenMP 'target' offloading" */

/* { dg-additional-options -DDEFAULT=defaultmap(to) }
   Wrong code for offloading execution.
   { dg-xfail-run-if PR119692 { offload_device } } */
/* There are configurations where we 'WARNING: program timed out.' while in
   'dynamic_cast', see <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119692#c6>.
   { dg-timeout 10 { target offload_device } } ... to make sure that happens quickly.  */
/* { dg-additional-options -fdump-tree-gimple } */

#include "pr119692-1-1.C"

/* { dg-final { scan-tree-dump-not {(?n)#pragma omp target .* defaultmap\(to\) map\(to:_ZTI2C2 \[len: [0-9]+\] \[runtime_implicit\]\) map\(to:_ZTI2C1 \[len: [0-9]+\] \[runtime_implicit\]\) map\(to:_ZTV2C1 \[len: [0-9]+\] \[runtime_implicit\]\)$} gimple { xfail *-*-* } } } */
