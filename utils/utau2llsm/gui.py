"""Application lifecycle: manual conversion first, then the loop editor."""
from i18n import get
import queue
import threading
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox
from i18n import I18N
from settings import Settings
from converter import run_conversion
from converter_form import build_form, DEFAULTS


class ConverterApp(tk.Tk):
    def __init__(self, i18n, exe_paths):
        super().__init__()
        self.settings = Settings()
        self.i18n = I18N(self.settings.get('language', i18n.lang))
        self.exe_paths = exe_paths
        self.messages, self.busy, self.editor = queue.Queue(), False, None
        self.geometry('620x740')
        self.protocol('WM_DELETE_WINDOW', self.on_closing)
        self.rebuild()
        self.after(80, self.poll)

    def rebuild(self):
        self.title(self.i18n.get('window_title'))
        menu = tk.Menu(self)
        menu.add_command(label=self.i18n.get('open_existing_project'), command=self.open_project)
        lang = tk.Menu(menu,tearoff=False)
        for label, value in [('中文','zh_CN'), ('English','en'), ('Español','es')]:
            lang.add_command(label=label,command=lambda v=value:self.set_language(v))
        menu.add_cascade(label=self.i18n.get('menu_language'),menu=lang)
        self.configure(menu=menu)
        if hasattr(self,'form'):
            self.form.destroy()
        self.form = build_form(self)

    def save_config(self):
        for control in self.numeric_controls:
            control.commit()
        self.settings.update({k:v.get() for k,v in self.vars.items()})
        self.settings['language'] = self.i18n.lang
        self.settings.save()

    def set_language(self, language):
        if self.busy:
            return
        self.save_config()
        self.i18n = I18N(language)
        self.save_config()
        self.rebuild()

    def on_convert(self):
        if self.busy:
            return
        try:
            self.save_config()
            params = {k:type(default)(self.vars[k].get()) for k,default in DEFAULTS.items()}
            source, csv, output = [self.vars[k].get().strip() for k in ('utau_dir','csv_path','out_dir')]
            if not output or not (Path(source)/'oto.ini').is_file() or not Path(csv).is_file():
                raise ValueError(self.i18n.get('check_input_paths'))
            if not Path(self.exe_paths['wav2llsm']).is_file():
                raise ValueError(self.i18n.get('err_exe_missing'))
            if params['fmin'] <= 0 or params['fmax'] <= params['fmin']:
                raise ValueError(get('0_fmin_fmax'))
            self.busy = True
            self.convert_btn.configure(state='disabled')
            def work():
                try:
                    result = run_conversion(self.exe_paths,source,csv,output,params,
                        progress_cb=lambda n,total:self.messages.put(('progress',100*n/total)))
                    self.messages.put(('done',(output,*result)))
                except Exception as error:
                    self.messages.put(('error',str(error)))
            threading.Thread(target=work,daemon=True,name='utau-convert').start()
        except Exception as error:
            messagebox.showerror(self.i18n.get('err_title'),str(error),parent=self)

    def poll(self):
        try:
            while True:
                kind, data = self.messages.get_nowait()
                if kind == 'progress':
                    self.progress['value'] = data
                else:
                    self.busy = False
                    self.convert_btn.configure(state='normal')
                    if kind == 'error':
                        messagebox.showerror(self.i18n.get('err_title'),data,parent=self)
                    else:
                        output, count, failures = data
                        if failures:
                            messagebox.showwarning(self.i18n.get('warn_title'),'\n'.join(failures[:30]),parent=self)
                        if count:
                            self.launch_editor(Path(output)/'project.json')
        except queue.Empty:
            pass
        self.after(80,self.poll)

    def open_project(self):
        if not self.busy:
            path = filedialog.askopenfilename(parent=self,filetypes=[(self.i18n.get('project'),'project.json')])
            if path:
                self.launch_editor(path)

    def launch_editor(self, path):
        from project import Project
        from editor_window import EditorWindow
        try:
            project = Project(path)
            self.editor = EditorWindow(self,project)
            self.withdraw()
        except Exception as error:
            messagebox.showerror(self.i18n.get('err_title'),str(error),parent=self)

    def on_closing(self):
        if self.busy:
            messagebox.showinfo(self.i18n.get('converting'),self.i18n.get('please_wait_for_conversion'),parent=self)
            return
        try:
            self.save_config()
        except OSError as error:
            messagebox.showerror(self.i18n.get('settings'),str(error),parent=self)
            return
        self.destroy()
