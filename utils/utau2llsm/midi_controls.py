"""The CC numbers used by this core, not General MIDI parameter names."""
GROUPS = [
    ('main', [('Volume',7,127), ('Glide',5,0), ('A',73,0), ('P',76,127), ('D',75,0), ('S',79,112), ('R',72,6)]),
    ('fm', [('A',102,0), ('P',103,127), ('D',104,0), ('S',105,127), ('R',106,0),
            ('Period',107,64), ('Depth',108,0)]),
    ('am', [('A',109,0), ('P',110,127), ('D',111,0), ('S',112,127), ('R',113,0),
            ('Period',114,64), ('Depth',115,0)]),
    ('speed', [('Consonant',116,64), ('Vowel',117,64)]),
    ('voice', [('Nyquist',118,64), ('AperiodicGain',119,64), ('Openness',20,64), ('LipRadius',21,64)])]
DEFAULTS = {cc: value for _, controls in GROUPS for _, cc, value in controls}
