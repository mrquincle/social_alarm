# Social alarm

A social alarm is a [short-range device](https://en.wikipedia.org/wiki/Short-range_device).

It operates on 868.20-868.25 MHz.

# SDR

First we get the data using a software-defined radio. The data itself is too large to upload here.

# Analyze in gnuradio

![Schema](https://raw.githubusercontent.com/mrquincle/social_alarm/main/images/schema_threshold.png)

This will lead to data like this:

![Data](https://raw.githubusercontent.com/mrquincle/social_alarm/main/images/waveform_threshold.png)

You see it just shifted in time a bit (too lazy to add a delay to match them), but that they match pretty well.

The data from `decoded_message.bin` can subsequently be processed by the C file in `/decoder`. Just install `clang` and run `make`.

# Replay attack

The hackrf uses interleaved 8-bit data. Record through something like:

```
hackrf_transfer -r alarm869300000.wav -f 869300000
```

This can be replayed with:

```
hackrf_transfer -d 0000000000000000088869dc2532111b -t alarm869300000.raw -f 869300000 -x 20
```

The file is binary (s8) format.

```
# hd alarm869300000.raw | head -n2
00000000  02 02 01 02 01 02 01 02  02 02 01 02 01 02 02 02  |................|
00000010  01 02 02 02 01 02 01 02  01 02 01 02 01 02 01 02  |................|
```

The default sample rate of the hackrf is 10MHz.

To go from these signed bytes to something gnuradio can fathom, we first split this file in pieces of 5M.

```
split -b5m ../alarm869300000.raw s8alarm_
```

We can play a few, but the one we like to have is `s8alarm_at`.

Use `sox` to create floating point values of 32-bit for gnuradio.

```
sox -r 10000k -c 2 -t s8 s8alarm_at -t f32 -r 500k result.f32
```

# Data

The `500k_alarm_recorded` file when parsed contains two times the same sequence.

This might very well have been recorded at 869.2 MHz because the frequencies are slightly different and I have forgotten...


```
# hd 500k_alarm_recorded | head -n2
00000000  00 00 00 3c 00 00 c0 bc  00 00 80 3c 00 00 c0 bc  |...<.......<....|
00000010  00 00 80 3c 00 00 c0 bc  00 00 80 3c 00 00 c0 bc  |...<.......<....|
```

A zero or one seems to take 0.5ms, 2 kHz.

The zero seems 2.5 sine wave, say 5 kHz, the one is 25(?) sine waves, say 50 kHz.

Okay, that would mean: 869.205 kHz versus 869.250 kHz.

