/* { dg-do assemble } */
/* { dg-options {-march-map=sm_103a -mptx=_} } */
/* { dg-additional-options -save-temps } */
/* { dg-final { scan-assembler-times {(?n)^	\.version	7\.8$} 1 } } */
/* { dg-final { scan-assembler-times {(?n)^	\.target	sm_89$} 1 } } */

#if __PTX_ISA_VERSION_MAJOR__ != 7
#error wrong value for __PTX_ISA_VERSION_MAJOR__
#endif

#if __PTX_ISA_VERSION_MINOR__ != 8
#error wrong value for __PTX_ISA_VERSION_MINOR__
#endif

#if __PTX_SM__ != 890
#error wrong value for __PTX_SM__
#endif

int dummy;
