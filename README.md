# noobhour modules

## Baseliner
A probabilistic attenuator switch.

![Baseliner](./doc/Baseliner.png)

Baseliner returns one of two possible signals ("Signal" or "Base"),
based on a gate input. In its basic state, it will return Base if the
gate is inactive and Signal otherwise. Since Base defaults to 0, this
is simply an AND gate. 

However, when selecting and shaping a modulator signal for further
use, there are recurring tasks that are beyond the simple AND gate,
covered by Baseliner:
- Base can be tied to an input just like Signal in case the value for
  gate off shouldn't be 0.
- Both signals can be offset and attenuated to shape a CV for further
  processing - the final output is input * att + abs.
- In Toggle mode, it acts like a 2:1 switch.
- Changing probability p via parameter or input adds random elements
  to the processing of the gate.

### Modes and Probabilities 

Besides the regular Gate mode, there are two modes, Latch and Toggle,
which behave like the modes in Audible Instruments' Bernoulli Gate
(Mutable Instruments Branches).

In Gate mode, Signal is returned if Gate is on - but only with
probability p, determined each time Gate triggers (switches from off
to on).

In Latch mode, Gate is only used as a trigger: When it triggers, the
output switches to Signal with probability p or to Base otherwise.

In Toggle mode, Gate is only used as a trigger as well: When it
triggers, the output switches from Base to Signal or from Signal to
Base with probability p.








