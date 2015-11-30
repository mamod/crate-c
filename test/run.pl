use strict;
use warnings;
use Data::Dumper;
use IO::Socket::INET;
use Config;
use Archive::Extract;

my $isWin = ($^O =~ /win32/i);
my $childpid;

my $cc = $Config{cc} || 'cc';


#is this travis?
if ($ENV{TRAVIS}){
# if (1){
    #crate server has been downloaded, unpack,
    #run the server and make sure it's running
    $childpid = fork();
    if ($childpid == -1){
        die "can't fork and run crate server";
    }

    if ($childpid == 0){
        my $ae = Archive::Extract->new( archive => "/home/travis/tmp/crate.tar.gz" );
        my $ok = $ae->extract( to => './build' );
        my $ret = system('./build/crate-0.52.2/bin/crate > CrateResult.txt');
        exit(0);
    }

    my $socket;
    my $timeout = 15;
    while (--$timeout){
        $socket = IO::Socket::INET->new (
            Proto           => "tcp",
            PeerAddr        => "localhost",
            PeerPort        => "4200",
            Timeout         => "10",
        );

        if ($socket){
            $socket->close();
            print STDERR "Crate Server Running, start testing\n";
            last;
        } else {
            print STDERR "Crate Server NOT Running, try again\n";
        }
        sleep 1;
    }

    #timed out
    if ($timeout <= 0){
        clean_all();
        die "Crate didn't start for more than 10 seconds, tests aborted" ;
    }

    test();
} else {
    test();
}

##building process
sub test {
    my $pid = shift;
    my $isWin = ($^O =~ /win32/i);

    my $buildTest = $cc . " -Wall -Werror -Wno-missing-braces"
                 . " -I./src/deps"
                 . " -I./test/lib"
                 . " ./src/deps/duktape/duktape.c"
                 . " ./src/crate.c"
                 . " ./test/lib/ptest.c"
                 . " ./test/test.c"
                 . " -o ./build/test" . ($isWin ? ".exe" : "")
                 . ($isWin ? " -lws2_32" : " -lm");

    system($buildTest) && die $!;

    my $ret = system( "./build/" . ($isWin ? "test.exe" : "./test"));

    clean_all();

    if ($ret){
        die "Test Failed " . $!; 
    }

    print "Tests Passed Successfully!\n\n";
}

##crate -d option didn't work
sub get_crate_pid {
    my $result;
    if (!open($result, "<", "CrateResult.txt")){
        return 0;
    }

    while (<$result>){
        $_ =~ /pid\[(.*?)\]/g || next;
        close $result;
        return $1;
    }
    close $result;
}

sub clean_all {

    my $cratepid = get_crate_pid();

    kill 9, $cratepid if $cratepid;
    kill 9, $childpid if $childpid;

    unlink "./build/" . ($isWin ? "test.exe" : "test");
    unlink "./CrateResult.txt";
}
