use strict;
use warnings;
my $isWin = ($^O =~ /win32/i);

my $buildTest = "gcc -Wall -Werror -Wno-missing-braces"
				 . " -I../src/deps"
				 . " -I./lib"
				 . " ../src/deps/duktape/duktape.c"
				 . " ../src/crate.c"
				 . " ./lib/ptest.c"
				 . " test.c"
				 . " -o test" . ($isWin ? ".exe" : "")
				 . ($isWin ? " -lws2_32" : " -lm");

system($buildTest) && die $!;
my $ret = system(($isWin ? "test.exe" : "./test"));

unlink (($isWin ? "test.exe" : "test"));

if ($ret){ 
	die "Test Failed"; 
}

print "Tests Passed Successfully!\n\n";
