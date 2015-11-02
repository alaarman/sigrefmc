/*
 * Copyright 2011-2015 Formal Methods and Tools, University of Twente
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * This is an implementation of GMP mpq custom leaves of MTBDDs
 */

#ifndef SYLVAN_GMP_H
#define SYLVAN_GMP_H

#include <sylvan.h>
#include <gmp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initialize GMP custom leaves
 */
void gmp_init();

/**
 * Create MPQ leaf
 */
MTBDD mtbdd_gmp(mpq_t val);

/**
 * Operation "plus" for two mpq MTBDDs
 */
TASK_DECL_2(MTBDD, gmp_op_plus, MTBDD*, MTBDD*);
TASK_DECL_3(MTBDD, gmp_abstract_op_plus, MTBDD, MTBDD, int);

/**
 * Operation "times" for two mpq MTBDDs
 */
TASK_DECL_2(MTBDD, gmp_op_times, MTBDD*, MTBDD*);
TASK_DECL_3(MTBDD, gmp_abstract_op_times, MTBDD, MTBDD, int);

/**
 * Operation "min" for two mpq MTBDDs
 */
TASK_DECL_2(MTBDD, gmp_op_min, MTBDD*, MTBDD*);
TASK_DECL_3(MTBDD, gmp_abstract_op_min, MTBDD, MTBDD, int);

/**
 * Operation "max" for two mpq MTBDDs
 */
TASK_DECL_2(MTBDD, gmp_op_max, MTBDD*, MTBDD*);
TASK_DECL_3(MTBDD, gmp_abstract_op_max, MTBDD, MTBDD, int);

/**
 * Compute a + b
 */
#define gmp_plus(a, b) mtbdd_apply(a, b, TASK(gmp_op_plus))

/**
 * Compute a * b
 */
#define gmp_times(a, b) mtbdd_apply(a, b, TASK(gmp_op_times))

/**
 * Compute min(a, b)
 */
#define gmp_min(a, b) mtbdd_apply(a, b, TASK(gmp_op_min))

/**
 * Compute max(a, b)
 */
#define gmp_max(a, b) mtbdd_apply(a, b, TASK(gmp_op_max))

/**
 * Abstract the variables in <v> from <a> by taking the sum of all values
 */
#define gmp_abstract_plus(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_plus))

/**
 * Abstract the variables in <v> from <a> by taking the product of all values
 */
#define gmp_abstract_times(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_times))

/**
 * Abstract the variables in <v> from <a> by taking the minimum of all values
 */
#define gmp_abstract_min(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_min))

/**
 * Abstract the variables in <v> from <a> by taking the maximum of all values
 */
#define gmp_abstract_max(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_max))

/**
 * Multiply <a> and <b>, and abstract variables <vars> using summation.
 * This is similar to the "and_exists" operation in BDDs.
 */
TASK_DECL_3(MTBDD, gmp_and_exists, MTBDD, MTBDD, MTBDD);
#define gmp_and_exists(a, b, vars) CALL(gmp_and_exists, a, b, vars)

/**
 * Convert to a Boolean MTBDD, translate terminals >= value to 1 and to 0 otherwise;
 */
TASK_DECL_2(MTBDD, gmp_threshold_d, MTBDD, double);
#define gmp_threshold_d(dd, value) CALL(gmp_threshold_d, dd, value)

/**
 * Convert to a Boolean MTBDD, translate terminals > value to 1 and to 0 otherwise;
 */
TASK_DECL_2(MTBDD, gmp_strict_threshold_d, MTBDD, double);
#define gmp_strict_threshold_d(dd, value) CALL(gmp_strict_threshold_d, dd, value)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
