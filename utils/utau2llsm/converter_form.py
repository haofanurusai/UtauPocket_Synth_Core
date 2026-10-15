"""Conversion dialog widgets; no conversion work is done on the Tk thread."""
from pathlib import Path
from numeric_control import NumericControl
import tkinter as tk
from tkinter import ttk, filedialog

DEFAULTS = dict(loop_start_pct=20., loop_end_pct=80., fmin=50, fmax=800,
                trange=24, har=400, har_e=5, bias=2., bap=5, hop=128, fft=2048, psd=128, spec=64)


RANGES = dict(loop_start_pct=(0,100), loop_end_pct=(0,100), fmin=(20,20000),
              fmax=(20,20000), trange=(1,96), har=(1,2000), har_e=(1,100),
              bias=(.01,10), bap=(1,50))

def build_form(app):
    frame = ttk.Frame(app, padding=12)
    frame.pack(fill='both', expand=True)
    frame.columnconfigure(1, weight=1)
    app.vars = {}
    app.numeric_controls = []
    row = 0
    for key, label, directory in [('utau_dir','lbl_utau_dir',True),
                                  ('csv_path','lbl_csv_path',False), ('out_dir','lbl_out_dir',True)]:
        default = str(Path(__file__).with_name('new_geping.csv')) if key == 'csv_path' else ''
        var = tk.StringVar(value=app.settings.get(key, default))
        app.vars[key] = var
        ttk.Label(frame, text=app.i18n.get(label)).grid(row=row,column=0,sticky='e',pady=5)
        ttk.Entry(frame, textvariable=var).grid(row=row,column=1,sticky='ew',padx=8)
        def browse(v=var, d=directory):
            value = filedialog.askdirectory(parent=app) if d else filedialog.askopenfilename(parent=app)
            if value:
                v.set(value)
        ttk.Button(frame,text=app.i18n.get('btn_browse'),command=browse).grid(row=row,column=2)
        row += 1
    for key, default in DEFAULTS.items():
        value = app.settings.get(key, default)
        label = {'loop_start_pct':'lbl_loop_start','loop_end_pct':'lbl_loop_end'}.get(key, 'lbl_'+key)
        ttk.Label(frame,text=app.i18n.get(label)).grid(row=row,column=0,sticky='e',pady=4)
        if key in ('hop','fft','psd','spec'):
            var = tk.IntVar(value=value)
            widget = ttk.Combobox(frame,textvariable=var,values=[2**i for i in range(5,13)],state='readonly')
        else:
            minimum,maximum = RANGES[key]
            control = NumericControl(frame,value,minimum,maximum,1 if isinstance(default,int) else .01)
            app.numeric_controls.append(control)
            var,widget = control.number,control.slider
            control.editor.grid(row=row,column=2)
        app.vars[key] = var
        widget.grid(row=row,column=1,sticky='ew',padx=8)
        row += 1
    app.convert_btn = ttk.Button(frame,text=app.i18n.get('btn_convert'),command=app.on_convert)
    app.convert_btn.grid(row=row,column=0,columnspan=3,sticky='ew',pady=8)
    app.progress = ttk.Progressbar(frame, maximum=100)
    app.progress.grid(row=row+1,column=0,columnspan=3,sticky='ew')
    app.status = tk.StringVar()
    ttk.Label(frame,textvariable=app.status,wraplength=520).grid(row=row+2,column=0,columnspan=3,sticky='w')
    return frame
