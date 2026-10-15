"""Loop editor layout and GUI-thread event dispatch."""
from i18n import get
import queue
import threading
import tkinter as tk
from tkinter import ttk
import numpy as np
from audio_utils import read_wav
from analysis_job import load_cached
from editor_actions import EditorActions
from editor_preview import PreviewPanel
from preview_worker import PreviewWorker
from timeline import Timeline


class EditorWindow(EditorActions,tk.Toplevel):
    def __init__(self,app,project):
        super().__init__(app)
        self.app,self.project,self.i18n = app,project,app.i18n
        self.current,self.locked,self.closing = 0,set(),False
        self.analysis_running,self.next_project = False,None
        self.cancel_analysis = threading.Event()
        self.messages = queue.Queue()
        self.preview = PreviewWorker(self.messages)
        self.features,self.inputs = {},{}
        self.geometry('1280x940')
        self.minsize(960,760)
        self.protocol('WM_DELETE_WINDOW',self.close)
        self.status = tk.StringVar(value=self.i18n.get('wheel_to_zoom_drag_l_r_lines_to_edit_loop_frames'))
        self.show_features = tk.BooleanVar(value=True)
        self.build_menu()
        self.build_layout()
        self.bind('<Control-s>',lambda e:self.save())
        self.bind('<Control-z>',lambda e:self.undo())
        self.bind('<Control-y>',lambda e:self.redo())
        self.refresh()
        self.table.selection_set('0')
        self.table.focus('0')
        adjusted = sum('initial_adjustment' in e.meta for e in project.entries)
        if adjusted:
            self.status.set(self.i18n.get('initial_loops_adjusted').format(count=adjusted))
        self.after(70,self.poll)

    def build_menu(self):
        menu = tk.Menu(self)
        file = tk.Menu(menu,tearoff=False)
        for label,command in [(self.i18n.get('save_oto_db'),self.save),
                              (self.i18n.get('open_project'),self.open_other),
                              (self.i18n.get('return_to_converter'),self.close)]:
            file.add_command(label=label,command=command)
        menu.add_cascade(label=self.i18n.get('file'),menu=file)
        edit = tk.Menu(menu,tearoff=False)
        edit.add_command(label=self.i18n.get('undo_ctrl_z'),command=self.undo)
        edit.add_command(label=self.i18n.get('redo_ctrl_y'),command=self.redo)
        menu.add_cascade(label=self.i18n.get('edit'),menu=edit)
        analysis = tk.Menu(menu,tearoff=False)
        analysis.add_command(label=self.i18n.get('contentvec_analysis_2'),command=self.start_analysis)
        analysis.add_command(label=self.i18n.get('cancel_analysis'),command=self.cancel_analysis.set)
        menu.add_cascade(label=self.i18n.get('analysis'),menu=analysis)
        view = tk.Menu(menu,tearoff=False)
        view.add_checkbutton(label=self.i18n.get('contentvec_grayscale'),
                             variable=self.show_features,command=self.refresh_current)
        view.add_command(label=self.i18n.get('fit_whole_clip'),command=lambda:self.refresh_current(reset=True))
        menu.add_cascade(label=self.i18n.get('view'),menu=view)
        self.configure(menu=menu)

    def build_layout(self):
        pane = ttk.Panedwindow(self,orient='horizontal')
        pane.pack(fill='both',expand=True,padx=8,pady=8)
        sidebar = ttk.Frame(pane)
        pane.add(sidebar,weight=1)
        ttk.Label(sidebar,text=self.i18n.get('phonemes_unsaved'),padding=6).pack(anchor='w')
        holder = ttk.Frame(sidebar)
        holder.pack(fill='both',expand=True)
        self.table = ttk.Treeview(holder,columns=('alias','L','R','con','n'),show='headings',selectmode='extended',height=20)
        for col,width in [('alias',105),('L',60),('R',60),('con',80),('n',60)]:
            self.table.heading(col,text={'alias':self.i18n.get('phoneme'),'con':self.i18n.get('consonant_end'),'n':self.i18n.get('frames'),'L':self.i18n.get('left'),'R':self.i18n.get('right')}.get(col,col))
            self.table.column(col,width=width,stretch=col=='alias')
        scroll = ttk.Scrollbar(holder,command=self.table.yview)
        self.table.configure(yscrollcommand=scroll.set)
        self.table.pack(side='left',fill='both',expand=True)
        scroll.pack(side='right',fill='y')
        for i in range(len(self.project.entries)):
            self.table.insert('', 'end', iid=str(i))
        self.table.bind('<<TreeviewSelect>>',self.select)
        self.detail = tk.StringVar()
        ttk.Label(sidebar,textvariable=self.detail,wraplength=290,padding=8).pack(fill='x')
        main = ttk.Frame(pane)
        pane.add(main,weight=4)
        self.timeline = Timeline(main,self.commit,(self.i18n.get('waveform'),))
        self.timeline.pack(fill='both',expand=True)
        numeric = ttk.Frame(main,padding=5)
        numeric.pack(fill='x')
        self.left,self.right = tk.StringVar(),tk.StringVar()
        self.bound_widgets = []
        for label,var in [('L',self.left),('R',self.right)]:
            ttk.Label(numeric,text=label).pack(side='left',padx=5)
            widget = ttk.Entry(numeric,textvariable=var,width=9)
            widget.pack(side='left')
            widget.bind('<Return>',lambda e:self.numeric_bounds())
            self.bound_widgets.append(widget)
        button = ttk.Button(numeric,text=self.i18n.get('apply_frames'),command=self.numeric_bounds)
        button.pack(side='left',padx=8)
        self.bound_widgets.append(button)
        ttk.Label(numeric,text=self.i18n.get('integer_frames_con_l_r_nfrm')).pack(side='left')
        self.preview_panel = PreviewPanel(self,main)
        self.preview_panel.pack(fill='x')
        ttk.Label(self,textvariable=self.status,anchor='w',padding=7,wraplength=1200).pack(fill='x')

    def select(self,event=None):
        focus = self.table.focus()
        if not focus:
            return
        index = int(focus)
        if index != self.current:
            self.current = index
            self.refresh_current()
            self.preview_panel.restart()

    def refresh(self):
        self.title(('* ' if self.project.dirty else '') + self.i18n.get('oto_loop_editor') + str(self.project.root))
        for i,entry in enumerate(self.project.entries):
            left,right = entry.bounds
            self.table.item(str(i),values=(('* ' if entry.dirty else '')+entry.alias,left,right,entry.fields[3],entry.fields[6]))
        self.refresh_current()

    def refresh_current(self,reset=False):
        entry = self.project.entries[self.current]
        self.left.set(str(entry.bounds[0]))
        self.right.set(str(entry.bounds[1]))
        locked = self.current in self.locked
        self.timeline.locked = locked
        for widget in self.bound_widgets:
            widget.configure(state='disabled' if locked else 'normal')
        self.detail.set(f'{entry.alias}\nCC32={entry.symbol & 255} · PC={entry.symbol >> 8}\n'
                        + self.i18n.get('saved_l_r')+str(entry.saved))
        try:
            if self.current not in self.inputs:
                audio,fs = read_wav(self.project.asset(entry.meta['wav']))
                f0 = np.load(self.project.asset(entry.meta['f0']),allow_pickle=False)
                rd = np.load(self.project.asset(entry.meta['rd']),allow_pickle=False)
                if (fs != entry.meta['fs'] or f0.shape != (entry.fields[6],)
                        or rd.shape != f0.shape or not np.isfinite(rd).all()):
                    raise ValueError(get('project_wav_f0_rd_metadata_mismatch'))
                self.inputs[self.current] = audio,f0,rd
            if self.current not in self.features:
                self.features[self.current] = load_cached(self.project,entry)
            audio,f0,rd = self.inputs[self.current]
            self.timeline.show_features = self.show_features.get()
            self.timeline.set_data(entry,audio,f0,rd,self.features[self.current],reset)
        except Exception as error:
            self.timeline.entry = None
            self.timeline.delete('all')
            self.status.set(str(error))

    def poll(self):
        try:
            while True:
                kind,data = self.messages.get_nowait()
                if kind == 'status':
                    self.status.set(data)
                elif kind == 'features':
                    index,features = data
                    self.features[index] = features
                    if index == self.current:
                        self.refresh_current()
                elif kind == 'analysis_done':
                    self.finish_analysis(data)
                elif kind == 'preview_error':
                    self.preview_panel.playing = False
                    self.error(data)
                elif kind == 'preview_closed':
                    self.finish_close()
                    return
        except queue.Empty:
            pass
        self.after(70,self.poll)
