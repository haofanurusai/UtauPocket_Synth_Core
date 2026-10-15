"""Integer MIDI controller sliders, grouped by core envelope."""
from tkinter import ttk
from midi_controls import GROUPS
from numeric_control import NumericControl


class CCPanel(ttk.Notebook):
    def __init__(self,parent,settings,changed,i18n):
        super().__init__(parent)
        self.controls = {}
        stored = settings.get('cc',{})
        titles = dict(main=i18n.get('main_glide'),fm=i18n.get('fm'),
                      am=i18n.get('am'),speed=i18n.get('speed'),voice=i18n.get('voice'))
        labels = dict(A=i18n.get('attack_a'),P=i18n.get('peak_p'),
                      D=i18n.get('decay_d'),S=i18n.get('sustain_s'),R=i18n.get('release_r'),
                      Glide=i18n.get('glide'),Period=i18n.get('period'),Depth=i18n.get('depth'),
                      Consonant=i18n.get('cc_consonant'),Vowel=i18n.get('vowel'),
                      Volume=i18n.get('volume'),Nyquist=i18n.get('nyquist_multiplier'),
                      AperiodicGain=i18n.get('aperiodic_gain'),Openness=i18n.get('openness_rd'),
                      LipRadius=i18n.get('lip_radius'))
        for group, controls in GROUPS:
            frame = ttk.Frame(self,padding=8)
            frame.columnconfigure(1,weight=1)
            self.add(frame,text=titles[group])
            for row,(label,cc,default) in enumerate(controls):
                try:
                    value = max(0,min(127,int(stored.get(str(cc),default))))
                except (ValueError,TypeError):
                    value = default
                ttk.Label(frame,text=f'{labels.get(label,label)} · CC{cc}',width=20).grid(row=row,column=0,sticky='w')
                control = NumericControl(frame,value,0,127,1,
                                         changed=lambda value,c=cc:changed(c,value),spin=True)
                self.controls[cc] = control
                control.slider.grid(row=row,column=1,sticky='ew')
                control.editor.grid(row=row,column=2,padx=5)

    def values(self):
        return {cc:control.commit() for cc,control in self.controls.items()}
