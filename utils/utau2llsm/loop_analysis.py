"""Whole-interval loop search; no sample-phase snapping of frame indices."""
from i18n import get
import numpy as np
from loop_score import LoopScore
from analysis_params import DEFAULTS, validate


def find_loop(audio, fs, hop, f0, rd, features, consonant=0, cancel=None, params=None):
    p = validate(DEFAULTS if params is None else params)
    f0, rd, features = [np.asarray(x,dtype=np.float64) for x in (f0,rd,features)]
    n = len(f0)
    if (fs <= 0 or hop <= 0 or f0.shape != (n,) or rd.shape != (n,)
            or features.shape != (n,768) or not 0 <= consonant < n
            or not all(np.isfinite(x).all() for x in (audio,f0,rd,features))):
        raise ValueError(get('invalid_loop_analysis_shapes_values_or_frame_parameters'))
    dt = hop/fs
    minimum = max(1,int(np.ceil(p['min_loop_ms']/1000/dt)))
    power = np.array([np.sqrt(np.mean(audio[k*hop:min((k+1)*hop,len(audio))]**2))
                      if k*hop < len(audio) else 0 for k in range(n)])
    valid = ((power > p['power_threshold']*max(float(power.max()),1e-12)) & (f0 > 0) & (f0 >= p['min_f0_hz']) & (rd > p['min_rd'])
             & (np.linalg.norm(features,axis=1) > 1e-12) & (np.arange(n) >= consonant))
    # Every frame must be valid; never bridge a silent/unvoiced gap.
    invalid = np.r_[0,np.cumsum(~valid)]
    score = LoopScore(features,f0,rd,dt,p)
    active = np.flatnonzero(valid)
    last = int(active[-1]) if len(active) else 0
    best, pair = float('inf'), None
    for end in range(consonant+minimum,n):
        if cancel is not None and cancel.is_set():
            raise InterruptedError(get('analysis_cancelled'))
        if not valid[end]:
            continue
        starts = np.arange(consonant,end-minimum+1)
        starts = starts[invalid[end+1]-invalid[starts] == 0]
        if not len(starts):
            continue
        costs = score.evaluate(starts,end) + p['tail_weight']*(last-end)/n
        index = int(np.argmin(costs))
        if costs[index] < best:
            best, pair = float(costs[index]), (int(starts[index]),end)
    if pair is None:
        raise ValueError(get('no_continuous_valid_vowel_interval_meets_the_thresholds_and_minimum_duration'))
    return pair
