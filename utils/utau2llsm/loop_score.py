"""Prefix-sum interval variance and time-scaled motion for ping-pong loops."""
import numpy as np

from analysis_params import DEFAULTS, validate


class LoopScore:
    def __init__(self, features, f0, rd, dt, params=None):
        self.params = p = validate(DEFAULTS if params is None else params)
        self.dt = dt
        norms = np.linalg.norm(features, axis=1, keepdims=True)
        unit = features / np.maximum(norms, 1e-12)
        pitch = 12*np.log2(np.maximum(f0,1e-12))
        # Configurable weighted feature metric.
        z = np.column_stack((unit*np.sqrt(p['cv_weight']/p['cv_scale']),
                             pitch*np.sqrt(p['f0_weight'])/p['f0_scale'],
                             rd*np.sqrt(p['rd_weight'])/p['rd_scale']))
        z -= z.mean(axis=0)  # Reduce cancellation in the variance calculation.
        self.sums = np.vstack((np.zeros(z.shape[1]),np.cumsum(z,axis=0)))
        self.squares = np.r_[0,np.cumsum(np.sum(z*z,axis=1))]
        motion = np.sum(np.diff(z,axis=0)**2,axis=1)*(p['rate_reference_ms']/1000/dt)**2
        self.motion = np.r_[0,np.cumsum(motion)]
        self.edge_frames = max(1,round(p['edge_window_ms']/1000/dt))

    def evaluate(self, starts, end):
        count = end-starts+1
        means = (self.sums[end+1]-self.sums[starts])/count[:,None]
        variance = np.maximum(0,(self.squares[end+1]-self.squares[starts])/count
                              - np.sum(means*means,axis=1))
        rate = (self.motion[end]-self.motion[starts])/(count-1)
        width = np.minimum(self.edge_frames,count-1)
        edges = ((self.motion[starts+width]-self.motion[starts])
                 + (self.motion[end]-self.motion[end-width]))/(2*width)
        duration = (count-1)*self.dt
        p = self.params
        return (p['variance_weight']*variance + p['rate_weight']*rate
                + p['edge_weight']*edges + p['length_weight']*p['min_loop_ms']/1000/duration)
