#include "../src/crate.h"
#include "lib/ptest.h"

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>
#endif

#include <limits.h>
#include <stdio.h>

#define ExampleTable \
"CREATE TABLE c_test_table1 (\
	id integer primary key,\
	title string\
)"


#define ExampleDataTypesTable \
"create table c_datatypes_test_table (\
	id integer,\
	integer_column integer,\
	long_column long,\
	short_column short,\
	double_column double,\
	float_column float,\
	byte_column byte,\
	ip_addr ip,\
	time timestamp\
)"


crate_db *crate;

void test_sleep(int milliseconds){
	#ifdef _WIN32
        Sleep(milliseconds);
    #else
        usleep(1000 * milliseconds);
    #endif
}

PT_SUITE(suite_basics) {

	PT_TEST(create_table) {
	    int ret = crate_execute(crate, ExampleTable);
		PT_ASSERT(ret == 1);
	}
	
	PT_TEST(create_already_existing_table) {
	    int ret = crate_execute(crate, ExampleTable);
		PT_ASSERT(ret == -1);
		int errcode = crate_error_code(crate);
		PT_ASSERT(errcode == 4093);
	}

	PT_TEST(adding_some_values) {
		int ret = crate_execute(crate, "INSERT INTO c_test_table1(id, title) VALUES (1, 'test title')");
		PT_ASSERT(ret == 1);
	}

	PT_TEST(adding_another_value) {
		int ret = crate_execute(crate, "INSERT INTO c_test_table1(id, title) VALUES (2, 'test title 2')");
		PT_ASSERT(ret == 1);
	}

	PT_TEST(search_table_single_result) {
		test_sleep(1000); /* some time for indexing */
		int rows = crate_execute(crate, "SELECT id,title FROM c_test_table1 WHERE id = 1");
		PT_ASSERT(rows == 1);
	}

	PT_TEST(search_table_all_results) {
		test_sleep(1000); /* some time for indexing */
		int rows = crate_execute(crate, "SELECT id,title FROM c_test_table1 WHERE id >= 1 ORDER BY id");
		PT_ASSERT(rows == 2);
		int i;
		for (i = 0; i < rows; i++){
			int id = crate_get_int(crate, i, "id");
			PT_ASSERT((i+1) == id);
		}
	}

	PT_TEST(crate_statement) {
		crate_stmt *stmt = crate_stmt_init(crate);
		crate_stmt *stmt2 = crate_stmt_init(crate);
		PT_ASSERT(stmt != NULL);
		PT_ASSERT(stmt2 != NULL);

		crate_stmt_prepare(stmt, "SELECT id,title FROM c_test_table1 WHERE id >= ? ORDER BY id");
		crate_stmt_bind_int(stmt, 1);

		crate_stmt_prepare(stmt2, "SELECT id,title FROM c_test_table1 WHERE id >= ? ORDER BY id");
		crate_stmt_bind_int(stmt2, 2);

		int rows2  = crate_stmt_execute(stmt2);
		int rows   = crate_stmt_execute(stmt);

		PT_ASSERT(rows2 == 1);
		PT_ASSERT(rows == 2);

		int i;
		for (i = 0; i < rows; i++){
			int id = crate_stmt_get_int(stmt, i, "id");
			const char *title = crate_stmt_get_string(stmt, i, "title");
			if (i == 0){
				PT_ASSERT_STR_EQ(title, "test title");
			} else {
				PT_ASSERT_STR_EQ(title, "test title 2");
			}
			
			PT_ASSERT((i+1) == id);
		}

		crate_stmt_close(stmt);
		crate_stmt_close(stmt2);
	}

	PT_TEST(drop_table) {
		int ret = crate_execute(crate, "DROP TABLE c_test_table1");
		PT_ASSERT(ret == 1);
	}
}

/* create numeric data types */
PT_SUITE(suite_data_types) {

	PT_TEST(int_size) {
		PT_ASSERT(sizeof(int) == 4);
	}

	PT_TEST(create_data_types_table) {
		int ret = crate_execute(crate, ExampleDataTypesTable);
		PT_ASSERT(ret == 1);
	}

	PT_TEST(integer_column) {
		crate_stmt *stmt = crate_stmt_init(crate);
		crate_stmt_prepare(stmt, "INSERT INTO c_datatypes_test_table (id, integer_column) Values (1,?)");
		long long y;
		y = 2147483649u;

		PT_ASSERT(y > INT_MAX);
		crate_stmt_bind_number(stmt, (double)y);
		/* should fail, number larger than allowed field size */
		int ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == -1);
		PT_ASSERT(crate_stmt_error_code(stmt) == 4003);

		/* now should succeed */
		crate_stmt_bind_int(stmt, 2147483647);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		test_sleep(1000);
		crate_stmt_prepare(stmt, "SELECT integer_column FROM c_datatypes_test_table WHERE id = ?");
		crate_stmt_bind_int(stmt, 1);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);
		int integer_val = crate_stmt_get_int(stmt, 0, "integer_column");
		PT_ASSERT(integer_val == 2147483647);

		crate_stmt_close(stmt);
	}

	PT_TEST(long_column) {
		int ret;

		crate_stmt *stmt = crate_stmt_init(crate);
		crate_stmt_prepare(stmt, "INSERT INTO c_datatypes_test_table (id, long_column) Values (2,?)");

		/* 
			since we are using javascript engine
			internally, javascript numbers are double precision floats
			so the maximum number for int64 is actually
			2^53         = 9007199254740992
			not (2^63)-1 = 9223372036854775807
		*/
		crate_long_t long_num = 9007199254740992;
		crate_stmt_bind_long(stmt, long_num);

		// crate_long_t long_num = 9223372036854775807;
		// crate_stmt_bind_string(stmt, "9223372036854775807");

		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		test_sleep(1000);
		crate_stmt_prepare(stmt, "SELECT long_column FROM c_datatypes_test_table WHERE id = ?");
		crate_stmt_bind_int(stmt, 2);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);
		
		crate_long_t long_val = crate_stmt_get_long(stmt, 0, "long_column");
		PT_ASSERT(long_val == long_num);
		// printf("LONG  = %I64d\n", long_num);
		// printf("LONG2 = %I64d\n", long_val);
		
		crate_stmt_close(stmt);
	}

	/* ip address */
	PT_TEST(ip_address) {
		int ret;

		crate_stmt *stmt = crate_stmt_init(crate);
		crate_stmt_prepare(stmt, "INSERT INTO c_datatypes_test_table (id, ip_addr) Values (3,?)");
		// crate_stmt_bind_string(stmt, "209.121.233.126");
		/* IP field are just of type long */
		crate_stmt_bind_long(stmt, 3514427774u);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		test_sleep(1000);
		crate_stmt_prepare(stmt, "SELECT ip_addr FROM c_datatypes_test_table WHERE id = ?");
		crate_stmt_bind_int(stmt, 3);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		if (ret == 1){
			const char *ip = crate_stmt_get_string(stmt, 0, "ip_addr");
			PT_ASSERT_STR_EQ(ip, "209.121.233.126");
		}

		crate_stmt_close(stmt);
	}

	/* timestamp test */
	PT_TEST(timestamp) {
		int ret;

		crate_stmt *stmt = crate_stmt_init(crate);
		crate_stmt_prepare(stmt, "INSERT INTO c_datatypes_test_table (id, time) Values (4,?)");
		/* timestamp is just of type long */
		/* far far away into future :) */
		crate_stmt_bind_long(stmt, 96108847875000);

		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		test_sleep(1000);
		crate_stmt_prepare(stmt, "SELECT format('%tY', date_trunc('year', time)) as year FROM c_datatypes_test_table WHERE id = ?");
		crate_stmt_bind_int(stmt, 4);
		ret = crate_stmt_execute(stmt);
		PT_ASSERT(ret == 1);

		if (ret == 1){
			const char *year = crate_stmt_get_string(stmt, 0, "year");
			PT_ASSERT_STR_EQ(year, "5015");
			// printf("TIME %s\n", year);
		}

		crate_stmt_close(stmt);
	}

	PT_TEST(drop_data_types_table) {
		int ret = crate_execute(crate, "DROP TABLE c_datatypes_test_table");
		PT_ASSERT(ret == 1);
	}
}

int main(int argc, char** argv) {
	crate = crate_init("options.js");

	pt_add_suite(suite_basics);
	pt_add_suite(suite_data_types);

	int ret = pt_run();
	crate_close(crate);
	return ret;
}
