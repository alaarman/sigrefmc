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

#include <sylvan_config.h>

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sylvan.h>
#include <sylvan_common.h>
#include <sylvan_mtbdd_int.h>
#include <sylvan_gmp.h>
#include <gmp.h>

/**
 * hash16
 */
#ifndef rotl64
static inline uint64_t
rotl64(uint64_t x, int8_t r)
{
    return ((x<<r) | (x>>(64-r)));
}
#endif

static uint64_t
gmp_hash(const uint64_t v, const uint64_t seed)
{
    /* Hash the mpq in pointer v 
     * A simpler way would be to hash the result of mpq_get_d.
     * We just hash on the contents of the memory */
    
    mpq_ptr x = (mpq_ptr)(size_t)v;

    const uint64_t prime = 1099511628211;
    uint64_t hash = seed;
    mp_limb_t *limbs;

    // hash "numerator" limbs
    limbs = x[0]._mp_num._mp_d;
    for (int i=0; i<x[0]._mp_num._mp_size; i++) {
        hash = hash ^ limbs[i];
        hash = rotl64(hash, 47);
        hash = hash * prime;
    }

    // hash "denominator" limbs
    limbs = x[0]._mp_den._mp_d;
    for (int i=0; i<x[0]._mp_den._mp_size; i++) {
        hash = hash ^ limbs[i];
        hash = rotl64(hash, 31);
        hash = hash * prime;
    }

    return hash ^ (hash >> 32);
}

static int
gmp_equals(const uint64_t left, const uint64_t right)
{
    mpq_ptr x = (mpq_ptr)(size_t)left;
    mpq_ptr y = (mpq_ptr)(size_t)right;

    /* Just compare x and y */
    return mpq_equal(x, y) ? 1 : 0;
}

static void
gmp_create(uint64_t *val)
{
    /* Make a copy */
    mpq_ptr x = (mpq_ptr)malloc(sizeof(__mpq_struct));
    mpq_init(x);
    mpq_set(x, *(mpq_ptr*)val);
    *(mpq_ptr*)val = x;
}

static void
gmp_destroy(uint64_t val)
{
    mpq_clear((mpq_ptr)val);
    free((void*)val);
}

/**
 * Initialize gmp custom leaves
 */
void
gmp_init()
{
    /* Register custom leaf 3 */
    mtbdd_register_custom_leaf(3, gmp_hash, gmp_equals, gmp_create, gmp_destroy);
}

/**
 * Create GMP mpq leaf
 */
MTBDD
mtbdd_gmp(mpq_t val)
{
    mpq_canonicalize(val);
    return mtbdd_makeleaf(3, (size_t)val);
}

/**
 * Operation "plus" for two mpq MTBDDs
 */
TASK_IMPL_2(MTBDD, gmp_op_plus, MTBDD*, pa, MTBDD*, pb)
{
    MTBDD a = *pa, b = *pb;

    /* Check for partial functions */
    if (a == mtbdd_false) return b;
    if (b == mtbdd_false) return a;

    /* If both leafs, compute plus */
    if (mtbdd_isleaf(a) && mtbdd_isleaf(b)) {
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        mpq_ptr mb = (mpq_ptr)mtbdd_getvalue(b);

        mpq_t mres;
        mpq_init(mres);
        mpq_add(mres, ma, mb);
        MTBDD res = mtbdd_gmp(mres);
        mpq_clear(mres);
        return res;
    }

    /* Communitative, so swap a,b for better cache performance */
    if (a < b) {
        *pa = b;
        *pb = a;
    }

    return mtbdd_invalid;
}

/**
 * Operation "times" for two mpq MTBDDs.
 * One of the parameters can be a BDD, then it is interpreted as a filter.
 */
TASK_IMPL_2(MTBDD, gmp_op_times, MTBDD*, pa, MTBDD*, pb)
{
    MTBDD a = *pa, b = *pb;

    /* Check for partial functions and for Boolean (filter) */
    if (a == mtbdd_false || b == mtbdd_false) return mtbdd_false;

    /* If one of Boolean, interpret as filter */
    if (a == mtbdd_true) return b;
    if (b == mtbdd_true) return a;

    /* Handle multiplication of leafs */
    if (mtbdd_isleaf(a) && mtbdd_isleaf(b)) {
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        mpq_ptr mb = (mpq_ptr)mtbdd_getvalue(b);

        // compute result
        mpq_t mres;
        mpq_init(mres);
        mpq_mul(mres, ma, mb);
        MTBDD res = mtbdd_gmp(mres);
        mpq_clear(mres);
        return res;
    }

    /* Commutative, so make "a" the lowest for better cache performance */
    if (a < b) {
        *pa = b;
        *pb = a;
    }

    return mtbdd_invalid;
}

/**
 * Operation "min" for two mpq MTBDDs.
 */
TASK_IMPL_2(MTBDD, gmp_op_min, MTBDD*, pa, MTBDD*, pb)
{
    MTBDD a = *pa, b = *pb;

    /* Handle partial functions */
    if (a == mtbdd_false) return b;
    if (b == mtbdd_false) return a;

    /* Handle trivial case */
    if (a == b) return a;

    /* Compute result for leaves */
    if (mtbdd_isleaf(a) && mtbdd_isleaf(b)) {
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        mpq_ptr mb = (mpq_ptr)mtbdd_getvalue(b);
        int cmp = mpq_cmp(ma, mb);
        return cmp < 0 ? a : b;
    }

    /* For cache performance */
    if (a < b) {
        *pa = b;
        *pb = a;
    }

    return mtbdd_invalid;
}

/**
 * Operation "max" for two mpq MTBDDs.
 */
TASK_IMPL_2(MTBDD, gmp_op_max, MTBDD*, pa, MTBDD*, pb)
{
    MTBDD a = *pa, b = *pb;

    /* Handle partial functions */
    if (a == mtbdd_false) return b;
    if (b == mtbdd_false) return a;

    /* Handle trivial case */
    if (a == b) return a;

    /* Compute result for leaves */
    if (mtbdd_isleaf(a) && mtbdd_isleaf(b)) {
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        mpq_ptr mb = (mpq_ptr)mtbdd_getvalue(b);
        int cmp = mpq_cmp(ma, mb);
        return cmp > 0 ? a : b;
    }

    /* For cache performance */
    if (a < b) {
        *pa = b;
        *pb = a;
    }

    return mtbdd_invalid;
}

TASK_IMPL_3(MTBDD, gmp_abstract_op_plus, MTBDD, a, MTBDD, b, int, k)
{
    if (k==0) {
        return mtbdd_apply(a, b, TASK(gmp_op_plus));
    } else {
        MTBDD res = a;
        for (int i=0; i<k; i++) {
            mtbdd_refs_push(res);
            res = mtbdd_apply(res, res, TASK(gmp_op_plus));
            mtbdd_refs_pop(1);
        }
        return res;
    }
}

TASK_IMPL_3(MTBDD, gmp_abstract_op_times, MTBDD, a, MTBDD, b, int, k)
{
    if (k==0) {
        return mtbdd_apply(a, b, TASK(gmp_op_times));
    } else {
        MTBDD res = a;
        for (int i=0; i<k; i++) {
            mtbdd_refs_push(res);
            res = mtbdd_apply(res, res, TASK(gmp_op_times));
            mtbdd_refs_pop(1);
        }
        return res;
    }
}

TASK_IMPL_3(MTBDD, gmp_abstract_op_min, MTBDD, a, MTBDD, b, int, k)
{
    return k == 0 ? mtbdd_apply(a, b, TASK(gmp_op_min)) : a;
}

TASK_IMPL_3(MTBDD, gmp_abstract_op_max, MTBDD, a, MTBDD, b, int, k)
{
    return k == 0 ? mtbdd_apply(a, b, TASK(gmp_op_max)) : a;
}

/**
 * Convert to Boolean MTBDD, terminals >= value (double) to True, or False otherwise.
 */
TASK_2(MTBDD, gmp_op_threshold_d, MTBDD, a, size_t, svalue)
{
    /* Handle partial function */
    if (a == mtbdd_false) return mtbdd_false;

    /* Compute result */
    if (mtbdd_isleaf(a)) {
        double value = *(double*)&svalue;
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        return mpq_get_d(ma) >= value ? mtbdd_true : mtbdd_false;
    }

    return mtbdd_invalid;
}

/**
 * Convert to Boolean MTBDD, terminals > value (double) to True, or False otherwise.
 */
TASK_2(MTBDD, gmp_op_strict_threshold_d, MTBDD, a, size_t, svalue)
{
    /* Handle partial function */
    if (a == mtbdd_false) return mtbdd_false;

    /* Compute result */
    if (mtbdd_isleaf(a)) {
        double value = *(double*)&svalue;
        mpq_ptr ma = (mpq_ptr)mtbdd_getvalue(a);
        return mpq_get_d(ma) > value ? mtbdd_true : mtbdd_false;
    }

    return mtbdd_invalid;
}

TASK_IMPL_2(MTBDD, gmp_threshold_d, MTBDD, dd, double, d)
{
    return mtbdd_uapply(dd, TASK(gmp_op_threshold_d), *(size_t*)&d);
}

TASK_IMPL_2(MTBDD, gmp_strict_threshold_d, MTBDD, dd, double, d)
{
    return mtbdd_uapply(dd, TASK(gmp_op_strict_threshold_d), *(size_t*)&d);
}

/**
 * Multiply <a> and <b>, and abstract variables <vars> using summation.
 * This is similar to the "and_exists" operation in BDDs.
 */
TASK_IMPL_3(MTBDD, gmp_and_exists, MTBDD, a, MTBDD, b, MTBDD, v)
{
    /* Check terminal case */
    if (v == mtbdd_true) return mtbdd_apply(a, b, TASK(gmp_op_times));
    MTBDD result = CALL(gmp_op_times, &a, &b);
    if (result != mtbdd_invalid) {
        mtbdd_refs_push(result);
        result = mtbdd_abstract(result, v, TASK(gmp_abstract_op_plus));
        mtbdd_refs_pop(1);
        return result;
    }

    /* Maybe perform garbage collection */
    sylvan_gc_test();

    /* Check cache */
    if (cache_get(a | CACHE_MTBDD_AND_EXISTS, b, v, &result)) return result;

    /* Now, v is not a constant, and either a or b is not a constant */

    /* Get top variable */
    int la = mtbdd_isleaf(a);
    int lb = mtbdd_isleaf(b);
    mtbddnode_t na = la ? 0 : GETNODE(a);
    mtbddnode_t nb = lb ? 0 : GETNODE(b);
    uint32_t va = la ? 0xffffffff : mtbddnode_getvariable(na);
    uint32_t vb = lb ? 0xffffffff : mtbddnode_getvariable(nb);
    uint32_t var = va < vb ? va : vb;

    mtbddnode_t nv = GETNODE(v);
    uint32_t vv = mtbddnode_getvariable(nv);

    if (vv < var) {
        /* Recursive, then abstract result */
        result = CALL(gmp_and_exists, a, b, node_gethigh(v, nv));
        mtbdd_refs_push(result);
        result = mtbdd_apply(result, result, TASK(gmp_op_plus));
        mtbdd_refs_pop(1);
    } else {
        /* Get cofactors */
        MTBDD alow, ahigh, blow, bhigh;
        alow  = (!la && va == var) ? node_getlow(a, na)  : a;
        ahigh = (!la && va == var) ? node_gethigh(a, na) : a;
        blow  = (!lb && vb == var) ? node_getlow(b, nb)  : b;
        bhigh = (!lb && vb == var) ? node_gethigh(b, nb) : b;

        if (vv == var) {
            /* Recursive, then abstract result */
            mtbdd_refs_spawn(SPAWN(gmp_and_exists, ahigh, bhigh, node_gethigh(v, nv)));
            MTBDD low = mtbdd_refs_push(CALL(gmp_and_exists, alow, blow, node_gethigh(v, nv)));
            MTBDD high = mtbdd_refs_push(mtbdd_refs_sync(SYNC(gmp_and_exists)));
            result = CALL(mtbdd_apply, low, high, TASK(gmp_op_plus));
            mtbdd_refs_pop(2);
        } else /* vv > v */ {
            /* Recursive, then create node */
            mtbdd_refs_spawn(SPAWN(gmp_and_exists, ahigh, bhigh, v));
            MTBDD low = mtbdd_refs_push(CALL(gmp_and_exists, alow, blow, v));
            MTBDD high = mtbdd_refs_sync(SYNC(gmp_and_exists));
            mtbdd_refs_pop(1);
            result = mtbdd_makenode(var, low, high);
        }
    }

    /* Store in cache */
    cache_put(a | CACHE_MTBDD_AND_EXISTS, b, v, result);
    return result;
}
