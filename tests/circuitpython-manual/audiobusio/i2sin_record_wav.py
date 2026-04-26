"""Record I2S audio to a WAV file using the adafruit_wave library.

To run:
1. Copy adafruit_wave.py from
     Adafruit_CircuitPython_Wave/adafruit_wave.py
   onto the CIRCUITPY drive's /lib/ directory.
2. Make CIRCUITPY writable from code.py by creating boot.py with:
       import storage
       storage.remount("/", readonly=False)
   then power-cycle. Or write to an SD card mounted via sdcardio.

Strategy: record the entire clip into RAM first, then write the WAV.
Writing to flash mid-capture stalls the loop long enough that the I2S DMA
ring overruns, which scrambles timing and drops samples.

Memory note: on a plain ESP32 (e.g. Adafruit Sparkle Motion) free RAM is
about 100 KB after import. The buffer needs sample_rate * duration * 2
bytes for 16-bit mono. Reduce DURATION_S to fit.
  16 kHz * 1 s = 32 KB   (works repeatedly)
  16 kHz * 2 s = 64 KB   (works after a fresh boot, may MemoryError later)

Note on sample rate: I2S MEMS microphones (SPH0645, INMP441, etc.) have a
minimum BCLK frequency (~1 MHz) below which they enter sleep mode and emit
silence. At 16-bit slots, sample_rate=16000 produces BCLK=512 kHz which is
already at the low end; dropping to 8000 (BCLK=256 kHz) silences most mics.
Keep SAMPLE_RATE at 16000 or higher for these mics. To get longer
recordings on a RAM-constrained board, stream to an SD card instead.
"""

import array
import gc
import time

import audiobusio
import adafruit_wave
import board

SAMPLE_RATE = 16000
BIT_DEPTH = 16
N_CHANNELS = 1
DURATION_S = 1
N_SAMPLES = SAMPLE_RATE * DURATION_S
OUT_PATH = "/recording.wav"


def free_mem():
    gc.collect()
    return gc.mem_free()


print("free before:", free_mem())

mic = None
samples = None
warmup = None
try:
    # Allocate from zero-bytes rather than [0] * N — the list form needs ~8 bytes
    # per element transiently, which on a plain ESP32 can exceed total free RAM
    # after one run's worth of fragmentation.
    samples = array.array("H", bytes(2 * N_SAMPLES))

    mic = audiobusio.I2SIn(
        bit_clock=board.D26,
        word_select=board.D33,
        data=board.D25,
        sample_rate=SAMPLE_RATE,
        bit_depth=BIT_DEPTH,
        mono=True,
        left_justified=False,
    )

    # Discard one chunk so the mic's internal filters can settle.
    warmup = array.array("H", bytes(2 * 1000))
    mic.record(warmup, len(warmup))
    warmup = None
    gc.collect()

    print("recording {} s ({} samples) into RAM...".format(DURATION_S, N_SAMPLES))
    start = time.monotonic()
    got = mic.record(samples, N_SAMPLES)
    elapsed = time.monotonic() - start

    measured_rate = got / elapsed if elapsed > 0 else 0
    print("captured {} samples in {:.3f} s".format(got, elapsed))
    print("configured rate: {} Hz".format(SAMPLE_RATE))
    print("measured  rate: {:.1f} Hz".format(measured_rate))
    print("ratio (configured / measured): {:.3f}".format(SAMPLE_RATE / measured_rate))

    # Free the I2S peripheral before opening the file — its DMA buffers
    # are reclaimed here, freeing room for the WAV writer.
    mic.deinit()
    mic = None
    gc.collect()

    print("free before write:", gc.mem_free())
    print("writing", OUT_PATH)
    # adafruit_wave.writeframesraw accepts a memoryview directly, so we don't
    # need a bytes() copy (which would double peak RAM).
    view = memoryview(samples)[:got]
    with adafruit_wave.open(OUT_PATH, "wb") as w:
        w.setnchannels(N_CHANNELS)
        w.setsampwidth(BIT_DEPTH // 8)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(view)
    view = None

    print("done")
finally:
    # Always release hardware + buffers, even on error, so a re-run after
    # a failure does not leak the I2S peripheral or the sample buffer.
    if mic is not None:
        try:
            mic.deinit()
        except Exception:
            pass
    mic = None
    samples = None
    warmup = None
    gc.collect()
    print("free after:", gc.mem_free())
