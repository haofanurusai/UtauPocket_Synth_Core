"""Shared frame axis for waveform, F0, Rd and optional 768-dimensional grayscale."""
import tkinter as tk
import numpy as np
from PIL import Image, ImageTk


class Timeline(tk.Canvas):
    def __init__(self, parent, commit, labels, hit_tolerance_px=8):
        super().__init__(parent,background='#111c2c',highlightthickness=0,height=420)
        self.commit, self.labels = commit, labels
        self.entry, self.audio, self.f0, self.features = None, None, None, None
        self.rd = None
        self.hit_tolerance_px = hit_tolerance_px
        self.bounds, self.drag, self.locked = None, None, False
        self.lo, self.hi, self.show_features = 0, 1, True
        self.bind('<Motion>',self.hover)
        self.bind('<Leave>',lambda event:self.configure(cursor=''))
        self.bind('<Configure>',lambda event:self.draw())
        self.bind('<ButtonPress-1>',self.press)
        self.bind('<B1-Motion>',self.motion)
        self.bind('<ButtonRelease-1>',self.release)
        self.bind('<MouseWheel>',self.zoom)
        self.bind('<Button-4>',lambda e:self.zoom(e,1))
        self.bind('<Button-5>',lambda e:self.zoom(e,-1))

    def set_data(self, entry, audio, f0, rd, features=None, reset=False):
        different = self.entry is not entry
        self.entry, self.audio, self.f0, self.features = entry, audio, f0, features
        self.rd = rd
        self.bounds = entry.bounds
        if different or reset:
            self.lo, self.hi = 0, entry.fields[6]-1
        self.draw()

    def x(self, frame):
        return 55 + (frame-self.lo)/(self.hi-self.lo)*max(1,self.winfo_width()-75)

    def frame(self,x):
        value = self.lo + (x-55)/max(1,self.winfo_width()-75)*(self.hi-self.lo)
        return max(0,min(self.entry.fields[6]-1,int(round(value))))

    def zoom(self,event,direction=None):
        if self.entry is None or self.drag is not None:
            return
        direction = direction if direction is not None else event.delta
        n = self.entry.fields[6]-1
        center = self.frame(event.x)
        span = min(n,max(4,(self.hi-self.lo)*(.8 if direction>0 else 1.25)))
        self.lo = max(0,min(n-span,center-span/2))
        self.hi = self.lo+span
        self.draw()

    def hit_boundary(self,event):
        if self.entry is None or self.locked or not 22 <= event.y <= self.winfo_height()-30:
            return None
        distance = [abs(event.x-self.x(b)) for b in self.bounds]
        closest = int(np.argmin(distance))
        return closest if distance[closest] <= self.hit_tolerance_px else None

    def hover(self,event):
        self.configure(cursor='sb_h_double_arrow' if self.hit_boundary(event) is not None else '')

    def press(self,event):
        self.drag = self.hit_boundary(event)
        self.hover(event)

    def motion(self,event):
        if self.drag is None or self.locked:
            return
        left,right = self.bounds
        value = self.frame(event.x)
        if self.drag == 0:
            left = max(self.entry.fields[3],min(right-1,value))
        else:
            right = max(left+1,min(self.entry.fields[6]-1,value))
        self.bounds = left,right
        self.draw()

    def release(self,event):
        if self.drag is not None:
            self.motion(event)
            self.drag = None
            if not self.locked:
                self.commit(self.bounds)
        self.hover(event)

    def draw(self):
        self.delete('all')
        if self.locked:
            self.configure(cursor='')
        if self.entry is None:
            return
        width,height = max(100,self.winfo_width()),max(250,self.winfo_height())
        feature_on = self.show_features and self.features is not None
        wave_top,wave_bottom = 30, height*.25
        f0_top,f0_bottom = height*.32,height*.47
        rd_top,rd_bottom = height*.55,height*(.68 if feature_on else .86)
        self.create_text(8,12,anchor='w',text=self.labels[0],fill='#d8e6f8')
        self.create_text(8,f0_top-12,anchor='w',text='F0 / Hz',fill='#d8e6f8')
        left,right = self.bounds
        self.create_rectangle(self.x(left),22,self.x(right),height-30,fill='#193a49',outline='')
        hop = self.entry.meta['hop']
        samples = self.audio
        mid,scale = (wave_top+wave_bottom)/2,(wave_bottom-wave_top)*.46
        # Min/max per pixel retains impulses when zoomed out.
        for x in range(55,width-20):
            span = (self.hi-self.lo)*hop/max(1,width-75)
            a = max(0,int(self.lo*hop+(x-55)*span))
            b = min(len(samples),max(a+1,int(self.lo*hop+(x-54)*span)))
            segment = samples[a:b]
            if len(segment):
                self.create_line(x,mid-segment.min()*scale,x,mid-segment.max()*scale,fill='#79c0cd')
        voiced = self.f0 > 0
        self.curve(self.f0,voiced,f0_top,f0_bottom,max(100,float(np.max(self.f0))),
                   '#9bdfa1','f0')
        self.create_text(8,rd_top-12,anchor='w',text='Rd',fill='#d8e6f8')
        maximum_rd = max(3.,float(np.max(self.rd[voiced])) if np.any(voiced) else 3.)
        self.curve(self.rd,voiced,rd_top,rd_bottom,maximum_rd,'#d4b5ff','rd')
        if feature_on:
            data = self.features[max(0,int(self.lo)):min(len(self.features),int(self.hi)+1)].T
            low,high = np.percentile(self.features,[1,99])
            gray = np.clip((data-low)/max(float(high-low),1e-9)*255,0,255).astype(np.uint8)
            image = Image.fromarray(gray).resize((max(1,width-75),max(1,int(height*.20))))
            self.photo = ImageTk.PhotoImage(image)
            self.create_image(55,height*.73,anchor='nw',image=self.photo)
            self.create_text(8,height*.70,anchor='w',text='ContentVec · 768',fill='#d8e6f8')
        for frame,color,label in [(left,'#5abbff','L'),(right,'#ffa94e','R')]:
            x = self.x(frame)
            self.create_line(x,22,x,height-30,fill=color,width=2)
            self.create_text(x+5,12,anchor='w',text=f'{label} {frame}',fill=color)
        for index,frame in enumerate(np.linspace(self.lo,self.hi,6)):
            text = f'{round(frame)} / {frame*hop/self.entry.meta["fs"]:.3f}s'
            self.create_text(self.x(frame),height-12,text=text,fill='#c0ccdc',
                             anchor='w' if index == 0 else 'e' if index == 5 else 'center')

    def curve(self,values,valid,top,bottom,maximum,color,tag):
        # Separate paths across unvoiced frames; never connect through missing estimates.
        points = []
        def flush():
            if len(points) >= 4:
                self.create_line(*points,fill=color,width=2,tags=tag)
            elif len(points) == 2:
                x,y = points
                self.create_oval(x-1,y-1,x+1,y+1,fill=color,outline='',tags=tag)
        for frame in range(max(0,int(self.lo)),min(len(values),int(self.hi)+1)):
            if valid[frame]:
                points.extend((self.x(frame),bottom-values[frame]/maximum*(bottom-top)))
            else:
                flush()
                points=[]
        flush()
        self.create_text(8,top+5,anchor='w',text=f'{maximum:g}',fill=color)
        self.create_text(8,bottom,anchor='w',text='0',fill=color)
