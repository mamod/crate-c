#include "main.h"
#include "crate.h"
#include <float.h>
#include <limits.h>

#include "crate_javascript.h"
#include "deps/duktape/duktape.h"
#include "deps/socket.c"

crate_db *crate_init (const char *options){
    crate_db *crate = malloc(sizeof(*crate));

    if (crate == NULL){
        return crate;
    }

    duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);

    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "crate");
    duk_pop(ctx);

    /* initiate global process */
    duk_push_global_object(ctx);
    duk_push_object(ctx);

    /* initiate socket bindings */
    init_binding_socket(ctx);
    duk_put_prop_string(ctx, -2, "socket");
    duk_put_prop_string(ctx, -2, "process");
    duk_pop(ctx);

    /* load internal functions */
    if (duk_peval_file(ctx, "crate.js") != 0) {
        printf("Error %s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }

    duk_pop(ctx);

    /* load options */
    if (duk_peval_file(ctx, options) != 0) {
        printf("Error %s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }

    duk_pop(ctx);
    
    crate->ctx = ctx;
    return crate;
}

/* execute sql query */
int crate_execute (crate_db *crate, const char *sql){
    duk_context *ctx = crate->ctx;
    duk_eval_string(ctx, "crate_db.execute_sql");
    duk_push_string(ctx, sql);
    duk_call(ctx, 1);
    int ret = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return ret;
}

void crate_close (crate_db *crate){
    if (crate == NULL){
        return;
    }

    duk_context *ctx = crate->ctx;
    duk_eval_string(ctx, "crate_db.close");
    duk_call(ctx, 0);
    duk_destroy_heap(ctx);
    free(crate);
}

static void crate_get_row_data(duk_context *ctx, int row, const char *field) {
    duk_eval_string(ctx, "crate_db.get_row_data");
    duk_push_int(ctx, row);
    duk_push_string(ctx, field);
    duk_call(ctx, 2);
}

const char *crate_get_string(crate_db *crate, int row, const char *field) {
    duk_context *ctx = crate->ctx;
    crate_get_row_data(ctx, row, field);
    const char *ret = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return ret;
}

int crate_get_int(crate_db *crate, int row, const char *field) {
    duk_context *ctx = crate->ctx;
    crate_get_row_data(ctx, row, field);
    int ret = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return ret;
}

double crate_get_number(crate_db *crate, int row, const char *field) {
    duk_context *ctx = crate->ctx;
    crate_get_row_data(ctx, row, field);
    double ret = (double)duk_get_number(ctx, -1);
    duk_pop(ctx);
    return ret;
}

const char *crate_error_message(crate_db *crate) {
    duk_context *ctx = crate->ctx;
    duk_eval_string(ctx, "crate_db.error_message");
    duk_push_null(ctx);
    duk_call(ctx, 1);
    const char *error = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return error;
}

int crate_error_code(crate_db *crate) {
    duk_context *ctx = crate->ctx;
    duk_eval_string(ctx, "crate_db.error_code");
    duk_push_null(ctx);
    duk_call(ctx, 1);
    int error = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return error;
}

/* crate statment */
crate_stmt *crate_stmt_init(crate_db *crate){
    duk_context *ctx = crate->ctx;
    crate_stmt *stmt = malloc(sizeof(*stmt));
    if (stmt == NULL){
        return stmt;
    }

    duk_eval_string(ctx, "crate_db.stmt_init");
    duk_call(ctx, 0);
    double id = (double)duk_get_number(ctx, -1);
    duk_pop(ctx);

    stmt->id = id;
    stmt->ctx = ctx;
    return stmt;
}

int crate_stmt_prepare(crate_stmt *stmt, const char *query){
    duk_context *ctx = stmt->ctx;
    duk_eval_string(ctx, "crate_db.stmt_query");
    duk_push_number(ctx, stmt->id);
    duk_push_string(ctx, query);
    duk_call(ctx, 2);
    duk_pop(ctx);
    return 1;
}

/* BIND FUNCTIONS */

#define STMT_BIND_START \
duk_context *ctx = stmt->ctx;\
duk_eval_string(ctx, "crate_db.stmt_bind");\
duk_push_number(ctx, stmt->id)

#define STMT_BIND_END \
duk_call(ctx, 2);\
duk_pop(ctx);\
return 1

int crate_stmt_bind_int(crate_stmt *stmt, int val) {
    STMT_BIND_START;
    duk_push_int(ctx, val);
    STMT_BIND_END;
}

int crate_stmt_bind_long(crate_stmt *stmt, crate_long_t val) {
    STMT_BIND_START;
    duk_push_number(ctx, (duk_double_t)val);
    STMT_BIND_END;
}

int crate_stmt_bind_number(crate_stmt *stmt, double val){
    STMT_BIND_START;
    duk_push_number(ctx, val);
    STMT_BIND_END;
}

int crate_stmt_bind_string(crate_stmt *stmt, const char *val) {
    STMT_BIND_START;
    duk_push_string(ctx, val);
    STMT_BIND_END;
}

int crate_stmt_bind_true(crate_stmt *stmt) {
    STMT_BIND_START;
    duk_push_true(ctx);
    STMT_BIND_END;
}

int crate_stmt_bind_false(crate_stmt *stmt) {
    STMT_BIND_START;
    duk_push_false(ctx);
    STMT_BIND_END;
}

/* END BIND FUNCTIONS */


int crate_stmt_execute(crate_stmt *stmt) {
    duk_context *ctx = stmt->ctx;
    duk_eval_string(ctx, "crate_db.stmt_execute");
    duk_push_number(ctx, stmt->id);
    duk_call(ctx, 1);
    int rows = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return rows;
}

const char *crate_stmt_error_message(crate_stmt *stmt) {
    duk_context *ctx = stmt->ctx;
    duk_eval_string(ctx, "crate_db.error_message");
    duk_push_number(ctx, stmt->id);
    duk_call(ctx, 1);
    const char *error = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return error;
}

int crate_stmt_error_code(crate_stmt *stmt) {
    duk_context *ctx = stmt->ctx;
    duk_eval_string(ctx, "crate_db.error_code");
    duk_push_number(ctx, stmt->id);
    duk_call(ctx, 1);
    int error = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return error;
}

static void crate_stmt_get_row_data(duk_context *ctx, int row, const char *field, double id) {
    duk_eval_string(ctx, "crate_db.stmt_get_row_data");
    duk_push_number(ctx, id);
    duk_push_int(ctx, row);
    duk_push_string(ctx, field);
    duk_call(ctx, 3);
}

const char *crate_stmt_get_string(crate_stmt *stmt, int row, const char *field) {
    duk_context *ctx = stmt->ctx;
    crate_stmt_get_row_data(ctx, row, field, stmt->id);
    const char *ret = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return ret;
}

int crate_stmt_get_int(crate_stmt *stmt, int row, const char *field) {
    duk_context *ctx = stmt->ctx;
    crate_stmt_get_row_data(ctx, row, field, stmt->id);
    int ret = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return ret;
}

crate_long_t crate_stmt_get_long(crate_stmt *stmt, int row, const char *field){
    duk_context *ctx = stmt->ctx;
    crate_stmt_get_row_data(ctx, row, field, stmt->id);
    crate_long_t ret = (crate_long_t)duk_get_number(ctx, -1);
    duk_pop(ctx);
    return ret;
}

double crate_stmt_get_number(crate_stmt *stmt, int row, const char *field){
    duk_context *ctx = stmt->ctx;
    crate_stmt_get_row_data(ctx, row, field, stmt->id);
    double ret = (double)duk_get_number(ctx, -1);
    duk_pop(ctx);
    return ret;
}

void crate_stmt_close(crate_stmt *stmt) {
    duk_context *ctx = stmt->ctx;
    duk_eval_string(ctx, "crate_db.stmt_close");
    duk_push_number(ctx, stmt->id);
    duk_call(ctx, 1);
    free(stmt);
}
