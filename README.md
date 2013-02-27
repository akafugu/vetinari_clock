VETINARI CLOCK
==============

Introduction
---

A clock that ticks randomly, but still keeps accurate time!

The idea comes from the Discworld series of books by Sir Terry Pratchett.
Lord Havelock Vetinari is a recurring character in the book series, and he
is in possesion of a very strange clock.

From wikipedia:

"Lord Vetinari also has a strange clock in his waiting-room. While it does keep completely accurate time overall, it sometimes ticks and tocks out of sync (example: "tick, tock... ticktocktick, tock...") and occasionally misses a tick or tock altogether, which has the net effect of turning one's brain "into a sort of porridge". (Feet of Clay, Going Postal). In Feet of Clay Vimes observes that it must have cost him quite a lot of money."

Design files
---

Cad files for Eagle (version 6 or newer) are available in this repository.

Full kits with all required components are also available directly from
[Akafugu Corporation](http://akafugu.jp/posts/products/vetinariclock)

Source code
---

The firmware is a straight port of Simon Inns' original version for PIC12F683
to Attiny25.

The microcontroller is run from a 32kHz crystal, with Timer 1 set up to give
4 overflows each second.

From the original explanation on the webpage:

The firmware has a 'random' sequence of pulses which over 32 seconds moves the
second hand 32 times. This sequence is long enough to give the appearance of
randomness without unnecessarily consuming processor cycles. I chose 32 seconds
as it means the pattern is constantly offset from the 60 second rotation of the
minutes. The 'random' pattern is 128 steps (and is therefore checked 4 times a
second). This means the clock moves between 0-4 times within each actual second.
By moving more or less in each of the 32 seconds the clock looks random, however
the sequence always moves the clock exactly 32 seconds in the 128 steps.

Attribution
---

Original project and code for PIC PIC12F683 by Simon Inns:

http://www.waitingforfriday.com/index.php/Vetinari%27s_Clock

avr-gcc port for Attiny25 by Akafugu Corporation:

http://akafugu.jp/
