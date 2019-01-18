# perl adapter to xcept
# by Thomas Eirich (eirich@informatik.uni-erlangen.de)
#

package xcept;

require 'sys/socket.ph';
require 'fcntl.ph';

$BTX           = "/usr/local/btx";		# path to btx installation
$prog_pid      = 0;			# xcept's pid if started successfully
$error_msg     = "";			# error strings if commands have failed
@reply	       = ();			# reply to a cmd
$prompt        = "XCEPT??????\n";	# xcept prompt string
$prefix        = "XCEPT!!!!!!";		# xcept prefix for non error messages
$version       = 1.0;			# expected version of xcept's client mode
$errlevel      = 0;			# flag for error in error handling

################################################################################
##  the following variables may be changed prior to calling
##  initiate() to change some default behavior.

$prog        = "$BTX/bin/xcept";	# the xcept server executable
$prog_opts   = "-x";			# more program options (like -h etc.)
$client_mode = "$BTX/lib/client-mode";	# client mode of xcept for interaction with perl
$timeout     = 30;			# time out for xcept communication
$debug       = 0;			# set true if debug information required
$logfile     = "";			# if set write logging info to this file
$replay      = "";			# if set replay info instead of starting xcept


################################################################################
##  use the following functions as constants for the corresponding
##  BTX symbols.
##
##  &INI
##  &TER

sub INI { "\\*"; }	# BTX-initiator symbol in xcept representation
sub TER { "\\#"; }	# BTX-terminator symbol in xcept representation


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  if the user has defined a function on_error in main call
#  this function each time an error occures
#
#  error( msg )  -->  -1

sub error {
    if( $errlevel++ > 0 ) {
	die "Double error!! - Exit.\n";
    }
    $error_msg = @_[0];
    $debug && print STDERR "\nPERL: error_msg = $error_msg\n\n";
    if( defined(&main'on_error()) ) {
	&main'on_error($error_msg);
    }
    return -1;
}


################################################################################
##  Initialize package and initiate a connection to the
##  xcept server. Check prompting and version of client mode etc.
##
##  initiate()  -->  0/-1

sub initiate {
    local( $server_exec );
    local( $i );

    if( $replay ne "" ) {
	$debug && print STDERR "PERL: Replaying '$replay'.\n";
	open( XCEPT_S1, "<$replay" ) || &error( "Cannot open '$replay': $!" );
    }
    else {
	if( $client_mode eq "" ) {
	    return &error( "No client script for xcept set" );
	}
	$server_exec = "$prog $prog_opts -s $client_mode";

	$debug && print STDERR "PERL: exec( $server_exec )\n";

	if( !socketpair( XCEPT_S1, XCEPT_S2, &PF_UNIX, &SOCK_STREAM, 0 ) ) {
	    return &error( "Creating a socket pair failed: $!" );
	}

	$SIG{'PIPE'} = 'IGNORE';
	if( ($prog_pid = fork()) < 0 ) {
	    return &error( "Fork failed: $!" );
	}

	if( $prog_pid == 0 ) {			# xcept (server) side
	    fcntl(XCEPT_S2,&F_SETFD,0);		# clear close on exec flag
	    if( !open(STDIN,  "<&XCEPT_S2") ) {
		return &error( "Dupping socket to stdin failed: $!" );
	    }
	    if( !open(STDOUT, ">&XCEPT_S2") ) {
		return &error( "Dupping socket to stdout failed: $!" );
	    }
	    close(XCEPT_S1);
	    close(XCEPT_S2);
	    select(STDOUT); $| = 1;		# make communication unbuffered

	    exec($server_exec);
	    die "Exec failed for: $server_exec\n";
	}

	close(XCEPT_S2);

	if( $logfile && !open( XCEPT_LG, ">$logfile" ) ) {
	    return &error( "Cannot open '$logfile': $!" );
	}
    }
    #
    # set non-blocking reads, since perl reads more than one line
    # on commands like $_=<XCEPT_S1>. Thus you have first to check if
    # there is more data in XCEPT_S1 without blocking if nothing there.
    # if there no more data in XCEPT_S1 (input eq "") than call
    # select.
    #
    fcntl(XCEPT_S1,&F_GETFL,$i);	# non-blocking reads, since perl read more than
    $i |= &FNDELAY;
    fcntl(XCEPT_S1,&F_SETFL,$i);

    select(XCEPT_S1);		# make comm. unbuffered
    $| = 1;
    select(STDOUT);

    if( ($ln = &receive_from_xcept()) eq "" ) {
	return -1;
    }
    if( $ln ne $prompt ) {
	return &error( "Missing initial prompt" );
    }
    return &version();
}


################################################################################
##  shutdown connection to xcept server and terminate the server
##
##  terminate()  --> %

sub terminate {
    &send_to_xcept( "quit" );		# tell xcept to terminate
    close(XCEPT_S1);			# security mode
    $logfile  && close(XCEPT_LG);	# close logfile (if was open)
    $prog_pid && kill( 9, $prog_pid );	# paranoid security mode
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  Send the specified strings to the xcept server
#
#  send_to_xcept( m1, ... )  -->  0/-1

sub send_to_xcept {
    local( $msg1, $msg2 );

    $msg1 = join( "\n", @_ )."\n";
    $msg2 = join( " ",  @_ )."\n";

    $debug && print STDERR "PERL>>>>XCEPT: $msg2";

    if( $replay ) {
	$msg1 = <XCEPT_S1>;
	if( $msg1 ne $msg2 ) {
	    &error( "Current behavior diverges from replay" );
	}
    }
	
    $logfile && print XCEPT_LG $msg2;
    print XCEPT_S1 $msg1;

    return 0;
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  Receive exactly one line from the xcept server and
#  return it. Wait only a limited time for input.
#
#  receive_from_xcept()  --> line/""

sub receive_from_xcept {
    local( $nfound, $rin, $rout, $eout, $line );

    $debug && print STDERR "PERL<<<<XCEPT: ";

    while( ($line = <XCEPT_S1>) eq "" ) {	# non-blocking reads
	if( $replay ) {
	    &error( "EOF on replay" );
	    return "";
	}

	$rin  = '';
	vec($rin,fileno(XCEPT_S1),1) = 1;
	$nfound = select( $rout=$rin, undef, $eout=$rin, $timeout );

	if( $nfound < 0 ) {
	    &error( "Select failed: $!" );
	    return "";
	}
	if( $nfound == 0 ) {
	    &error( "Broken communication (TIMEOUT)" );
	    return "";
	}
	if( vec($eout,fileno(XCEPT_S1),1) ) {
	    &error( "Broken communication (Exception)" );
	    return "";
	}
	if( vec($rout,fileno(XCEPT_S1),1) ) {
	    if( ($line = <XCEPT_S1>) eq "" ) {
		&error( "Broken communication (EOF)" );
		return "";
	    }
	    last;
	}
    }
    $debug   && print STDERR  $line;
    $logfile && print XCEPT_LG $line;

    return $line;
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  Send specified message to xcept server and read in turn from
#  xcept up to the next prompt. All lines with a special prefix
#  are treated as regular reply to the issued command. All other
#  lines are saved and passed to error().
#  n specifies the expected number of regular reply lines.
#  The procedure fails if n doesn't match the replied lines.
#  If n < 0 the check is omitted.
#
#  command( n, m1, [m2, ...] )  --> 0/-1

sub command {
    local( $nlines ) = shift( @_ );
    local( $ln );
    local( $errlines );

    $errlines = "";
    @reply = ();

    if( &send_to_xcept( @_ ) < 0 ) {
	return -1;
    }
    while( ($ln = &receive_from_xcept()) ne "" ) {
	if( $ln eq $prompt ) {
	    last;
	}
	if( $ln =~ s/^$prefix// ){
	    chop $ln;
	    push( @reply, $ln );
	}
	else {
	    $errlines .= "xcept I/O: " . $ln;
	}
    }
    if( $errlines ne "" ) {
	return &error( $errlines );
    }
    if( $nlines >= 0 && $#reply != ($nlines - 1) ) {
	return &error( "Unexpected number of reply lines" );
    }
    return 0;
}

################################################################################
##
##  The rest are procedures relying on command()
##
################################################################################


##  version()  --> 0/-1
##
sub version {
    if( &command( 1, "version" ) < 0 ) {
	return -1;
    }
    if( $reply[0] > $version ) {
	return &error( "Wrong version (found $reply[0], but expecting $version or less)" );
    }
    return 0;
}


##  connect()  -->  0/-1
##
sub connect {
    return &command( -1, "connect" );
}


##  disconnect()  -->  0/-1
##
sub disconnect {
    return &command( 0, "disconnect" );
}


##  status()  -->  "status"/""
##
sub status {
    local( $stat );

    if( &command( 1, "status" ) < 0 ) {
	return "";
    }
    return $reply[0];
}


##  dial( number )  -->  0/-1
##
sub dial {
    return &command( 0, "dial", @_[0] );
}


##  screen()  --> @s[0-23]/()
##
sub screen {
    if( &command( 24, "screen" ) < 0 ) {
	return ();
    }
    return @reply;
}


##  waitdct()  --> 0/-1
##
sub waitdct {
    return &command( 0, "waitdct" );
}


##  diaplay_on( [x-disp-spec] )  --> 0/-1
##
sub display_on {
    if( $_[0] ne "" && &command( 0, "display", $_[0] ) < 0 ) {
	return -1;
    }
    return &command( 0, "onx" );
}


##  display_off()  --> 0/-1
##
sub display_off {
    return &command( 0, "offx" );
}


##  playback( filename )  -->  0/-1
##
sub playback {
    return &command( 0, "playback", $_[0] );
}


##  debug_xcept_client_mode()  -->  0/-1		defaults to on
##  debug_xcept_client_mode( "on"/"off" )  -->  0/-1
##
sub debug_xcept_client_mode {
    return &command( 0, "debug", $#_ >= 0 ? $_[0] : "on" );
}


##  send( s1, [s2, ...] )  -->  0/-1
##
##  send s1,... to btx server as typed to BTX
##
sub send {
    return &command( 0, "send", join("", @_) );
}


##  next()  --> 0/-1
##
##  Send terminator and wait for DCT
##  for user convenience (this sequence is often used)
##
sub next {
    return &command( 0, "send", &TER )
	|| &waitdct();
}


## input( prompt, default )  -->  "input\n"/""
##
## let xcept prompt for input (via popup forms if
## X display is on).
##
sub input {
    local( $iprompt, $idefault ) = @_;

    if( &command( 1, "input", $iprompt, $idefault ) ) {
	return "";
    }
    return $reply[0]."\n";
}


## guest_login()  -->  0/-1
##
## Login via guest id
##
sub guest_login {

    return &connect()		# connect to btx server and wait for btx online
        || &next()		# take preprinted guest id
        || &waitdct();		# wait for intro page 2
}


## user_login( loginID, passwd, userID )  -->  0/-1
##
## Login as user.
## login-id  must be a string of exactly 12 digits
## passwd    is a password string of up to 8 characters
## user-id   must be a string of a most 4 digits
##           (if omitted defaults to "\#" = preprinted userID)
## btxnr     BTX-number (string of up to ?? digits)
##           (if omited defaults to "\#" = preprinted number)
##
sub user_login {

    local( $loginID, $passwd, $userID, $btxnr ) = @_;
    local( $_ );

    # check validy of parameters
    #
    if( $loginID !~ /^[0-9]{12}$/ ) {
	return &error( "Badly formed login ID" );
    }
    if( $passwd !~ /^.{4,8}$/ ) {
	return &error( "Badly formed password" );
    }
    if( $userID ne "" && $userID !~ /^[1-9][0-9]{0,3}$/ ) {
	return &error( "Badly formed user ID" );
    }
    if( $btxnr ne "" && $btxnr !~ /^[0-9]+$/ ) {
	return &error( "Badly formed BTX number" );
    }

    # add #'s if necessary
    #
    if( length($userID) <  4 ) { $userID .= &TER; }
    if( length($passwd) <  8 ) { $passwd .= &TER; }
    if( length($btxnr)  < 12 ) { $btxnr  .= &TER; }

    # now let's try it
    #
    if( &connect()		# connect to btx server and wait for btx online
     || &send( $loginID )	# if ID is wrong BTX disconnects!
     || &waitdct() ) {		# wait for next intro page or error page
	return -1;
    }
    if( &status() !~ /^online/i ) {
	return &error( "Login procedure failed" );
    }
    if( &send( $btxnr )
    ||  &waitdct()
    ||  &send( $userID )
    ||  &waitdct()
    ||  &send( $passwd )
    ||  &waitdct() ) {
	return -1;
    }
    $_ = join("", &screen());
    if( !/Sie benutzten .* zuletzt/ ) {
	return &error( "Login procedure failed" );
    }

    return 0;
}

1;

