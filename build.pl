use strict;
use warnings;
use Config;
my $cc = $Config{cc} || 'cc';

##generate crate_javascript c string from
##crate.js file to be embedded directly into c
open(my $in, "<", "./src/crate.js") or die "cannot open < input.txt: $!";
open(my $out, ">", "./src/crate_javascript.h") or die "cannot open > output.txt: $!";

while(<$in>){
    my $line = $_;
    
    if (!$line || $line eq "" || $line eq "\n"){
        next;
    }

    $line =~ s/\\/\\\\/g;
    $line =~ s/"/\\"/g;
    $line =~ s/\r$//;
    $line =~ s/\n$//;
    $line = '"' . $line . '\n"';
    print $out $line, "\n";
}
print $out '""', "\n";

close $in;
close $out;

my $isWin = ($^O =~ /win32/i);
my $buildTest = $cc . " -Wall -Werror -Wno-missing-braces -shared" .  ($isWin ? "" : " -fPIC")
                . " -I./src/deps"
                . " ./src/deps/duktape/duktape.c"
                . " ./src/crate.c"
                . " -o ./build/crate" . ($isWin ? ".dll" : ".so")
                . ($isWin ? " -lws2_32" : " -lm");

system($buildTest) && die $!;
