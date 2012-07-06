/* XterWM was written by Mike Hoye <mhoye@bespokeio.com> in mid-2012 out of pure spite.
 *
 * XterWM is intended to be opened as a secondary X session in addition to your WM of
 * choice. The sole design goals for XterWM were correctly-sized fullscreen terminals
 * and not going completely insane 
 * 
 *
 * TinyWM written by Nick Welch <nick@incise.org> in 2005 & 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY. */

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <sys/types.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAXSCREENS 16


int main(void)
{
    Display * dpy;
    XWindowAttributes attr;
    XineramaScreenInfo * xinf;
      
    int screens;
    XButtonEvent start;
    XEvent ev;


    /* If we can't open a display on which Xinerama is active, bail immediately. */

    if(!(dpy = XOpenDisplay(0x0)) && XineramaIsActive(dpy) ) return 1;

    /* Now, find out: How many actual physical monitors do we have here? */
    
    xinf = XineramaQueryScreens(dpy, &screens);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;
    for(;;)
    {
        XNextEvent(dpy, &ev);

	/* here's where it goes - if you add a new monitor, manage it, if you remove it, kill whatever was shown in it.*/


        if(ev.type == KeyPress && ev.xkey.subwindow != None)
            XRaiseWindow(dpy, ev.xkey.subwindow);
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        else if(ev.type == MotionNotify && start.subwindow != None)
        {
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, start.subwindow,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
        {
            start.subwindow = None;
        }

        else if(ev.type == XRRScreenChangeNotifyEvent)
        {
            if(0 != frobAllTheTerms(dpy, xinf, screens)) return 3;

        }

    }
}

int frobAllTheTerms(Display * dpy, XineramaScreenInfo * xinf, int prev_screens)
{
   /* This is pretty straigtforward. The program has a list of xterms that are running, 
    * and a list of phyusical screens it extracts from Xinerama. 

