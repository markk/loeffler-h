% Loeffler *H*


# SuperCollider class

## Basic usage

```
a = LoefflerH.init;
a.play;             // play all
a.play(10);         // play from bar 10
a.play(10, 15);     // play from bar 10 to the end of bar 15
a.play(rate: 85);   // play at 85% tempo
a.stop;
a.free;
```

## Advanced usage

```
a = LoefflerH.init(false); // no GUI
a.putArduino(0, [209, 0]); // scale test on motor zero
```

## Action syntax

See `score.csv` for examples.

```
command [halfturns:H] [dir:l|r] pitch:P[-P][-P] [duration:D][-D] [recentre:c|n]
```

### Commands

- `t` — turn, e.g. `t 1 l 69` =
  one halfturn left at pitch 69.
- `d` — duration turn, e.g. `d r 60 3 n` =
  right turning for 3 beats at pitch 60, stop without finding sensor.
- `g` — duration gliss, e.g. `g r 57-69 1.5 c` =
  right turning gliss from pitch 57-69 over 1.5 beats, find sensor afterwards.
- `q` — double gliss, e.g. `q l 48-60-54 1-0.33 n` =
  left turning double gliss from pitch 48-60 over 1 beat, then from
  pitch 60-54 over 1/3 beat, stop without finding sensor.
- `T` — timed turn, e.g. `T 2 r 72 3` =
  two halfturns right over 3 beats. **N.B.** the pitch number is necessary but ignored.
- `G` — gliss, e.g. `G 3 l 66-60` =
  gliss from pitch 66-60 in 3 halfturns left. Duration is indeterminate.

### Shortcut

- `h` = single halfturn at fastest speed, this is equivalent to `t 1 r 72`

### Halfturns

- 1-19 number of halfturns to complete

### Directions

- l = left
- r = right

### Pitches (motor speed)

- 36-72 (including quartertones, e.g. 40.5)

### Duration

- in beats at current tempo

### Recentre

- `c` = recentre
- `n` = do not recentre
