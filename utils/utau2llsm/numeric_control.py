"""Separate editable text from the last committed slider value."""
import math
import tkinter as tk
from tkinter import ttk


class NumericValue:
    def __init__(self,parent,value,minimum,maximum,resolution,changed=lambda value:None):
        self.minimum,self.maximum,self.resolution = minimum,maximum,resolution
        self.changed = changed
        self.value = self.normalize(value)
        variable = tk.IntVar if resolution == 1 else tk.DoubleVar
        self.number = variable(master=parent,value=self.value)
        self.text = tk.StringVar(master=parent,value=self.format(self.value))

    def normalize(self,value):
        value = float(value)
        if not math.isfinite(value):
            raise ValueError('Non-finite number')
        value = max(self.minimum,min(self.maximum,value))
        value = max(self.minimum,min(self.maximum,round(value/self.resolution)*self.resolution))
        return int(value) if self.resolution == 1 else value

    def format(self,value):
        return str(value) if self.resolution == 1 else format(value,'.12g')

    def commit(self):
        try:
            value = self.normalize(self.text.get())
        except (ValueError,OverflowError):
            value = self.value
        self.apply(value)
        self.text.set(self.format(self.value))
        return self.value

    def apply(self,value):
        if value != self.value:
            self.value = value
            self.number.set(value)
            self.changed(value)

    def from_slider(self,raw):
        value = self.normalize(raw)
        # Tk can deliver a queued callback after programmatic synchronization.
        # Do not replace newer text or emit a second MIDI event for that value.
        if value == self.value:
            return
        self.apply(value)
        self.text.set(self.format(self.value))


class NumericControl(NumericValue):
    def __init__(self,parent,value,minimum,maximum,resolution,changed=lambda value:None,spin=False):
        super().__init__(parent,value,minimum,maximum,resolution,changed)
        self.slider = tk.Scale(parent,from_=minimum,to=maximum,resolution=resolution,
                               orient='horizontal',variable=self.number,showvalue=False,
                               command=self.from_slider,highlightthickness=0)
        if spin:
            self.editor = ttk.Spinbox(parent,from_=minimum,to=maximum,increment=resolution,
                                      textvariable=self.text,width=5,command=self.commit)
        else:
            self.editor = ttk.Entry(parent,textvariable=self.text,width=8)
        self.editor.bind('<Return>',self.on_return)
        self.editor.bind('<FocusOut>',lambda event:self.commit())

    def on_return(self,event):
        self.commit()
        return 'break'
