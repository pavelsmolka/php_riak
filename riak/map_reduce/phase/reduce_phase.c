/*
   Copyright 2013: Kaspar Bach Pedersen

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "reduce_phase.h"
#include "phase.h"

zend_class_entry *riak_mr_reducephase_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_map_red_phase_reduce_phase_toarr, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_map_red_phase_reduce_phase_ctor, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, function)
    ZEND_ARG_INFO(0, keep)
    ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()


static zend_function_entry riak_mrphase_reduce_methods[] = {
    PHP_ME(Riak_MapReduce_Phase_ReducePhase, __construct, arginfo_map_red_phase_reduce_phase_ctor, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Riak_MapReduce_Phase_ReducePhase, toArray, arginfo_map_red_phase_reduce_phase_toarr, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};


void riak_map_reduce_phase_reduce_phase_init(TSRMLS_D)/* {{{ */
{
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Riak\\MapReduce\\Phase", "ReducePhase", riak_mrphase_reduce_methods);
    riak_mr_reducephase_ce = zend_register_internal_class_ex(&ce, riak_mrphase_ce, NULL TSRMLS_CC);
}
/* }}} */


/*************************************************************
* Implementation: Riak\MapReduce\Phase\ReducePhase
*************************************************************/

/* {{{ proto void Riak\MapReduce\Phase\ReducePhase->__construct(Riak\MapReduce\Function\Function $function [, bool $keep [, array $arguments]])
Create a new ReducePhase */
PHP_METHOD(Riak_MapReduce_Phase_ReducePhase, __construct)
{
    zval *zfunction, *zargs;
    zend_bool keep;
    keep = 0;
    zargs = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o|ba", &zfunction, &keep, &zargs) == FAILURE) {
        return;
    }
    zend_update_property(riak_mr_reducephase_ce, getThis(), "function", sizeof("function")-1, zfunction TSRMLS_CC);
    zend_update_property_bool(riak_mr_reducephase_ce, getThis(), "keep", sizeof("keep")-1, keep TSRMLS_CC);
    if (zargs) {
        zend_update_property(riak_mr_reducephase_ce, getThis(), "arg", sizeof("arg")-1, zargs TSRMLS_CC);
    }
}
/* }}} */

/* {{{ proto array Riak\MapReduce\Phase\ReducePhase->toArray()
Convert this phase into an array */
PHP_METHOD(Riak_MapReduce_Phase_ReducePhase, toArray)
{
    zval *zarray, *zfuncarray, *zfunc, *zarg, zname;
    zend_bool keep;
    MAKE_STD_ZVAL(zarray);
    array_init(zarray);

    MAKE_STD_ZVAL(zfuncarray);
    zfunc = zend_read_property(riak_mr_reducephase_ce, getThis(), "function", sizeof("function")-1, 1 TSRMLS_CC);
    ZVAL_STRING(&zname, "toArray", 0);
    call_user_function(NULL, &zfunc, &zname, zfuncarray, 0, NULL TSRMLS_CC);

    keep = Z_BVAL_P(zend_read_property(riak_mr_reducephase_ce, getThis(), "keep", sizeof("keep")-1, 1 TSRMLS_CC));
    if (keep) {
        add_assoc_bool_ex(zfuncarray, "keep", sizeof("keep"), 1);
    }
    add_assoc_zval_ex(zarray, "reduce", sizeof("reduce"), zfuncarray);
    zarg = zend_read_property(riak_mr_reducephase_ce, getThis(), "arg", sizeof("arg")-1, 1 TSRMLS_CC);
    if (Z_TYPE_P(zarg) != IS_NULL) {
        zval_addref_p(zarg);
        add_assoc_zval_ex(zarray, "arg", sizeof("arg"), zarg);
    }
    RETURN_ZVAL(zarray, 0, 1);
}
/* }}} */

