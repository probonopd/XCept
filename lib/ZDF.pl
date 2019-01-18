#!/usr/local/bin/perl

push(@INC,"/local/btx/lib");
require 'xcept.pl';

($ME = $0) =~ s|.*/||;


sub on_error {
    local( $err ) = @_;

    die "$ME: $err\n";
}

#  $xcept'replay     = "perl-xcept.log";
#  $xcept'logfile    = "perl-xcept.log";
#  $xcept'debug      = 1;
#  $xcept'prog_opts .= " -h vespucci";
$xcept'timeout    = 60;

&xcept'initiate();
&xcept'connect();	# Verbindung zum BTX server
&xcept'next();		# Gast Kennung

$screen = join("\n",&xcept'screen());
if( $screen !~ /(\d\d)\.(\d\d)\.(\d\d)\s+\d\d:\d\d/ ) {
    &on_error( "Pattern match failed for date on welcome page" );
}

$day   = $1;
$month = $2;
$year  = "19$3";

&xcept'dial( "636004" );
$screen = join("\n",&xcept'screen())."\n";

if( $screen !~ /^(\w+).+$day\.$month.+$year.*(\d)/ ) {
    &on_error( "Pattern match failed for weekday selection" );
}

$weekday = $1;
$dial    = $2;

&xcept'send( "$dial" );
&xcept'waitdct();

@screen = &xcept'screen();
$out = join("\n", @screen[6..20]), "\n";
&xcept'next();
@screen = &xcept'screen();
$out .= join("\n", @screen[8..21]), "\n";
$out =~ s/\n\s*\n/\n/g;

print "$out\n";

&xcept'terminate();


