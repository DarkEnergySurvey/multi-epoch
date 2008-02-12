from __future__ import division

import os, sys
def fn_name(): return sys._getframe(1).f_code.co_name

import gobject
import gtk; gdk = gtk.gdk
import pango
pygtk_version_required = (2,2,0)
if gtk.pygtk_version < pygtk_version_required:
    raise SystemExit ("PyGTK %d.%d.%d is installed\n"
                      "PyGTK %d.%d.%d or later is required"
                      % (gtk.pygtk_version + pygtk_version_required))
del pygtk_version_required

import matplotlib
from matplotlib import verbose
from matplotlib._pylab_helpers import Gcf
from matplotlib.backend_bases import RendererBase, GraphicsContextBase, \
     FigureManagerBase, FigureCanvasBase, NavigationToolbar2, cursors
from matplotlib.backends.backend_gdk import RendererGDK, FigureCanvasGDK
from matplotlib.cbook import is_string_like, enumerate
from matplotlib.colors import colorConverter
from matplotlib.figure import Figure
import matplotlib.numerix as numerix
from matplotlib.numerix import asarray, fromstring, UInt8, zeros, \
     where, transpose, nonzero, indices, ones, nx
from matplotlib.widgets import SubplotTool


backend_version = "%d.%d.%d" % gtk.pygtk_version

_debug = False
#_debug = True

# the true dots per inch on the screen; should be display dependent
# see http://groups.google.com/groups?q=screen+dpi+x11&hl=en&lr=&ie=UTF-8&oe=UTF-8&safe=off&selm=7077.26e81ad5%40swift.cs.tcd.ie&rnum=5 for some info about screen dpi
PIXELS_PER_INCH = 96

# Image formats that this backend supports - for FileChooser and print_figure()
IMAGE_FORMAT = ['bmp', 'eps', 'jpg', 'png', 'ps', 'svg']
# pdf not ready yet
#IMAGE_FORMAT  = ['bmp', 'eps', 'jpg', 'png', 'pdf', 'ps', 'svg']
IMAGE_FORMAT.sort()
IMAGE_FORMAT_DEFAULT  = 'png'


cursord = {
    cursors.MOVE          : gdk.Cursor(gdk.FLEUR),
    cursors.HAND          : gdk.Cursor(gdk.HAND2),
    cursors.POINTER       : gdk.Cursor(gdk.LEFT_PTR),
    cursors.SELECT_REGION : gdk.Cursor(gdk.TCROSS),
    }

# ref gtk+/gtk/gtkwidget.h
def GTK_WIDGET_DRAWABLE(w):
    flags = w.flags();
    return flags & gtk.VISIBLE != 0 and flags & gtk.MAPPED != 0


def draw_if_interactive():
    """
    Is called after every pylab drawing command
    """
    if matplotlib.is_interactive():
        figManager =  Gcf.get_active()
        if figManager is not None:
            figManager.canvas.draw()

def show(mainloop=True):
    """
    Show all the figures and enter the gtk main loop
    This should be the last line of your script
    """
    for manager in Gcf.get_all_fig_managers():
        manager.window.show()

    if mainloop and gtk.main_level() == 0:
        gtk.main()

def new_figure_manager(num, *args, **kwargs):
    """
    Create a new figure manager instance
    """
    FigureClass = kwargs.pop('FigureClass', Figure)    
    thisFig = FigureClass(*args, **kwargs)
    canvas = FigureCanvasGTK(thisFig)
    manager = FigureManagerGTK(canvas, num)
    # equals:
    #manager = FigureManagerGTK(FigureCanvasGTK(Figure(*args, **kwargs), num)
    return manager


class FigureCanvasGTK (gtk.DrawingArea, FigureCanvasBase):
    keyvald = {65507 : 'control',
               65505 : 'shift',
               65513 : 'alt',
               65508 : 'control',
               65506 : 'shift',
               65514 : 'alt',
               65361 : 'left',
               65362 : 'up',
               65363 : 'right',
               65364 : 'down',
               }

    def __init__(self, figure):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        FigureCanvasBase.__init__(self, figure)
        gtk.DrawingArea.__init__(self)

        self._idleID        = 0
        self._need_redraw   = True
        self._pixmap_width  = -1
        self._pixmap_height = -1
        self._lastCursor    = None

        self.connect('button_press_event',   self.button_press_event)
        self.connect('button_release_event', self.button_release_event)
        self.connect('configure_event',      self.configure_event)
        self.connect('expose_event',         self.expose_event)
        self.connect('key_press_event',      self.key_press_event)
        self.connect('key_release_event',    self.key_release_event)
        self.connect('motion_notify_event',  self.motion_notify_event)

        self.set_events(
            gdk.BUTTON_PRESS_MASK   |
            gdk.BUTTON_RELEASE_MASK |
            gdk.EXPOSURE_MASK       |
            gdk.KEY_PRESS_MASK      |
            gdk.KEY_RELEASE_MASK    |
            gdk.LEAVE_NOTIFY_MASK   |
            gdk.POINTER_MOTION_MASK |
            gdk.POINTER_MOTION_HINT_MASK)

        self.set_double_buffered(False)
        self.set_flags(gtk.CAN_FOCUS)
        self._renderer_init()


    def button_press_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        x = event.x
        # flipy so y=0 is bottom of canvas
        y = self.allocation.height - event.y
        FigureCanvasBase.button_press_event(self, x, y, event.button)
        return False  # finish event propagation?

    def button_release_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        x = event.x
        # flipy so y=0 is bottom of canvas
        y = self.allocation.height - event.y
        FigureCanvasBase.button_release_event(self, x, y, event.button)
        return False  # finish event propagation?

    def key_press_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        key = self._get_key(event)
        if _debug: print "hit", key
        FigureCanvasBase.key_press_event(self, key)
        return False  # finish event propagation?

    def key_release_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        key = self._get_key(event)
        if _debug: print "release", key
        FigureCanvasBase.key_release_event(self, key)
        return False  # finish event propagation?

    def motion_notify_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        if event.is_hint:
            x, y, state = event.window.get_pointer()
        else:
            x, y, state = event.x, event.y, event.state

        # flipy so y=0 is bottom of canvas
        y = self.allocation.height - y
        FigureCanvasBase.motion_notify_event(self, x, y)
        return False  # finish event propagation?

    def _get_key(self, event):
        if self.keyvald.has_key(event.keyval):
            key = self.keyvald[event.keyval]
        elif event.keyval <256:
            key = chr(event.keyval)
        else:
            key = None

        ctrl  = event.state & gdk.CONTROL_MASK
        shift = event.state & gdk.SHIFT_MASK
        return key


    def configure_event(self, widget, event):
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()
        if widget.window is None:
            return
        w, h = event.width, event.height
        if w < 3 or h < 3:
            return # empty fig

        # resize the figure (in inches)
        dpi = self.figure.dpi.get()
        self.figure.set_size_inches (w/dpi, h/dpi)
        self._need_redraw = True

        return False  # finish event propagation?


    def draw(self):
        # Note: FigureCanvasBase.draw() is inconveniently named as it clashes
        # with the deprecated gtk.Widget.draw()

        self._need_redraw = True
        if GTK_WIDGET_DRAWABLE(self):
            self.queue_draw()
            # do a synchronous draw (its less efficient than an async draw,
            # but is required if/when animation is used)
            self.window.process_updates (False)

    def draw_idle(self):
        def idle_draw(*args):
            self.draw()
            self._idleID = 0
            return False
        if self._idleID == 0:
            self._idleID = gobject.idle_add(idle_draw)


    def _renderer_init(self):
        """Override by GTK backends to select a different renderer
        Renderer should provide the methods:
            set_pixmap ()
            set_width_height ()
        that are used by
            _render_figure() / _pixmap_prepare()
        """
        self._renderer = RendererGDK (self, self.figure.dpi)


    def _pixmap_prepare(self, width, height):
        """
        Make sure _._pixmap is at least width, height,
        create new pixmap if necessary
        """
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()

        create_pixmap = False
        if width > self._pixmap_width:
            # increase the pixmap in 10%+ (rather than 1 pixel) steps
            self._pixmap_width  = max (int (self._pixmap_width  * 1.1),
                                       width)
            create_pixmap = True

        if height > self._pixmap_height:
            self._pixmap_height = max (int (self._pixmap_height * 1.1),
                                           height)
            create_pixmap = True

        if create_pixmap:
            self._pixmap = gdk.Pixmap (self.window, self._pixmap_width,
                                       self._pixmap_height)
            self._renderer.set_pixmap (self._pixmap)


    def _render_figure(self, pixmap, width, height):
        """used by GTK and GTKcairo. GTKAgg overrides
        """
        self._renderer.set_width_height (width, height)
        self.figure.draw (self._renderer)


    def expose_event(self, widget, event):
        """Expose_event for all GTK backends. Should not be overridden.
        """
        if _debug: print 'FigureCanvasGTK.%s' % fn_name()

        if GTK_WIDGET_DRAWABLE(self):
            if self._need_redraw:
                x, y, w, h = self.allocation
                self._pixmap_prepare (w, h)
                self._render_figure(self._pixmap, w, h)
                self._need_redraw = False

            x, y, w, h = event.area
            self.window.draw_drawable (self.style.fg_gc[self.state],
                                       self._pixmap, x, y, x, y, w, h)
        return False  # finish event propagation?


    def print_figure(self, filename, dpi=None, facecolor='w', edgecolor='w',
                     orientation='portrait', **kwargs):
        # TODO - use gdk/cairo/agg print_figure?
        if dpi is None: dpi = matplotlib.rcParams['savefig.dpi']
        root, ext = os.path.splitext(filename)
        ext = ext[1:]
        if ext == '':
            ext      = IMAGE_FORMAT_DEFAULT
            filename = filename + '.' + ext

        # save figure settings
        origDPI       = self.figure.dpi.get()
        origfacecolor = self.figure.get_facecolor()
        origedgecolor = self.figure.get_edgecolor()
        origWIn, origHIn = self.figure.get_size_inches()

        if self.flags() & gtk.REALIZED == 0:
            # for self.window(for pixmap) and has a side effect of altering
            # figure width,height (via configure-event?)
            gtk.DrawingArea.realize(self)

        self.figure.dpi.set(dpi)
        self.figure.set_facecolor(facecolor)
        self.figure.set_edgecolor(edgecolor)

        ext = ext.lower()
        if ext in ('jpg', 'png'):          # native printing
            width, height = self.get_width_height()
            pixmap = gdk.Pixmap (self.window, width, height)
            self._renderer.set_pixmap (pixmap)
            self._render_figure(pixmap, width, height)

            # jpg colors don't match the display very well, png colors match
            # better
            pixbuf = gdk.Pixbuf(gdk.COLORSPACE_RGB, 0, 8, width, height)
            pixbuf.get_from_drawable(pixmap, pixmap.get_colormap(),
                                     0, 0, 0, 0, width, height)

            # pixbuf.save() recognises 'jpeg' not 'jpg'
            if ext == 'jpg': ext = 'jpeg'
            try:
                pixbuf.save(filename, ext)
            except gobject.GError, exc:
                error_msg_gtk('Save figure failure:\n%s' % (exc,), parent=self)

        elif ext in ('eps', 'ps', 'svg',):
            if ext == 'svg':
                from backend_svg import FigureCanvasSVG as FigureCanvas
            else:
                from backend_ps  import FigureCanvasPS  as FigureCanvas

            try:
                fc = self.switch_backends(FigureCanvas)
                fc.print_figure(filename, dpi, facecolor, edgecolor,
                                orientation, **kwargs)
            except IOError, exc:
                error_msg_gtk("Save figure failure:\n%s: %s" %
                          (exc.filename, exc.strerror), parent=self)
            except Exception, exc:
                error_msg_gtk("Save figure failure:\n%s" % exc, parent=self)

        elif ext in ('bmp', 'raw', 'rgb',):
            try:
                from backend_agg import FigureCanvasAgg  as FigureCanvas
            except:
                error_msg_gtk('Save figure failure:\n'
                          'Agg must be installed to save as bmp, raw and rgb',
                          parent=self)
            else:
                fc = self.switch_backends(FigureCanvas)
                fc.print_figure(filename, dpi, facecolor, edgecolor,
                                orientation, **kwargs)

        elif ext in ('pdf',):
            try:
                from backend_cairo import FigureCanvasCairo  as FigureCanvas
            except:
                error_msg_gtk('Save figure failure:\n'
                          'Cairo must be installed to save as pdf',
                          parent=self)
            else:
                fc = self.switch_backends(FigureCanvas)
                fc.print_figure(filename, dpi, facecolor, edgecolor,
                                orientation, **kwargs)

        else:
            error_msg_gtk('Format "%s" is not supported.\nSupported formats are %s.' %
                      (ext, ', '.join(IMAGE_FORMAT)),
                      parent=self)

        # restore figure settings
        self.figure.dpi.set(origDPI)
        self.figure.set_facecolor(origfacecolor)
        self.figure.set_edgecolor(origedgecolor)
        self.figure.set_size_inches(origWIn, origHIn)
        self.figure.set_canvas(self)


class FigureManagerGTK(FigureManagerBase):
    """
    Public attributes

    canvas      : The FigureCanvas instance
    num         : The Figure number
    toolbar     : The gtk.Toolbar  (gtk only)
    vbox        : The gtk.VBox containing the canvas and toolbar (gtk only)
    window      : The gtk.Window   (gtk only)
    """
    def __init__(self, canvas, num):
        if _debug: print 'FigureManagerGTK.%s' % fn_name()
        FigureManagerBase.__init__(self, canvas, num)

        self.window = gtk.Window()
        self.window.set_title("Figure %d" % num)

        self.vbox = gtk.VBox()
        self.window.add(self.vbox)
        self.vbox.show()

        self.canvas.show()
        self.vbox.pack_start(self.canvas, True, True)

        self.toolbar = self._get_toolbar(canvas)

        # calculate size for window
        w = int (self.canvas.figure.bbox.width())
        h = int (self.canvas.figure.bbox.height())

        if self.toolbar is not None:
            self.toolbar.show()
            self.vbox.pack_end(self.toolbar, False, False)

            tb_w, tb_h = self.toolbar.size_request()
            h += tb_h
        self.window.set_default_size (w, h)

        def destroy(*args):
            Gcf.destroy(num)
        self.window.connect("destroy", destroy)
        self.window.connect("delete_event", destroy)
        if matplotlib.is_interactive():
            self.window.show()

        def notify_axes_change(fig):
            'this will be called whenever the current axes is changed'
            if self.toolbar is not None: self.toolbar.update()
        self.canvas.figure.add_axobserver(notify_axes_change)

        self.canvas.grab_focus()

    def destroy(self, *args):
        if _debug: print 'FigureManagerGTK.%s' % fn_name()
        self.window.destroy()

        if Gcf.get_num_fig_managers()==0 and \
               not matplotlib.is_interactive() and \
               gtk.main_level() >= 1:
            gtk.main_quit()


    def full_screen_toggle (self):
        self._full_screen_flag = not self._full_screen_flag
        if self._full_screen_flag:
            self.window.fullscreen()
        else:
            self.window.unfullscreen()
    _full_screen_flag = False


    def _get_toolbar(self, canvas):
        # must be inited after the window, drawingArea and figure
        # attrs are set
        if matplotlib.rcParams['toolbar'] == 'classic':
            toolbar = NavigationToolbar (canvas, self.window)
        elif matplotlib.rcParams['toolbar'] == 'toolbar2':
            toolbar = NavigationToolbar2GTK (canvas, self.window)
        else:
            toolbar = None
        return toolbar


    def resize(self, width, height):
        'set the canvas size in pixels'
        #_, _, cw, ch = self.canvas.allocation
        #_, _, ww, wh = self.window.allocation
        #self.window.resize (width-cw+ww, height-ch+wh)
        self.window.resize(width, height)


class NavigationToolbar2GTK(NavigationToolbar2, gtk.Toolbar):
    # list of toolitems to add to the toolbar, format is:
    # text, tooltip_text, image_file, callback(str)
    toolitems = (
        ('Home', 'Reset original view', 'home.png', 'home'),
        ('Back', 'Back to  previous view','back.png', 'back'),
        ('Forward', 'Forward to next view','forward.png', 'forward'),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'move.png','pan'),
        ('Zoom', 'Zoom to rectangle','zoom_to_rect.png', 'zoom'),
        (None, None, None, None),
        ('Subplots', 'Configure subplots','subplots.png', 'configure_subplots'),
        ('Save', 'Save the figure','filesave.png', 'save_figure'),
        )

    def __init__(self, canvas, window):
        self.win = window
        gtk.Toolbar.__init__(self)
        NavigationToolbar2.__init__(self, canvas)
        self._idleId = 0

    def set_message(self, s):
        if self._idleId == 0:
            self.message.set_label(s)

    def set_cursor(self, cursor):
        self.canvas.window.set_cursor(cursord[cursor])

    def release(self, event):
        try: del self._imageBack
        except AttributeError: pass

    def dynamic_update(self):
        # legacy method; new method is canvas.draw_idle
        self.canvas.draw_idle()

    def draw_rubberband(self, event, x0, y0, x1, y1):
        'adapted from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/189744'
        drawable = self.canvas.window
        if drawable is None:
            return

        gc = drawable.new_gc()

        height = self.canvas.figure.bbox.height()
        y1 = height - y1
        y0 = height - y0

        w = abs(x1 - x0)
        h = abs(y1 - y0)

        rect = [int(val)for val in min(x0,x1), min(y0, y1), w, h]
        try: lastrect, imageBack = self._imageBack
        except AttributeError:
            #snap image back
            if event.inaxes is None:
                return

            ax = event.inaxes
            l,b,w,h = [int(val) for val in ax.bbox.get_bounds()]
            b = int(height)-(b+h)
            axrect = l,b,w,h
            self._imageBack = axrect, drawable.get_image(*axrect)
            drawable.draw_rectangle(gc, False, *rect)
            self._idleId = 0
        else:
            def idle_draw(*args):

                drawable.draw_image(gc, imageBack, 0, 0, *lastrect)
                drawable.draw_rectangle(gc, False, *rect)
                self._idleId = 0
                return False
            if self._idleId == 0:
                self._idleId = gobject.idle_add(idle_draw)


    def _init_toolbar(self):
        self.set_style(gtk.TOOLBAR_ICONS)

        if gtk.pygtk_version >= (2,4,0):
            self._init_toolbar2_4()
        else:
            self._init_toolbar2_2()


    def _init_toolbar2_2(self):
        basedir = matplotlib.rcParams['datapath']

        for text, tooltip_text, image_file, callback in self.toolitems:
            if text is None:
                 self.append_space()
                 continue

            fname = os.path.join(basedir, image_file)
            image = gtk.Image()
            image.set_from_file(fname)
            w = self.append_item(text,
                                 tooltip_text,
                                 'Private',
                                 image,
                                 getattr(self, callback)
                                 )

        self.append_space()

        self.message = gtk.Label()
        self.append_widget(self.message, None, None)
        self.message.show()

        self.fileselect = FileSelection(title='Save the figure',
                                        parent=self.win,)



    def _init_toolbar2_4(self):
        basedir = matplotlib.rcParams['datapath']
        self.tooltips = gtk.Tooltips()

        for text, tooltip_text, image_file, callback in self.toolitems:
            if text is None:
                self.insert( gtk.SeparatorToolItem(), -1 )
                continue
            fname = os.path.join(basedir, image_file)
            image = gtk.Image()
            image.set_from_file(fname)
            tbutton = gtk.ToolButton(image, text)
            self.insert(tbutton, -1)
            tbutton.connect('clicked', getattr(self, callback))
            tbutton.set_tooltip(self.tooltips, tooltip_text, 'Private')

        toolitem = gtk.SeparatorToolItem()
        self.insert(toolitem, -1)
        # set_draw() not making separator invisible,
        # bug #143692 fixed Jun 06 2004, will be in GTK+ 2.6
        toolitem.set_draw(False)
        toolitem.set_expand(True)

        toolitem = gtk.ToolItem()
        self.insert(toolitem, -1)
        self.message = gtk.Label()
        toolitem.add(self.message)

        self.show_all()

        self.fileselect = FileChooserDialog(title='Save the figure',
                                            parent=self.win,)


    def save_figure(self, button):
        fname = self.fileselect.get_filename_from_user()
        if fname:
            self.canvas.print_figure(fname)

    def configure_subplots(self, button):
        toolfig = Figure(figsize=(6,3))
        canvas = self._get_canvas(toolfig)
        toolfig.subplots_adjust(top=0.9)
        tool =  SubplotTool(self.canvas.figure, toolfig)

        w = int (toolfig.bbox.width())
        h = int (toolfig.bbox.height())


        window = gtk.Window()
        window.set_title("Subplot Configuration Tool")
        window.set_default_size(w, h)
        vbox = gtk.VBox()
        window.add(vbox)
        vbox.show()

        canvas.show()
        vbox.pack_start(canvas, True, True)
        window.show()

    def _get_canvas(self, fig):
        return FigureCanvasGTK(fig)


class NavigationToolbar(gtk.Toolbar):
    """
    Public attributes

      canvas - the FigureCanvas  (gtk.DrawingArea)
      win    - the gtk.Window

    """
    # list of toolitems to add to the toolbar, format is:
    # text, tooltip_text, image, callback(str), callback_arg, scroll(bool)
    toolitems = (
        ('Left', 'Pan left with click or wheel mouse (bidirectional)',
         gtk.STOCK_GO_BACK, 'panx', -1, True),
        ('Right', 'Pan right with click or wheel mouse (bidirectional)',
         gtk.STOCK_GO_FORWARD, 'panx', 1, True),
        ('Zoom In X',
         'Zoom In X (shrink the x axis limits) with click or wheel'
         ' mouse (bidirectional)',
         gtk.STOCK_ZOOM_IN, 'zoomx', 1, True),
        ('Zoom Out X',
         'Zoom Out X (expand the x axis limits) with click or wheel'
         ' mouse (bidirectional)',
         gtk.STOCK_ZOOM_OUT, 'zoomx', -1, True),
        (None, None, None, None, None, None,),
        ('Up', 'Pan up with click or wheel mouse (bidirectional)',
         gtk.STOCK_GO_UP, 'pany', 1, True),
        ('Down', 'Pan down with click or wheel mouse (bidirectional)',
         gtk.STOCK_GO_DOWN, 'pany', -1, True),
        ('Zoom In Y',
         'Zoom in Y (shrink the y axis limits) with click or wheel'
         ' mouse (bidirectional)',
         gtk.STOCK_ZOOM_IN, 'zoomy', 1, True),
        ('Zoom Out Y',
         'Zoom Out Y (expand the y axis limits) with click or wheel'
         ' mouse (bidirectional)',
         gtk.STOCK_ZOOM_OUT, 'zoomy', -1, True),
        (None, None, None, None, None, None,),
        ('Save', 'Save the figure',
         gtk.STOCK_SAVE, 'save_figure', None, False),
        )

    def __init__(self, canvas, window):
        """
        figManager is the FigureManagerGTK instance that contains the
        toolbar, with attributes figure, window and drawingArea

        """
        gtk.Toolbar.__init__(self)

        self.canvas = canvas
        # Note: gtk.Toolbar already has a 'window' attribute
        self.win    = window

        self.set_style(gtk.TOOLBAR_ICONS)

        if gtk.pygtk_version >= (2,4,0):
            self._create_toolitems_2_4()
            self.update = self._update_2_4
            self.fileselect = FileChooserDialog(title='Save the figure',
                                                parent=self.win,)
        else:
            self._create_toolitems_2_2()
            self.update = self._update_2_2
            self.fileselect = FileSelection(title='Save the figure',
                                            parent=self.win)
        self.show_all()
        self.update()


    def _create_toolitems_2_4(self):
        # use the GTK+ 2.4 GtkToolbar API
        iconSize = gtk.ICON_SIZE_SMALL_TOOLBAR
        self.tooltips = gtk.Tooltips()

        for text, tooltip_text, image, callback, callback_arg, scroll \
                in self.toolitems:
            if text is None:
                self.insert( gtk.SeparatorToolItem(), -1 )
                continue
            tbutton = gtk.ToolButton(gtk.image_new_from_stock(image, iconSize),
                                     text)
            self.insert(tbutton, -1)
            if callback_arg:
                tbutton.connect('clicked', getattr(self, callback),
                                callback_arg)
            else:
                tbutton.connect('clicked', getattr(self, callback))
            if scroll:
                tbutton.connect('scroll_event', getattr(self, callback))
            tbutton.set_tooltip(self.tooltips, tooltip_text, 'Private')

        # Axes toolitem, is empty at start, update() adds a menu if >=2 axes
        self.axes_toolitem = gtk.ToolItem()
        self.insert(self.axes_toolitem, 0)
        self.axes_toolitem.set_tooltip (
            self.tooltips,
            tip_text='Select axes that controls affect',
            tip_private = 'Private')

        align = gtk.Alignment (xalign=0.5, yalign=0.5, xscale=0.0, yscale=0.0)
        self.axes_toolitem.add(align)

        self.menubutton = gtk.Button ("Axes")
        align.add (self.menubutton)

        def position_menu (menu):
            """Function for positioning a popup menu.
            Place menu below the menu button, but ensure it does not go off
            the bottom of the screen.
            The default is to popup menu at current mouse position
            """
            x0, y0    = self.window.get_origin()
            x1, y1, m = self.window.get_pointer()
            x2, y2    = self.menubutton.get_pointer()
            sc_h      = self.get_screen().get_height()  # requires GTK+ 2.2 +
            w, h      = menu.size_request()

            x = x0 + x1 - x2
            y = y0 + y1 - y2 + self.menubutton.allocation.height
            y = min(y, sc_h - h)
            return x, y, True

        def button_clicked (button, data=None):
            self.axismenu.popup (None, None, position_menu, 0,
                                 gtk.get_current_event_time())

        self.menubutton.connect ("clicked", button_clicked)


    def _update_2_4(self):
        # for GTK+ 2.4+
        # called by __init__() and FigureManagerGTK

        self._axes = self.canvas.figure.axes

        if len(self._axes) >= 2:
            self.axismenu = self._make_axis_menu()
            self.menubutton.show_all()
        else:
            self.menubutton.hide()

        self.set_active(range(len(self._axes)))


    def _create_toolitems_2_2(self):
        # use the GTK+ 2.2 (and lower) GtkToolbar API
        iconSize = gtk.ICON_SIZE_SMALL_TOOLBAR

        for text, tooltip_text, image, callback, callback_arg, scroll \
                in self.toolitems:
            if text is None:
                self.append_space()
                continue
            item = self.append_item(text, tooltip_text, 'Private',
                                    gtk.image_new_from_stock(image, iconSize),
                                    getattr(self, callback), callback_arg)
            if scroll:
                item.connect("scroll_event", getattr(self, callback))

        self.omenu = gtk.OptionMenu()
        self.omenu.set_border_width(3)
        self.insert_widget(
            self.omenu,
            'Select axes that controls affect',
            'Private', 0)


    def _update_2_2(self):
        # for GTK+ 2.2 and lower
        # called by __init__() and FigureManagerGTK

        self._axes = self.canvas.figure.axes

        if len(self._axes) >= 2:
            # set up the axis menu
            self.omenu.set_menu( self._make_axis_menu() )
            self.omenu.show_all()
        else:
            self.omenu.hide()

        self.set_active(range(len(self._axes)))


    def _make_axis_menu(self):
        # called by self._update*()

        def toggled(item, data=None):
            if item == self.itemAll:
                for item in items: item.set_active(True)
            elif item == self.itemInvert:
                for item in items:
                    item.set_active(not item.get_active())

            ind = [i for i,item in enumerate(items) if item.get_active()]
            self.set_active(ind)

        menu = gtk.Menu()

        self.itemAll = gtk.MenuItem("All")
        menu.append(self.itemAll)
        self.itemAll.connect("activate", toggled)

        self.itemInvert = gtk.MenuItem("Invert")
        menu.append(self.itemInvert)
        self.itemInvert.connect("activate", toggled)

        items = []
        for i in range(len(self._axes)):
            item = gtk.CheckMenuItem("Axis %d" % (i+1))
            menu.append(item)
            item.connect("toggled", toggled)
            item.set_active(True)
            items.append(item)

        menu.show_all()
        return menu


    def set_active(self, ind):
        self._ind = ind
        self._active = [ self._axes[i] for i in self._ind ]

    def panx(self, button, arg):
        """arg is either user callback data or a scroll event
        """
        try:
            if arg.direction == gdk.SCROLL_UP: direction=1
            else: direction=-1
        except AttributeError:
            direction = arg

        for a in self._active:
            a.panx(direction)
        self.canvas.draw()
        return True

    def pany(self, button, arg):
        try:
            if arg.direction == gdk.SCROLL_UP: direction=1
            else: direction=-1
        except AttributeError:
            direction = arg

        for a in self._active:
            a.pany(direction)
        self.canvas.draw()
        return True

    def zoomx(self, button, arg):
        try:
            if arg.direction == gdk.SCROLL_UP: direction=1
            else: direction=-1
        except AttributeError:
            direction = arg

        for a in self._active:
            a.zoomx(direction)
        self.canvas.draw()
        return True

    def zoomy(self, button, arg):
        try:
            if arg.direction == gdk.SCROLL_UP: direction=1
            else: direction=-1
        except AttributeError:
            direction = arg

        for a in self._active:
            a.zoomy(direction)
        self.canvas.draw()
        return True


    def save_figure(self, button):
        fname = self.fileselect.get_filename_from_user()
        if fname:
            self.canvas.print_figure(fname)


class FileSelection(gtk.FileSelection):
    """GTK+ 2.2 and lower file selector which remembers the last
    file/directory selected
    """
    def __init__(self, path=None, title='Select a file', parent=None):
        super(FileSelection, self).__init__(title)

        if path: self.path = path
        else:    self.path = os.getcwd() + os.sep

        if parent: self.set_transient_for(parent)

    def get_filename_from_user(self, path=None, title=None):
        if path:  self.path = path
        if title: self.set_title(title)
        self.set_filename(self.path)

        filename = None
        if self.run() == gtk.RESPONSE_OK:
            self.path = filename = self.get_filename()
        self.hide()
        return filename


if gtk.pygtk_version >= (2,4,0):
    class FileChooserDialog(gtk.FileChooserDialog):
        """GTK+ 2.4 file selector which remembers the last file/directory
        selected and presents the user with a menu of supported image formats
        """
        def __init__ (self,
                      title   = 'Save file',
                      parent  = None,
                      action  = gtk.FILE_CHOOSER_ACTION_SAVE,
                      buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                 gtk.STOCK_SAVE,   gtk.RESPONSE_OK),
                      path    = None,
                      ):
            super (FileChooserDialog, self).__init__ (title, parent, action,
                                                      buttons)
            self.set_default_response (gtk.RESPONSE_OK)

            if path: self.path = path
            else:    self.path = os.getcwd() + os.sep

            # create an extra widget to list supported image formats
            self.set_current_folder (self.path)
            self.set_current_name ('image.' + IMAGE_FORMAT_DEFAULT)

            hbox = gtk.HBox (spacing=10)
            hbox.pack_start (gtk.Label ("Image Format:"), expand=False)

            self.cbox = gtk.combo_box_new_text()
            hbox.pack_start (self.cbox)

            for item in IMAGE_FORMAT:
                self.cbox.append_text (item)
            self.cbox.set_active (IMAGE_FORMAT.index (IMAGE_FORMAT_DEFAULT))

            def cb_cbox_changed (cbox, data=None):
                """File extension changed"""
                head, filename = os.path.split(self.get_filename())
                root, ext = os.path.splitext(filename)
                ext = ext[1:]
                new_ext = IMAGE_FORMAT[cbox.get_active()]

                if ext in IMAGE_FORMAT:
                    filename = filename.replace(ext, new_ext)
                elif ext == '':
                    filename = filename.rstrip('.') + '.' + new_ext

                self.set_current_name (filename)
            self.cbox.connect ("changed", cb_cbox_changed)

            hbox.show_all()
            self.set_extra_widget(hbox)


        def get_filename_from_user (self):
            while True:
                filename = None
                if self.run() != gtk.RESPONSE_OK:
                    break
                filename = self.get_filename()
                menu_ext  = IMAGE_FORMAT[self.cbox.get_active()]
                root, ext = os.path.splitext (filename)
                ext = ext[1:]
                if ext == '':
                    ext = menu_ext
                    filename += '.' + ext

                if ext in IMAGE_FORMAT:
                    self.path = filename
                    break
                else:
                    error_msg_gtk ('Image format "%s" is not supported' % ext,
                                   parent=self)
                    self.set_current_name (os.path.split(root)[1] + '.' +
                                           menu_ext)

            self.hide()
            return filename


class DialogLineprops:
    """
    A GUI dialog for controlling lineprops
    """
    signals = (
        'on_combobox_lineprops_changed',
        'on_combobox_linestyle_changed',
        'on_combobox_marker_changed',
        'on_colorbutton_linestyle_color_set',
        'on_colorbutton_markerface_color_set',
        'on_dialog_lineprops_okbutton_clicked',
        'on_dialog_lineprops_cancelbutton_clicked',
        )

    linestyles = (
        '-'    ,
        '--'   ,
        '-.'   ,
        ','    ,
        'steps',
        'None' ,
    )

    linestyled = dict([ (s,i) for i,s in enumerate(linestyles)])


    markers =  (
        'None',
        '.'  ,
        ','  ,
        'o'  ,
        'v'  ,
        '^'  ,
        '<'  ,
        '>'  ,
        '1'  ,
        '2'  ,
        '3'  ,
        '4'  ,
        's'  ,
        'p'  ,
        'h'  ,
        'H'  ,
        '+'  ,
        'x'  ,
        'D'  ,
        'd'  ,
        '|'  ,
        '_'  ,
        )

    markerd = dict([(s,i) for i,s in enumerate(markers)])

    def __init__(self, lines):

        datadir = matplotlib.get_data_path()
        gladefile = os.path.join(datadir, 'lineprops.glade')
        if not os.path.exists(gladefile):
            raise IOError('Could not find gladefile lineprops.glade in %s'%datadir)

        self._inited = False
        self._updateson = True # suppress updates when setting widgets manually
        self.wtree = gtk.glade.XML(gladefile, 'dialog_lineprops')
        self.wtree.signal_autoconnect(dict([(s, getattr(self, s)) for s in self.signals]))

        self.dlg = self.wtree.get_widget('dialog_lineprops')

        self.lines = lines

        cbox = self.wtree.get_widget('combobox_lineprops')
        cbox.set_active(0)
        self.cbox_lineprops = cbox

        cbox = self.wtree.get_widget('combobox_linestyles')
        for ls in self.linestyles:
            cbox.append_text(ls)
        cbox.set_active(0)
        self.cbox_linestyles = cbox

        cbox = self.wtree.get_widget('combobox_markers')
        for m in self.markers:
            cbox.append_text(m)
        cbox.set_active(0)
        self.cbox_markers = cbox
        self._lastcnt = 0
        self._inited = True


    def show(self):
        'populate the combo box'
        self._updateson = False
        # flush the old
        cbox = self.cbox_lineprops
        for i in range(self._lastcnt-1,-1,-1):
            cbox.remove_text(i)

        # add the new
        for line in self.lines:
            cbox.append_text(line.get_label())
        cbox.set_active(0)

        self._updateson = True
        self._lastcnt = len(self.lines)
        self.dlg.show()

    def get_active_line(self):
        'get the active line'
        ind = self.cbox_lineprops.get_active()
        line = self.lines[ind]
        return line


    def get_active_linestyle(self):
        'get the active lineinestyle'
        ind = self.cbox_linestyles.get_active()
        ls = self.linestyles[ind]
        return ls

    def get_active_marker(self):
        'get the active lineinestyle'
        ind = self.cbox_markers.get_active()
        m = self.markers[ind]
        return m

    def _update(self):
        'update the active line props from the widgets'
        if not self._inited or not self._updateson: return
        line = self.get_active_line()
        ls = self.get_active_linestyle()
        marker = self.get_active_marker()
        line.set_linestyle(ls)
        line.set_marker(marker)

        button = self.wtree.get_widget('colorbutton_linestyle')
        color = button.get_color()
        r, g, b = [val/65535. for val in color.red, color.green, color.blue]
        line.set_color((r,g,b))

        button = self.wtree.get_widget('colorbutton_markerface')
        color = button.get_color()
        r, g, b = [val/65535. for val in color.red, color.green, color.blue]
        line.set_markerfacecolor((r,g,b))

        line.figure.canvas.draw()



    def on_combobox_lineprops_changed(self, item):
        'update the widgets from the active line'
        if not self._inited: return
        self._updateson = False
        line = self.get_active_line()

        ls = line.get_linestyle()
        if ls is None: ls = 'None'
        self.cbox_linestyles.set_active(self.linestyled[ls])

        marker = line.get_marker()
        if marker is None: marker = 'None'
        self.cbox_markers.set_active(self.markerd[marker])

        r,g,b = colorConverter.to_rgb(line.get_color())
        color = gtk.gdk.Color(*[int(val*65535) for val in r,g,b])
        button = self.wtree.get_widget('colorbutton_linestyle')
        button.set_color(color)

        r,g,b = colorConverter.to_rgb(line.get_markerfacecolor())
        color = gtk.gdk.Color(*[int(val*65535) for val in r,g,b])
        button = self.wtree.get_widget('colorbutton_markerface')
        button.set_color(color)
        self._updateson = True

    def on_combobox_linestyle_changed(self, item):
        self._update()

    def on_combobox_marker_changed(self, item):
        self._update()

    def on_colorbutton_linestyle_color_set(self, button):
        self._update()

    def on_colorbutton_markerface_color_set(self, button):
        'called colorbutton marker clicked'
        self._update()

    def on_dialog_lineprops_okbutton_clicked(self, button):
        self._update()
        self.dlg.hide()

    def on_dialog_lineprops_cancelbutton_clicked(self, button):
        self.dlg.hide()

# set icon used when windows are minimized, it requires
# gtk.pygtk_version >= (2,2,0) with a GDK pixbuf loader for SVG installed
try:
    gtk.window_set_default_icon_from_file (
        os.path.join (matplotlib.rcParams['datapath'], 'matplotlib.svg'))
except:
    verbose.report('Could not load matplotlib icon: %s' % sys.exc_info()[1])


def error_msg_gtk(msg, parent=None):
    if parent is not None: # find the toplevel gtk.Window
        parent = parent.get_toplevel()
        if parent.flags() & gtk.TOPLEVEL == 0:
            parent = None

    if not is_string_like(msg):
        msg = ','.join(map(str,msg))

    dialog = gtk.MessageDialog(
        parent         = parent,
        type           = gtk.MESSAGE_ERROR,
        buttons        = gtk.BUTTONS_OK,
        message_format = msg)
    dialog.run()
    dialog.destroy()


FigureManager = FigureManagerGTK
