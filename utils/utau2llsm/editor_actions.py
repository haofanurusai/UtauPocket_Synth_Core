"""Editor transactions, optional analysis lifecycle, and dirty close handling."""
import threading
from tkinter import messagebox, filedialog
from analysis_dialog import AnalysisDialog
from analysis_job import run_analysis


class EditorActions:
    def error(self,error):
        messagebox.showerror(self.i18n.get('err_title'),str(error),parent=self)

    def commit(self,bounds):
        if self.current in self.locked:
            self.refresh_current()
            return
        try:
            if self.project.change({self.current:bounds}):
                self.preview_panel.restart()
        except Exception as error:
            self.error(error)
        self.refresh()

    def numeric_bounds(self):
        try:
            self.commit((int(self.left.get()),int(self.right.get())))
        except ValueError as error:
            self.error(error)

    def undo(self):
        if self.analysis_running:
            return
        self.project.undo()
        self.refresh()
        self.preview_panel.restart()

    def redo(self):
        if self.analysis_running:
            return
        self.project.redo()
        self.refresh()
        self.preview_panel.restart()

    def save(self):
        try:
            self.project.save()
            self.preview_panel.remember()
            self.refresh()
            self.status.set(self.i18n.get('saved_previous_database_oto_db_bak'))
            return True
        except Exception as error:
            self.error(error)
            return False

    def start_analysis(self):
        if self.analysis_running:
            return
        options = AnalysisDialog(self).result
        if options is None:
            return
        path,gpu,auto,scope,parameters = options
        if scope == 'all':
            indices = list(range(len(self.project.entries)))
        elif scope == 'selected':
            indices = [int(item) for item in self.table.selection()]
        else:
            indices = [self.current]
        if not indices:
            return
        self.app.settings['contentvec_path'] = path
        self.app.settings['loop_analysis'] = parameters
        try:
            self.app.settings.save()
        except OSError as error:
            self.error(error)
            return
        self.analysis_running = True
        self.locked = set(indices)
        self.cancel_analysis.clear()
        self.refresh_current()
        self.status.set(self.i18n.get('loading_contentvec'))
        threading.Thread(target=run_analysis,args=(self.project,indices,path,gpu,auto,
                         self.cancel_analysis,self.messages,parameters),daemon=True,name='utau-analysis').start()

    def finish_analysis(self,data):
        edits,errors,cancelled = data
        self.analysis_running = False
        self.locked.clear()
        try:
            self.project.change(edits)
        except Exception as error:
            errors.append(str(error))
        self.refresh()
        self.preview_panel.restart()
        self.status.set(self.i18n.get('cancelled_completed_results_retained') if cancelled else self.i18n.get('analysis_complete'))
        if errors:
            messagebox.showwarning('ContentVec','\n'.join(errors[:30]),parent=self)

    def open_other(self):
        path = filedialog.askopenfilename(parent=self,filetypes=[(self.i18n.get('project'),'project.json')])
        if path:
            self.close(next_project=path)

    def close(self,next_project=None):
        if self.closing:
            return
        if self.analysis_running:
            self.cancel_analysis.set()
            self.status.set(self.i18n.get('cancellation_requested_close_after_current_inference_finishes'))
            return
        if self.project.dirty:
            answer = messagebox.askyesnocancel(self.i18n.get('unsaved_changes'),
                       self.i18n.get('save_changes_to_oto_db'),parent=self)
            if answer is None or (answer and not self.save()):
                return
        try:
            self.preview_panel.remember()
        except Exception as error:
            self.error(error)
            return
        self.closing = True
        self.next_project = next_project
        self.preview.send('quit')
        self.status.set(self.i18n.get('closing_preview'))

    def finish_close(self):
        self.destroy()
        self.app.editor = None
        self.app.deiconify()
        if self.next_project:
            self.app.launch_editor(self.next_project)
