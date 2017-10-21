% Loeffler *H*


# SuperCollider class

## Basic usage

```
a = LoefflerH.init;
a.gui;
a.play;             // play all
a.play(10);         // play from bar 10
a.play(10, 15);     // play from bar 10 to the end of bar 15
a.play(rate: 85);   // play at 85% tempo
a.stop;
a.free;
```

## Advanced usage

```
a.putArduino(0, [208, 0]); // scale test on motor zero
```

## Action syntax

See `score.csv` for examples.

```
command [halfturns] direction pitch[-endpitch] [duration] [recentre]
```

### Commands

- `t` = turn            (halfturns, direction, pitch)
- `d` = duration turn   (direction, pitch, duration, recentre)
- `g` = duration gliss  (direction, pitch, endpitch, duration, recentre)
- `T` = timed turn      (halfturns, direction, duration)
- `G` = gliss           (halfturns, direction, pitch, endpitch)

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
