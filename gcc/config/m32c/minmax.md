;; Machine Descriptions for R8C/M16C/M32C
;; Copyright (C) 2005-2025 Free Software Foundation, Inc.
;; Contributed by Red Hat.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.
;;
;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; min, max

(define_insn "sminqi3"
  [(set (match_operand:QI 0 "mra_operand" "=RhlSd,RhlSd,??Rmm,??Rmm,Raa,Raa")
	(smin:QI (match_operand:QI 1 "mra_operand" "%0,0,0,0,0,0")
		 (match_operand:QI 2 "mrai_operand" "iRhlSdRaa,?Rmm,iRhlSdRaa,?Rmm,iRhlSd,?Rmm")))]
  "TARGET_A24"
  "min.b\t%2,%0"
  [(set_attr "flags" "n")]
  )

(define_insn "sminhi3"
  [(set (match_operand:HI 0 "mra_operand" "=RhiSd,RhiSd,??Rmm,??Rmm")
	(smin:HI (match_operand:HI 1 "mra_operand" "%0,0,0,0")
		 (match_operand:HI 2 "mrai_operand" "iRhiSd,?Rmm,iRhiSd,?Rmm")))]
  "TARGET_A24"
  "min.w\t%2,%0"
  [(set_attr "flags" "n")]
  )

(define_insn "smaxqi3"
  [(set (match_operand:QI 0 "mra_operand" "=RhlSd,RhlSd,??Rmm,??Rmm,Raa,Raa")
	(smax:QI (match_operand:QI 1 "mra_operand" "%0,0,0,0,0,0")
		 (match_operand:QI 2 "mrai_operand" "iRhlSdRaa,?Rmm,iRhlSdRaa,?Rmm,iRhlSd,?Rmm")))]
  "TARGET_A24"
  "max.b\t%2,%0"
  [(set_attr "flags" "n")]
  )

(define_insn "smaxhi3"
  [(set (match_operand:HI 0 "mra_operand" "=RhiSd,RhiSd,??Rmm,??Rmm")
	(smax:HI (match_operand:HI 1 "mra_operand" "%0,0,0,0")
		 (match_operand:HI 2 "mrai_operand" "iRhiSd,?Rmm,iRhiSd,?Rmm")))]
  "TARGET_A24"
  "max.w\t%2,%0"
  [(set_attr "flags" "n")]
  )
