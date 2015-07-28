#include <iostream>
#include "../src/crate.h"

using namespace std;

int main (){
	crate_db *db = crate_init("options.js");
	int count = crate_execute(db, "SELECT * FROM articles where id > 10 LIMIT 10");
	cout << "The result is " << count;
	crate_close(db);
	exit(1);
}
