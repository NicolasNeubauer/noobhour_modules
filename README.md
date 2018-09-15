# noobhour modules
A collection of one module for [VCV Rack](https://github.com/VCVRack/Rack).

## Baseliner
A 4x probabilistic attenuverting switch. 

![Baseliner](./doc/Baseliner.png)

Baseliner returns one of two possible signals ("High" or "Low"), based
on a gate input. In its basic state, it will return the High signal if
the gate is active and Low otherwise. 

"I see that Low defaults to 0, so are you saying you re-invented the
AND gate?" Yea, kind of (awesome, no?). But there are a number of ways
to modify this basic behaviour which I often find useful in turning
raw CV into music. They relate to shaping the High and Low signals,
and to deciding which signal to return.

### Signal shaping 

The original motivation for this module was to pass on a signal if
gate is on, but have it fall back to a definable baseline, not just 0,
when it's off, hence its name. 

The ways the two signals can be shaped are the same for both; the only
difference is when they are returned. Both High and Low are computed
as their respective input * att + abs. If no input is given, this
lets you dial in a constant value via abs (-5V..5V).  Once you provide
an input, you can offset it using abs and attenuate it via att, which
is an "attenuverter" i.e. also lets you invert the signal as its value
can be set from -1V to 1V.


### High or Low? - Modes and Probabilities 

The controls in the darker, lower area of the module can be used to
modify which signal is returned when.

Besides the regular Gate mode, there are two modes, Latch and Toggle,
which behave like the modes in
[Audible Instruments](https://github.com/VCVRack/AudibleInstruments)
Bernoulli Gate (a software implementation of
[Mutable Instruments Branches](https://mutable-instruments.net/modules/branches/)).
There is also a probability input which is computed as the sum of the
knob value (0..1) and the CV input.

- In Gate mode, High is returned if Gate is on - but only with
  probability p, determined each time Gate triggers (switches from off
  to on).

- In Latch mode, Gate is only used as a trigger: When it triggers, the
  output switches to High with probability p or to Low otherwise.

- In Toggle mode, Gate is only used as a trigger as well: When it
  triggers, the output switches from Low to High or from High to
  Low with probability p.








