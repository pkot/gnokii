                            - G N O K I I -
               (C) Hugh Blemings, hugh@vsb.com.au 1999
               Last update 18 May 1999   Version 0.2.5

- Introduction -

This is version 0.2.5 of gnokii, a Linux/Unix tool suite and (eventually)
modem/fax driver for Nokia mobile phones, released under the GPL.  Please
see COPYING for more details on license/copying issues.

At present 3110, 3810 and 8110 phones with the DAU-4F cable have had
the most testing.  Other cables, including home made ones will very likely
work.  Details of other handsets and networks are provided below.

Code for the 6110 phones has been tested with the DAU-9P MBus Cable NSE-3.
Note that the 6110 code is still under heavy development and may not work
with your particular firmware though testing to date has been encouraging.

Bugs where known are detailed in the file BUGS...  Play around
with it, hack at it and see what you discover but please do so in the
knowledge that it comes with no warranty or guarantee - you use
it at your own risk.

In the following notes, "3810 Series" referes to the 3110, 3810 and
8110 models.  "6110 Series" refers to the 6110, 5110 and hopefully
others.

As you may be aware, there are indications that Nokia may provide
some support for the project.  These negotations are continuing
and nothing is definate but are most encouraging.

- Notes for 3810 Series -

Relative to the previous version, little has changed in the
area of 3810 functionality due to (in part) a lack of (Hugh's) time 
to work on it and to a lesser extent the hope that we may receive some
information from Nokia which will alleviate much of the guesswork
that has been needed to date.

When built with 3810 support, the resulting executable allows you
to read/write phonebook entries, send SMS messages to the network,
retrive SMS messages from the phone, delete SMS messages from the phone
and enter a monitor mode to look at the protocol and any unknown
messages that may occur.  The latter are printed in hexadecimal form
and are provided to help with understanding of the protocol.

The code for reading/writing phonebook entries has had limited testing
but the read code is believed to be solid.  It has been observed
that the write code occasionally fails to write a location for
reasons as yet unknown.  The format of the input to the write
option is relatively unforgiving and does only very basic parsing.
This format should be compatible with the output from the Windows
NCDS software.

Routines for retrieving SMS messages have been tested and appear
to be bug free.  The 3810 used for testing appears to ignore the
memory type field however.

Sending SMS is something of another matter.  This has been tested
on a 3810 on the Australian Optus/Hutchison network.  Most of the
time the messages are sent as expected but occasionally they
fail for no apparent reason.  The code is designed to re-try up
to four times before giving up if a 0x65 0x26 response is received.

Sometimes however error codes are seen 0x65 0xYY, the meaning
of which is unknown.  Any light you can cast on this is welcome.
It is possible that switching off the phone and switching
on again may help but this may be just a coincidence.

It should also be noted that the SMS header message sent by
FB38_TX_Send0x23_SendSMSHeader code has no less than 10 bytes
in it that aren't understood.  The values used are simply those
that have been observed.  They may have to be totally different
on your network :(  They may inadvertently route the message to
a distant land or just not work at all.

A number of people have now tested SMS sending on various networks
around the world and in the main have found it to work correctly.

- Notes for 6110 Series -

Phones of 6110 Series have somewhat different protocol and that
is the main reason for having the 3810 Series and 6110 Series.
These phones also provide more features so hacking on it is
greater fun.

The current code allows you to enter PIN from the PC (gnokii asks
you for the PIN, you enter it and gnokii sends it to the phone
without touching the phone's keyboard). The code also allows you
to monitor the status of the mobile phone - it recognizes
network signal strength, your GSM provider network and battery
level, the clock, date and alarm setting of your phone (if the phone
supports it - e.g. 5110 doesn't seem to support date so the date is
bogus on it).

The newest feature is that gnokii can read your mobile phone's
preferred SMS Center so in the future you will not need to enter
the SMS Message Center number for SMS messages to be sent.
Gnokii will read it automatically.

There are some problems with the newest firmwares (V 05.24). It
seems that this firmware is weird. If you have a Nokia with this
firmware, it accepts the PIN request (displays Code Accepted message)
but nothing is returned to the PC. The communication is only in
one direction :-( I do not know why. If you can help with this,
please let me know.


- Installation -

Review the Makefile and make sure that the MODEL and PORT defines
are correct for your setup.  The file has comments on what to
do - if you have questions please ask them on the mailing list.
You'll need r/w permissions on the port you select.  Depending
on what functions have been implemented for your particular model,
you may see "Not Implemented" messages when certain functions
are used.

For 3810 series users if your SIM card allows names longer than
10 character, modify FB38_DEFAULT_SIM_PHONEBOOK_NAME_LENGTH in
fbus-3810.h accordingly.  There is a mechanism for this to be
set automatically but it isn't yet understood :(

Type make, all should build from there, it's pretty simple code
so you shouldn't have any problems.  Note that you need pthread
libraries, they are standard on most modern Linux distributions.
Note that gnokii is not only for Linux - you can use it on
FreeBSD and similar systems too.


- Known Bugs -

Review the file BUGS in the first instance.

If you are using gnokii, particularly in these early days,
it is worth joining the mailing list as bugs will be reported
there first!   Alternatively review the BUGS file on the web site.


- How to help -

You can assist the development process and by making note of any
messages that occur and seeing if you can work out what they mean.

Suggestions on changes to the code are welcome, the preference is that
they be aired on the mailing list so that a consensus can be reached
when appropriate.

The gnokii project now has a CVS server,  served by the Czech Linux
Users' Group. The server's address is cvs.linux.cz. If you are
already familiar with CVS, set the CVSROOT to 

	:pserver:cvs@cvs.linux.cz:/cvs/gnokii

and use an empty password for checkout the repository with the name
gnokii.

If you are not familiar with cvs, do not worry. It's very simple.
Just install the cvs package for your distribution and do these steps:

	1. set your environmental variable CVSROOT. If you have bash (Bourne
           Again Shell):

	export CVSROOT=:pserver:cvs@cvs.linux.cz:/cvs/gnokii

	   If you have different shell, use different command (e.g. setenv).


	2. Log in to the CVS server as anonymous reader by typing

	cvs login

	   Use an empty password.


	3. If you are logged in you are ready to checkout the newest sources:

	cvs co gnokii

If you prefer, there is a WWW interface to the CVS server at

http://odysseus.fi.muni.cz/cgi-bin/cvsweb.cgi/?cvsroot=gnokii

Now you are on your own - you have the current sources on your disk
in the directory gnokii.  If you are going to make substantial changes
and feel that they should be included into gnokii please mention it
on the list first to avoid duplicated effort.
	

- Protocol Details -

3810 Series

The protocol used between the phone and the PC has been worked out
by a mixture of trial and error, monitoring the data that goes between
the 3810 and its Windows 95 software and a few snippets of information
found on the 'net relating to older Nokia models.  Feedback from
people who played with the first release of gnokii was invaluable.


6110 Series

The same applies for 6110 Series (but I use Windows NT 4.0 for
monitoring). The protocol of 6110 Series is somewhat different from 3810
Series. Perhaps the first draft of the protocol for communication between
Nokia mobile phones and PCs was not so general to support the newest features
of Nokia's phones.

The 6110 protocol seems to use some magic in the communication (the phone
responds with different bytes each time and the next answer from the PC
is dependant on the previous `magic' answer from the phone). We do not
get the magic so we are not able to display the message "Accesory connected!"
on the display as Nokia Cellular DataSuite does. But hopefully we will be
able in the future. We are hopeful of getting some help from Nokia :-)


- To Do -

A lot! :)

The software roadmap document on the web site contains information
about the proposed future directions for the project.  Additionally,
these sort of matters are actively discussed on the mailing list.

At the present, the user interface, error reporting and such like
is hardly what one would call user friendly.  This too will improve
as the project progresses.


- References -

The web site for this project is at;

	<http://multivac.fatburen.org/gnokii>

The mailing list devoted to the gnokii project is

	<gnokii@net.lut.ac.uk>

You can subscribe by sending an email to

	<gnokii-request@net.lut.ac.uk>

with the word subscribe in the body.

The serial programming how-to was valuable in cobbling together
serial code for both monitoring and the tools themselves.  This
is the Australian mirror, there will be one nearby to you too :)

<http://www.monash.edu.au/mirror/ldp/HOWTO/Serial-Programming-HOWTO.html>


- Handsets/Networks tested -

3810 Series

3110/Sonera (Finland)
3110/Telefonica (Spain)
3810/Telstra (Australia)
3810/Optus [Actually Hutchison] (Australia)
3810/Mobile One (Singapore)
3810/SingTel Mobile (Singapore)
8110/Proximus (Belgium)
8110/D2 Privat (Germany)
8110i/MTN (South Africa)
8110/Beeline Vympelkom (Russia)

6110 Series

6190/Microcel (Canada) (GSM 1900)
6110/Panafon (Greece)
6110/Paegas (Czech Republic)
5110/Paegas (Czech Republic)
6150/Omnitel (Italy)
5110/Omnitel (Italy)

- Credits -

Thank you to, in no particular order...

*  Everyone on the gnokii mailing list for their interest
   and input into the project.

*  Pavel Janík ml. for the majority of work on gnokii 6110 series
   support, spelling corrections and miscellaneous code cleanups.

*  Francois Dessart, Brendan O'Dea, Tony Lindstrom, Steffan Henke
   and Cobus Van Eeden for dumps (and in some cases interpretation!)
   of unknown messages, bug reports and prompt feedback of being
   able to build new releases.

*  Staffan Ulfberg for taking care of the website and for instigating
   the 6110 project which has now been merged with the gnokii effort.

*  Martin Hamilton for looking after the mailing list.

*  Tim Potter some of the ideas that are now incorporated in gnokii
   to provide multiple model support.

*  Richard Kalton for his excellent page about Nokia 6110 protocol.

*  Colin Paton for his mail with subject "Dumps of phone<>PC traffic".
   It helped a lot.

*  Juan Altmayer Pizzorno for his help with MBUS/FBUS and other
   debugging.

*  Gary Reuter for his help when looking for a bug in phone numbers,
   for the set-time function debugging.

*  Jiri Meloun for voice calling.

*  Emilio Brambilla for SMS notification fixes, setdatetime implementation
   and typing GSM_Default_Alphabet.

*  Czech Linux Users' group for it's CVS server which hosts our
   development repository.

*  Goran Dokic, Samuli Sorvakko and anonymous for gettext files for
   Dutch, Finnish and German translations.

*  Lucy for making pizza and putting up with Hugh disappearing to
   work on this :)

*  Last not least: Mirka for allowing Pavel to work on gnokii...


- Release Notes -

19990519 Version 0.2.5
Little has changed on the 3810 side - mostly adding stub functions
to stay current with the 6110 code.  A no-brainer bug was found in 
the SMS send routines which is now corrected - could have caused
multiple messages to be sent.

6110 support has moved ahead in leaps and bounds, SMS sending is
now in there along with many other new function.

19990328 Version 0.2.4
Largely a synchronisation release to incorporate Pavel's numerous
patches for 6110 support and internationalisation (such a long
word, i18n is better :-).

Added SMS delete function for 3810 series.

19990305 Version 0.2.3
Added gsm-api.c/.h and gsm-common.h as a standard interface and
set of datatypes that can be used across multiple models.  This
API is still a work in progress.

Changed references to "handset" to "phone" to ensure a consistent
nomenclature.

Phonebook is now dumped with words int or sim for memory type
rather than a numeric value.

19990219 Version 0.2.2
Fixed bug and added support to display bytes returned when
the link is initialised.  If you see something other than the
values noted in fbus.c, please let me know and include
your phone type and software version.

Added initial cut of SMS send capability.  Caveats are
discussed above.

Added two different default values for length of phonebook
name and numbers.  These are selected according to whether
internal or SIM memory is specified.

Fixed bugs reported in 0.2.1 (errno.h and reversal of message
centre and sender fields)

19990216 Version 0.2.1
Fixed bug that caused phone book entries containing spaces to be
mangled.  Added retrieve SMS message function.

19990213 Version 0.2.0
First version that looks more like the finished product - supports
command line options etc. 

19990126 Version 0.1.0
Initial release of monitoring tool - first rough incarnation of
gnokii - what will ultimately be the monitoring and programming tool.
