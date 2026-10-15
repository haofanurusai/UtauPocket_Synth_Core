"""Compact tabbed parameter editor, independent of model selection."""
import tkinter as tk
from tkinter import ttk
from analysis_params import SPECS, DEFAULTS, validate


class AnalysisParamForm(ttk.Frame):
    def __init__(self,parent,stored,i18n):
        super().__init__(parent)
        self.variables = {}
        book = ttk.Notebook(self)
        book.pack(fill='both',expand=True)
        for group,label in [('bounds', 'analysis_group_bounds'), ('features', 'analysis_group_features'), ('objective', 'analysis_group_objective')]:
            page = ttk.Frame(book,padding=8)
            page.columnconfigure(1,weight=1)
            book.add(page,text=i18n.get(label))
            rows = [spec for spec in SPECS if spec[1]==group]
            for row,(key,_,label,default,_) in enumerate(rows):
                ttk.Label(page,text=i18n.get(label)).grid(row=row,column=0,sticky='w',padx=4,pady=4)
                var = tk.StringVar(value=str(stored.get(key,default)))
                self.variables[key] = var
                ttk.Entry(page,textvariable=var,width=16).grid(row=row,column=1,sticky='ew',padx=8)
        ttk.Label(self,text=i18n.get('weights_are_used_as_entered_without_normalization_affect_automatic_bounds_only'),
                  wraplength=590).pack(anchor='w',pady=6)
        ttk.Button(self,text=i18n.get('reset_all_loop_parameters'),command=self.reset).pack(anchor='e')

    def reset(self):
        for key,var in self.variables.items():
            var.set(str(DEFAULTS[key]))

    def values(self):
        return validate({key:var.get() for key,var in self.variables.items()})
