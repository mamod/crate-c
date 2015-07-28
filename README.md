# Crate IO C client

This is a [crate io database](https://crate.io) **C client**, it uses [duktape](https://duktape.org) javascript embeddable C engine internally to parse and fetch json data


````C
crate_db *crate = crate_init("options.js");
if (crate == NULL){
    printf("out of memory\n");
    exit(0);
}

int ret = crate_execute(crate, "drop table my_table");
if (ret == -1){
    //error
} else {
    printf("success: affected rows %i\n", ret);
}

crate_close(crate);
````

Methods
=======

**``crate_db *crate_init("options.js");``**

returns crate_db structure, options.js is the full path to crate database options javascript file. This should be called, properly only once, to initiate crate database

**``int crate_execute(crate_db *crate, const char *query);``**

executes query string and returns number of affected rows or 1 on success, on failure returns -1

**``const char *crate_error_message(crate_db *crate);``**

Returns error message in case of error, calling this on a successful operation may return previous error message, so you need to check if there is an error before calling this method.

**``int *crate_error_code(crate_db *crate);``**

Returns error code number in case of error, calling this on a successful operation may return previous error code, so you need to check if there is an error before calling this method.

**``const char *crate_get_string(crate_db *crate, int row, const char *field);``**

Statement Methods
===================

**``crate_stmt *stmt = crate_stmt_init(crate_db *crate);``**

Initiatt a new statement

**``int crate_stmt_prepare(crate_stmt *stmt, const char *query)``**

Prepare sql query for execution

**``int crate_stmt_bind_int(crate_stmt *stmt, int val)``**

bind int value to the statement

**``int crate_stmt_bind_number(crate_stmt *stmt, crate_double_t val)``**

bind number value to the statement, anything that doesn't fit into ``int``

**``int crate_stmt_bind_string(crate_stmt *stmt, const char *val)``**

bind string value to the statement

**``int crate_stmt_execute(crate_stmt *stmt)``**

just like ``crate_execute`` but executes query for the provided statement, returns number of affected rows or 1 on success, and -1 on failure, to check for error message use ``crate_stmt_error_message(stmt)``

**``void crate_stmt_close(crate_stmt *stmt)``**

clean statement upon finishing execution.

Caveats
=======

crate-c use duktape javascript engine internally , and since numbers are treated as **double** precision floats in javascript then the maximum crate **long** value (*int64_t*) / (*long long*) is actually  2^53 **not** (2^63)-1, smae thing with minimum value, so for example when trying to store ``9223372036854775807`` in long fields you will get ``9223372036854776000`` instead. This is also true with Crate Admin Console, because it runs inside the browser and internally use Javascript to serialize data flow.

In short, the maximum number can be stored is ``9007199254740992``

# TODO

* Blob support
* Connection Timeout
* Field info
* Table info
* Custom javascript functions
* Examples
