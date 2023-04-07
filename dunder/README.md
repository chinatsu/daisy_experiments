# dunder

```bash
make all
# set daisy into bootloader mode
make program-dfu
```

## Author

cn :)

## Description

A simple step sequencer and drum machine based on [EuclideanDrums](https://github.com/electro-smith/DaisyExamples/tree/master/pod/EuclideanDrums).
The sequencer's length is set to 16 steps.

## Controls

The encoder button changes modes, and the current mode can be seen in LED 1.

### Edit mode

If LED 1 is red, we're in *edit mode*.

| Control  | Description                                    |
|----------|------------------------------------------------|
| Encoder  | Moves the edit cursor along the step sequencer |
| Button 1 | Places a kick trigger at the cursor            |
| Button 2 | Places a snare trigger at the cursor           |


### Play mode

If LED 1 is blue, we're in *play mode*.
In this mode, various attributes can be changed with the knobs.
LED 2 indicates which attributes we're changing, or which "inner mode" we're in.

| Control | Description            |
|---------|------------------------|
| Encoder | Changes the inner mode |

#### Kick

If LED 2 is off, we're in *kick mode*.

| Control | Description                               |
|---------|-------------------------------------------|
| Knob 1  | Changes the kick volume envelope's decay  |
| Knob 2  | Changes the kick volume envelope's attack |


#### Snare

If LED 2 is at 20% brightness, we're in *snare mode*.

| Control | Description                         |
|---------|-------------------------------------|
| Knob 1  | Changes the snare envelope's decay  |
| Knob 2  | Changes the snare envelope's attack |

#### Reverb

If LED 2 is at 40% brightness, we're in *reverb mode*.

| Control | Description                 |
|---------|-----------------------------|
| Knob 1  | Changes the reverb feedback |
| Knob 2  | Changes the reverb cutoff   |

#### Overdrive

If LED 2 is at 60% brightness, we're in *overdrive mode*.
Although a reverb attribute is also modified here, I know it's a bit confusing.

| Control | Description                      |
|---------|----------------------------------|
| Knob 1  | Changes the overdrive amount     |
| Knob 2  | Changes the reverb wet/dryness   |


#### Tempo

If LED 2 is at 80% brightness, we're in *tempo mode*.

| Control | Description       |
|---------|-------------------|
| Knob 1  | Changes the tempo |


