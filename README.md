# ATTiny85_Fuse_Resetter
Want to use all SIX GPIO pins on the ATTiny85?  Now you can.  See Ralph's video
#87 at https://www.youtube.com/c/RalphBacon

So you like the ATTiny85 chip but wish you could use *all* the pins as GPIO
pins?  Your wish is granted!

OK, you have to build (or buy) a fuse resetter but it's a small project that
just about anyone with a soldering iron can successfully complete (Benny built
mine and he's got no thumbs, remember).

If you really don't want to build one for a few dollars you can always buy one
ready made, but at least by watching this video you will understand what it is
doing.

There are some "gotchas" to watch out for (just one or two but definitely worth
knowing about) but when it comes down to it *yes* you *can* have all **six
GPIO** pins working on a Tiny85, as I prove in this video.

I wish I had known this a few years ago as I would now be using my ATTiny85
instead of a Nano to control my fridge door alarm, that's for sure.  Sometimes
a Tiny85 is all you need.  And it can run off batteries for weeks if not months,
as my previous video on the Tiny85 mentioned.

# LINKS LINKS LINKS and MORE LINKS!

High Voltage programming/Unbricking for Attiny - Yikes! Invalid device signature
https://arduinodiy.wordpress.com/2015/05/16/high-voltage-programmingunbricking-for-attiny/

ATTiny85 Spec Sheet (Good Bedtime Reading, really!)
Page 184-185 has graphs showing the current/voltage.
http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf

Embedded Fuse Calculator (for all ATMEL chips)
http://www.engbedded.com/fusecalc/

All about ATMEGA328P (Uno/Nano) Fuse Settings
http://www.martyncurrey.com/arduino-atmega-328p-fuse-settings/

Setting and reading AtTiny85 fuses
https://dntruong.wordpress.com/2015/07/08/setting-and-reading-attiny85-fuses/

You can use `avrdude` (that's the AVR DownandUploadDEr - contrived or what?) to
give you info on fuses too.
Execute these commands:

```avrdude -F -c usbasp -p t85 -U hfuse:r:-:h```
You can remove the `-F` that just skips an invalid signature (like if you have
the RESET pin configured as an IO pin!)

# Using the Programmer

When the device is first switched on the indicator will flash a specific number
of times to denote the current state of the fuses in line with the programming
options available.  For instance, once if the fuse values are non-default but
the RST pin acts as *reset*; twice if the fuse values are non-default and the
RST pin is fused to act as a *GPIO*, and three times if the fuses are at the
factory default values.

Here is an example of the programming in action, with the output captured from
the serial console.  In this example the programming button was held down
continually for three seconds to reset the fuse values from their current
Digispark settings back to the default.

```
Turn on the 12 volt power

Entering programming Mode

Reading signature...1E930B
Reading locks and fuses
hfuse...DD
lfuse...E1
efuse...FE
lock....FF

You can ENABLE the RST pin (as RST) to allow programming
or DISABLE it to turn it into a (weak) GPIO pin.

Press the desired number or hold the button for the desired number of seconds.

1 second  : ENABLE the RST pin (back to normal)
2 seconds : DISABLE the RST pin (make it a GPIO pin)
3 seconds : Revert to factory defaults
1...2...3...

hfuse current=DD target=DF updating...done
lfuse current=E1 target=62 updating...done
efuse current=FE target=FE
lock  current=FF target=FF

Reading locks and fuses
hfuse...DF
lfuse...62
efuse...FE
lock....FF

Programming complete. Press RESET to run again.
```
