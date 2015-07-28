#ifndef _CRATE_H
#define _CRATE_H

typedef int crate_int_t;
typedef long long crate_long_t;
typedef double crate_double_t;
typedef float crate_float_t;

typedef struct {
	void *ctx; /* duktape context */
} crate_db;

typedef struct {
	double id;
	const char *query;
	void *ctx; /* duktape context */
} crate_stmt;

#ifdef __cplusplus
extern "C" {
#endif

crate_db *crate_init (const char *options);

int crate_execute (crate_db *crate, const char *sql);

const char *crate_get_string(crate_db *crate, int row, const char *field);

int crate_get_int(crate_db *crate, int row, const char *field);

double crate_get_number(crate_db *crate, int row, const char *field);

const char *crate_error_message(crate_db *crate);

int crate_error_code(crate_db *crate);

void crate_close(crate_db *crate);

/* sql statement functions */

crate_stmt *crate_stmt_init(crate_db *crate);

int crate_stmt_prepare(crate_stmt *stmt, const char* query);

int crate_stmt_bind_int(crate_stmt *stmt, int val);

int crate_stmt_bind_long(crate_stmt *stmt, crate_long_t val);

int crate_stmt_bind_number(crate_stmt *stmt, double val);

int crate_stmt_bind_string(crate_stmt *stmt, const char *val);

int crate_stmt_bind_true(crate_stmt *stmt);

int crate_stmt_bind_false(crate_stmt *stmt);

int crate_stmt_execute(crate_stmt *stmt);

const char *crate_stmt_get_string(crate_stmt *stmt, int row, const char *field);

int crate_stmt_get_int(crate_stmt *stmt, int row, const char *field);

crate_long_t crate_stmt_get_long(crate_stmt *stmt, int row, const char *field);

double crate_stmt_get_number(crate_stmt *stmt, int row, const char *field);

const char *crate_stmt_error_message(crate_stmt *stmt);

int crate_stmt_error_code(crate_stmt *stmt);

void crate_stmt_close(crate_stmt *stmt);

#ifdef __cplusplus
}
#endif

#endif /* _CRATE_H */
