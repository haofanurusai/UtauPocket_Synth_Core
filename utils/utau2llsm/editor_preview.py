"""Preview UI and requests; core state belongs to preview_worker."""
from i18n import get
from tkinter import ttk, filedialog
import tkinter as tk
from cc_panel import CCPanel


class PreviewPanel(ttk.Frame):
    def __init__(self,editor,parent):
        super().__init__(parent)
        self.editor, self.i18n = editor, editor.i18n
        settings = editor.app.settings
        row = ttk.Frame(self)
        row.pack(fill='x',pady=4)
        self.note = tk.IntVar(value=int(settings.get('note',60)))
        self.velocity = tk.IntVar(value=int(settings.get('velocity',108)))
        self.version = tk.StringVar(value='working')
        self.library = settings.get('core_library','')
        self.playing = False
        for label,var,minimum in [(self.i18n.get('note'),self.note,0),(self.i18n.get('velocity'),self.velocity,1)]:
            ttk.Label(row,text=label).pack(side='left',padx=4)
            ttk.Spinbox(row,from_=minimum,to=127,textvariable=var,width=5).pack(side='left')
        for label,value in [(self.i18n.get('working'),'working'),(self.i18n.get('saved'),'saved')]:
            ttk.Radiobutton(row,text=label,variable=self.version,value=value,command=self.restart).pack(side='left',padx=4)
        actions = ttk.Frame(self)
        actions.pack(fill='x')
        for label,callback in [(self.i18n.get('play'),self.play),('CC123 '+self.i18n.get('release'),self.release),
                               ('CC120 '+self.i18n.get('stop'),self.stop),
                               (self.i18n.get('export_3s_wav'),self.export),
                               (self.i18n.get('core_library'),self.choose_library)]:
            ttk.Button(actions,text=label,command=callback).pack(side='left',padx=3,pady=4)
        self.controllers = CCPanel(self,settings,self.change_cc,self.i18n)
        self.controllers.pack(fill='both',expand=True)

    def request(self):
        entry = self.editor.project.entries[self.editor.current]
        # Snapshot fields: a worker must never read the GUI's mutable working record.
        from project import Entry
        snapshot = Entry(entry.fields.copy(),entry.meta.copy(),entry.saved)
        bounds = entry.bounds if self.version.get() == 'working' else entry.saved
        note,velocity = self.note.get(),self.velocity.get()
        if not 0 <= note <= 127 or not 1 <= velocity <= 127:
            raise ValueError(get('midi_note_0_127_velocity_1_127'))
        return (self.editor.project,snapshot,bounds,self.library,self.controllers.values(),note,velocity)

    def play(self):
        try:
            request = self.request()
        except Exception as error:
            self.editor.error(error)
            return
        self.playing = True
        self.editor.preview.send('play',request)

    def restart(self):
        if self.playing:
            self.play()

    def release(self):
        self.playing = False
        self.editor.preview.send('release')

    def stop(self):
        self.playing = False
        self.editor.preview.send('cc',120,0)
        self.editor.preview.send('stop')

    def change_cc(self,cc,value):
        self.editor.preview.send('cc',cc,value)

    def export(self):
        try:
            request = self.request()
        except Exception as error:
            self.editor.error(error)
            return
        path = filedialog.asksaveasfilename(parent=self.editor,defaultextension='.wav',filetypes=[('WAV','*.wav')])
        if path:
            self.playing = False
            self.editor.preview.send('export',request,path)

    def choose_library(self):
        path = filedialog.askopenfilename(parent=self.editor,filetypes=[(self.i18n.get('shared_library'),'*.dll *.so *.dylib'),(self.i18n.get('all_files'),'*')])
        if path:
            self.library = path
            self.restart()

    def remember(self):
        settings = self.editor.app.settings
        settings.update(cc={str(k):v for k,v in self.controllers.values().items()},
                        core_library=self.library,note=self.note.get(),velocity=self.velocity.get())
        settings.save()
