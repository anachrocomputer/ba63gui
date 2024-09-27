# BA63GUI

This is a simple GUI for the Wincor-Nixdorf BA63 Customer Display,
written in C using GTK+.

At present,
the code works but does not have a good serial port selection mechanism.
The serial port is hard-wired at compile time as /dev/ttyUSB0.

## The Displays

The Wincor-Nixdorf BA63 and BA66 are pole-mounted customer displays for an
electronic till or cash register.
They use a serial (RS-232) interface to connect to the PC inside the till.
The serial interface uses odd parity to ensure data integrity.
The BA63 has a two-line vacuum fluorescent display,
and the BA66 has four lines.

The other type of display is the IEE A3282-LB324-301,
an LCD with four lines of text.
It also requires odd parity but runs at 19200 baud.
It's not quite fast enough to keep up with the data stream at that speed though,
and the code incorporates a small delay.
IEE is well-known for making a range of in-line projection displays.

Display selection is via a command-line argument.
The default is the two-line BA63.

## USB Serial Adaptors

Note that some low-cost USB serial adaptor cables fail to work with
the BA63.
The reason for this is not clear, but probably has something to do
with the need for odd parity.
It's possible that the chip(s) inside the cable just don't work properly
when odd parity is selected by the host operating system.

## Building the Program ##

This program uses GTK+ 3.0 for all the GUI elements.
GTK+ is normally pre-installed on modern Linux systems,
but the development libraries and header files are not.
To install them:

```sudo apt-get install libgtk-3-dev```

Once that's installed, simply run 'make':

```make```

