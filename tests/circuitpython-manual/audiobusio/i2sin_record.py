import array
import math
import time

import audiobusio
import board


def mean(values):
    return sum(values) / len(values)


def normalized_rms(values):
    avg = mean(values)
    samples_sum = sum(float(sample - avg) * (sample - avg) for sample in values)
    return math.sqrt(samples_sum / len(values))


SAMPLE_RATE = 16000
BIT_DEPTH = 16
N_SAMPLES = 16000  # ~1 second at 16 kHz

samples = array.array("H", [0] * N_SAMPLES)

mic = audiobusio.I2SIn(
    bit_clock=board.D9,
    word_select=board.D10,
    data=board.D11,
    sample_rate=SAMPLE_RATE,
    bit_depth=BIT_DEPTH,
    mono=True,
    left_justified=False,
)

print("starting read")
start = time.monotonic()
count = mic.record(samples, len(samples))
elapsed = time.monotonic() - start
print("read done in {:.3f} s".format(elapsed))
print("recorded {} samples".format(count))

view = samples[:count]
print("min", min(view))
print("max", max(view))
print("mean", mean(view))
print("rms", normalized_rms(view))

print("sample_rate", mic.sample_rate)
print("bit_depth", mic.bit_depth)

mic.deinit()
print("done")
