comm BSD Sockets Sample Server
==============================

This is a sample BSD Sockets server closely related to the one given
in Beej's Guide to Network Programming.  The most noteworthy additions
in particular is support for Windows Socket servers and MS-DOS 32-bit
DPMI socket servers (via DJGPP and Watt-32 TCP/IP).  For Unix, there
is both a threaded server and a forking server, but Windows only
supports a threaded server.  MS-DOS currently only uses a
single-tasking server... though I'm not positive I've also tested the
server, maybe I only tested the client thus far.

There is also a simple example test for creating and using a Unix
FIFO.  It only works on Unix, of course.
