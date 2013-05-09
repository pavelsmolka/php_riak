/*
   Copyright 2013 Trifork A/S
   Author: Kaspar Bach Pedersen

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
#include <php.h>
#include "bucket.h"
#include "bucket_properties.h"
#include "client.h"
#include "object.h"
#include "exceptions.h"
#include "php_riak.h"

#define GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR( VAR ) VAR = get_riack_client(getThis() TSRMLS_CC); \
  if (!VAR) { \
      zend_throw_exception(riak_not_found_exception_ce, "No client", 500 TSRMLS_CC); \
      return; \
  } 


zend_class_entry *riak_bucket_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_ctor, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, client)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_put, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_get, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_delete, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_fetchprops, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_applyprops, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, bucket_properties)
ZEND_END_ARG_INFO()

static zend_function_entry riak_bucket_methods[] = {
	PHP_ME(RiakBucket, __construct, arginfo_bucket_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(RiakBucket, putObject, arginfo_bucket_put, ZEND_ACC_PUBLIC)
	PHP_ME(RiakBucket, getObject, arginfo_bucket_get, ZEND_ACC_PUBLIC)
	PHP_ME(RiakBucket, deleteObject, arginfo_bucket_delete, ZEND_ACC_PUBLIC)

	PHP_ME(RiakBucket, fetchProperties, arginfo_bucket_fetchprops, ZEND_ACC_PUBLIC)
	PHP_ME(RiakBucket, applyProperties, arginfo_bucket_applyprops, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void riak_bucket_init(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "RiakBucket", riak_bucket_methods);
	riak_bucket_ce = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_null(riak_bucket_ce, "name", sizeof("name")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(riak_bucket_ce, "client", sizeof("client")-1, ZEND_ACC_PUBLIC TSRMLS_CC);

	zend_declare_property_null(riak_bucket_ce, "nVal", sizeof("nVal")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(riak_bucket_ce, "allowMult", sizeof("allowMult")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
}

/////////////////////////////////////////////////////////////

PHP_METHOD(RiakBucket, __construct)
{
	char *name;
	int nameLen;
	zval* client;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "os", &client, &name, &nameLen) == FAILURE) {
		return;
	}
	zend_update_property_stringl(riak_bucket_ce, getThis(), "name", sizeof("name")-1, name, nameLen TSRMLS_CC);
	zend_update_property(riak_bucket_ce, getThis(), "client", sizeof("client")-1, client TSRMLS_CC);
}

PHP_METHOD(RiakBucket, applyProperties)
{
	struct RIACK_CLIENT *client;
	RIACK_STRING bucketName;
	zval* zBucketPropsObj, **zTmp;
	HashTable *htBucketPropsProps;
	int riackResult;
	uint32_t nVal;
	uint8_t allowMult;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zBucketPropsObj) == FAILURE) {
		return;
	}
	GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR(client)
	bucketName = get_riack_bucket_name(getThis() TSRMLS_CC);
	htBucketPropsProps = zend_std_get_properties(zBucketPropsObj TSRMLS_CC);

	if ((zend_hash_find(htBucketPropsProps, "nVal", sizeof("nVal"), (void**)&zTmp) == SUCCESS)
     	&& (Z_TYPE_P(*zTmp) == IS_LONG)) {
		nVal = (uint32_t)Z_LVAL_PP(zTmp);
 	} else {
 		zend_throw_exception(riak_badarguments_exception_ce, "nVal must be integer", 5000 TSRMLS_CC);
 		return;
 	}
 	if ((zend_hash_find(htBucketPropsProps, "allowMult", sizeof("allowMult"), (void**)&zTmp) == SUCCESS)
     	&& (Z_TYPE_P(*zTmp) == IS_BOOL)) {
		allowMult = Z_BVAL_PP(zTmp);
 	} else {
 		zend_throw_exception(riak_badarguments_exception_ce, "allowMult must be boolean", 5001 TSRMLS_CC);
 		return;
 	}
 	riackResult = riack_set_bucket_props(client, bucketName, nVal, allowMult);
 	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(client, riackResult);
}

PHP_METHOD(RiakBucket, fetchProperties)
{
	struct RIACK_CLIENT *client;
	RIACK_STRING bucketName;
	uint32_t nVal = 3;
	uint8_t allowMult = 0;
	int riackResult;
	zval *zBucketProps, *zAllowMult, *zNVal;

	GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR(client)

	bucketName = get_riack_bucket_name(getThis() TSRMLS_CC);
	riackResult = riack_get_bucket_props(client,  bucketName, &nVal, &allowMult);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(client, riackResult);

	MAKE_STD_ZVAL(zNVal);
	ZVAL_LONG(zNVal, nVal);

	MAKE_STD_ZVAL(zAllowMult);
	ZVAL_BOOL(zAllowMult, allowMult);

	MAKE_STD_ZVAL(zBucketProps);
	object_init_ex(zBucketProps, riak_bucket_properties_ce);
	CALL_METHOD2(RiakBucketProperties, __construct, zBucketProps, zBucketProps, zNVal, zAllowMult);
	RETVAL_ZVAL(zBucketProps, 0, 1);

	zval_ptr_dtor(&zNVal);
	zval_ptr_dtor(&zAllowMult);
}

PHP_METHOD(RiakBucket, deleteObject)
{
	struct RIACK_DEL_PROPERTIES props;
	struct RIACK_CLIENT *client;
	zval *zObject, **zTmp;
	HashTable *htObjectProps;
	RIACK_STRING bucketName, key;
	int riackResult;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zObject) == FAILURE) {
		return;
	}
	htObjectProps = zend_std_get_properties(zObject TSRMLS_CC);

	GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR(client)

	// Set bucket name
	bucketName = get_riack_bucket_name(getThis() TSRMLS_CC);
	// Set key
	HASH_GET_INTO_RIACK_STRING_OR_ELSE(htObjectProps, "key", zTmp, key) {
		zend_throw_exception(riak_badarguments_exception_ce, "key missing from object", 5001 TSRMLS_CC);
 		return;
	}
	memset(&props, 0, sizeof(props));
	// TODO properties
	riackResult = riack_delete(client, bucketName, key, &props);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(client, riackResult);
}

PHP_METHOD(RiakBucket, putObject)
{
	char *key, *contentType;
	int keyLen, contentTypeLen;
	zval *zObject, **zTmp;
	HashTable *zObjectProps;
	struct RIACK_OBJECT obj, returnedObj;
	struct RIACK_CONTENT riackContent;
	struct RIACK_PUT_PROPERTIES props;
	struct RIACK_CLIENT *client;
	int riackResult;
	
	key = NULL;
	keyLen = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o|s", &zObject, &key, &keyLen) == FAILURE) {
		return;
	}
	zObjectProps = zend_std_get_properties(zObject TSRMLS_CC);

	GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR(client)

	memset(&obj, 0, sizeof(obj));
	memset(&returnedObj, 0, sizeof(returnedObj));
	memset(&riackContent, 0, sizeof(riackContent));
	memset(&props, 0, sizeof(props));
	//// fill content ////

	// Set content type
	HASH_GET_INTO_RIACK_STRING_OR_ELSE(zObjectProps, "contentType", zTmp, riackContent.content_type) ;
	// Set content encoding
	HASH_GET_INTO_RIACK_STRING_OR_ELSE(zObjectProps, "contentEncoding", zTmp, riackContent.content_encoding) ;

	// Set data
	if (zend_hash_find(zObjectProps, "data", sizeof("data"), (void**)&zTmp) == SUCCESS) {
		if (Z_TYPE_P(*zTmp) == IS_STRING) {
			riackContent.data_len = Z_STRLEN_P(*zTmp);
			riackContent.data = (uint8_t*)Z_STRVAL_P(*zTmp);
		}
	}

	//// fill obj ////
	// Set vclock
	if (zend_hash_find(zObjectProps, "vclock", sizeof("vclock"), (void**)&zTmp) == SUCCESS) {
		if (Z_TYPE_P(*zTmp) == IS_STRING) {
			obj.vclock.len = Z_STRLEN_P(*zTmp);
			obj.vclock.clock = (uint8_t*)Z_STRVAL_P(*zTmp);
		}
	}
	// Set bucket name
	obj.bucket = get_riack_bucket_name(getThis() TSRMLS_CC);
	obj.content_count = 1;
	obj.content = &riackContent;

	if (key) {
		// Key was passed to function use it.
		obj.key.len = keyLen;
		obj.key.value = key;
	} else {
		// No ket provided on function call, get it from RiakObject
		HASH_GET_INTO_RIACK_STRING_OR_ELSE(zObjectProps, "key", zTmp, obj.key) {
			// Handle possible errors
		}
	}

	riackResult = riack_put(client, obj, &returnedObj, &props);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(client, riackResult);

	// TODO Return updated object
	riack_free_object(client, &returnedObj);
}

PHP_METHOD(RiakBucket, getObject)
{
	char *key;
	int i, keyLen, riackResult;
	size_t contentCount;
	zval *zBucket, *zKey, *zVclock, *zExc, **zTmp, *zObjArr, *zObj;
	struct RIACK_GET_PROPERTIES props;
	struct RIACK_GET_OBJECT getResult;
	RIACK_STRING rsBucket, rsKey;
	struct RIACK_CLIENT* client;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &keyLen) == FAILURE) {
		return;
	}
	
	GET_RIACK_CLIENT_RETURN_EXC_ON_ERROR(client)

	MAKE_STD_ZVAL(zKey);
	ZVAL_STRINGL(zKey, key, keyLen, 1);

	memset(&props, 0, sizeof(props));
	memset(&getResult, 0, sizeof(getResult));
	rsBucket = get_riack_bucket_name(getThis() TSRMLS_CC);

	rsKey.len = keyLen;
	rsKey.value = key;
	riackResult = riack_get(client, rsBucket, rsKey, &props, &getResult);

	if (riackResult == RIACK_SUCCESS) {
		MAKE_STD_ZVAL(zVclock);
		ZVAL_STRINGL(zVclock, (char*)getResult.object.vclock.clock, getResult.object.vclock.len, 1);
		contentCount = getResult.object.content_count;
		if (contentCount > 1) {
			MAKE_STD_ZVAL(zObjArr);
			MAKE_STD_ZVAL(zExc);
			
			array_init(zObjArr);
			while (contentCount > 0) {
				zObj = object_from_riak_content(zKey, &getResult.object.content[--contentCount] TSRMLS_CC);
				add_next_index_zval(zObjArr, zObj);
			}
			object_init_ex(zExc, riak_conflicted_object_exception_ce);
			CALL_METHOD2(RiakConflictedObjectException, __construct, zExc, zExc, zVclock, zObjArr);
			zend_throw_exception_object(zExc TSRMLS_CC);
			
			zval_ptr_dtor(&zObjArr);
		} else if (contentCount == 1) {
			// Create a new RiakObject
			zObj = object_from_riak_content(zKey, &getResult.object.content[0] TSRMLS_CC);
			// Update vclock on RiakObject
			zend_update_property(riak_object_ce, zObj, "vclock", sizeof("vclock")-1, zVclock TSRMLS_CC);
			RETVAL_ZVAL(zObj, 0, 1);
		} else {
			// Throw not found exception
			zend_throw_exception(riak_not_found_exception_ce, "Not Found", 2000 TSRMLS_CC);
		}
		zval_ptr_dtor(&zVclock);
		riack_free_get_object(client, &getResult);
	} else {
		zval_ptr_dtor(&zKey);
		CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(client, riackResult);
	}
	zval_ptr_dtor(&zKey);
}

///////////////////////////////////////////////////////////////////////////////

zval* object_from_riak_content(zval* key, struct RIACK_CONTENT* content TSRMLS_DC)
{
	zval *object;
	MAKE_STD_ZVAL(object);
	object_init_ex(object, riak_object_ce);
	CALL_METHOD1(RiakObject, __construct, object, object, key);
	set_object_from_riak_content(object, content TSRMLS_CC);
	return object;
}

RIACK_STRING get_riack_bucket_name(zval* bucket TSRMLS_DC)
{
    zval **zTmp;
    RIACK_STRING bucketName;
    HashTable *zBucketProps;
    zBucketProps = zend_std_get_properties(bucket TSRMLS_CC);
    HASH_GET_INTO_RIACK_STRING_OR_ELSE(zBucketProps, "name", zTmp, bucketName) ;
    return bucketName;

}

struct RIACK_CLIENT* get_riack_client(zval *zbucket TSRMLS_DC)
{
	zval **zTmp;
	HashTable *zBucketProps;
	struct RIACK_CLIENT *client;
	zBucketProps = zend_std_get_properties(zbucket TSRMLS_CC);
	if (zend_hash_find(zBucketProps, "client", sizeof("client"), (void**)&zTmp) == SUCCESS) {
		GET_RIACK_CLIENT(*zTmp, client);
	} else {
		return NULL;
	}
	return client;
}