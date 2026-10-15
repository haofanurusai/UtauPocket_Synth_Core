"""Modal options only; analysis itself runs in analysis_job."""
from pathlib import Path
from analysis_param_form import AnalysisParamForm
import tkinter as tk
from tkinter import ttk, filedialog, messagebox


class AnalysisDialog(tk.Toplevel):
    def __init__(self, editor):
        super().__init__(editor)
        self.result = None
        i18n = editor.i18n
        self.title(i18n.get('contentvec_analysis'))
        self.transient(editor)
        self.columnconfigure(1,weight=1)
        self.path = tk.StringVar(value=editor.app.settings.get('contentvec_path',''))
        self.gpu, self.auto = tk.BooleanVar(value=False), tk.BooleanVar(value=False)
        self.scope = tk.StringVar(value='current')
        ttk.Label(self,text=i18n.get('model_path')).grid(row=0,column=0,padx=8,pady=10)
        ttk.Entry(self,textvariable=self.path,width=55).grid(row=0,column=1,sticky='ew')
        def browse():
            path = filedialog.askopenfilename(parent=self,filetypes=[('ContentVec','*.pt'),(i18n.get('all_files'),'*')])
            if path:
                self.path.set(path)
        ttk.Button(self,text='…',command=browse).grid(row=0,column=2,padx=8)
        ttk.Checkbutton(self,text=i18n.get('try_gpu_fall_back_to_cpu'),variable=self.gpu).grid(row=1,column=0,columnspan=3,sticky='w',padx=8)
        ttk.Checkbutton(self,text=i18n.get('also_set_loop_bounds_replace_working_values_undoable'),variable=self.auto).grid(row=2,column=0,columnspan=3,sticky='w',padx=8,pady=8)
        scope = ttk.Frame(self)
        scope.grid(row=3,column=0,columnspan=3,sticky='w',padx=8)
        for value,label in [('current',i18n.get('current')),('selected',i18n.get('selected')),('all',i18n.get('all'))]:
            ttk.Radiobutton(scope,text=label,variable=self.scope,value=value).pack(side='left',padx=8)
        ttk.Label(self,text=i18n.get('analyzes_the_entire_cropped_wav_initial_model_loading_may_take_a_while')).grid(row=4,column=0,columnspan=3,padx=8,pady=8)
        self.parameters = AnalysisParamForm(self,editor.app.settings.get('loop_analysis',{}),i18n)
        self.parameters.grid(row=5,column=0,columnspan=3,sticky='ew',padx=8)
        def accept():
            try:
                parameters = self.parameters.values()
            except ValueError as error:
                messagebox.showerror(i18n.get('invalid_parameters'),str(error),parent=self)
                return
            if not Path(self.path.get()).is_file():
                messagebox.showerror('ContentVec',i18n.get('choose_an_existing_model_file'),parent=self)
                return
            self.result = (self.path.get(),self.gpu.get(),self.auto.get(),self.scope.get(),parameters)
            self.destroy()
        ttk.Button(self,text=i18n.get('analyze'),command=accept).grid(row=6,column=1,pady=12)
        self.grab_set()
        self.wait_window()
