"""One schema for loop-analysis defaults, validation and localized controls."""
import math
from i18n import get

# key, group, label key, default, strictly positive
SPECS = [
 ('min_loop_ms', 'bounds', 'analysis_min_loop_ms', 120.0, True),
 ('min_f0_hz', 'bounds', 'analysis_min_f0_hz', 60.0, False),
 ('min_rd', 'bounds', 'analysis_min_rd', 0.0, False),
 ('power_threshold', 'bounds', 'analysis_power_threshold', 0.25, False),
 ('tail_weight', 'objective', 'analysis_tail_weight', 0.5, False),
 ('cv_weight', 'features', 'analysis_cv_weight', 0.7, False),
 ('f0_weight', 'features', 'analysis_f0_weight', 0.05, False),
 ('rd_weight', 'features', 'analysis_rd_weight', 0.25, False),
 ('cv_scale', 'features', 'analysis_cv_scale', 0.02, True),
 ('f0_scale', 'features', 'analysis_f0_scale', 0.5, True),
 ('rd_scale', 'features', 'analysis_rd_scale', 0.15, True),
 ('variance_weight', 'objective', 'analysis_variance_weight', 0.55, False),
 ('rate_weight', 'objective', 'analysis_rate_weight', 0.3, False),
 ('edge_weight', 'objective', 'analysis_edge_weight', 0.15, False),
 ('length_weight', 'objective', 'analysis_length_weight', 0.1, False),
 ('rate_reference_ms', 'objective', 'analysis_rate_reference_ms', 20.0, True),
 ('edge_window_ms', 'objective', 'analysis_edge_window_ms', 20.0, True),
]
DEFAULTS = {key:default for key,_,_,default,_ in SPECS}


def validate(values):
    result = {}
    for key,_,label,_,positive in SPECS:
        try:
            value = float(values[key])
        except (ValueError,TypeError,KeyError):
            raise ValueError(get(label)+get('enter_a_number')) from None
        if not math.isfinite(value) or (value <= 0 if positive else value < 0):
            raise ValueError(get(label)+get('must_be_finite_and_positive' if positive else 'must_be_finite_and_nonnegative'))
        result[key] = value
    if result['power_threshold'] >= 1:
        raise ValueError(get('relative_rms_threshold_must_be_below_1'))
    for keys in [('cv_weight','f0_weight','rd_weight'),
                 ('variance_weight','rate_weight','edge_weight')]:
        if sum(result[k] for k in keys) == 0:
            raise ValueError(get('feature_and_stability_weights_each_need_a_positive_member'))
    return result
