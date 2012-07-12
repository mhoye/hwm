/* Hwm was written by Mike Hoye <mhoye@bespokeio.com> in mid-2012 out of pure spite.
 *
 * Hwm is intended to be opened as a secondary X session in addition to your WM of
 * choice. The sole design goals for Hwm were correctly-sized terminals
 * and not going completely insane 
 * 
 * It's derived from 
 * TinyWM written by Nick Welch <nick@incise.org> in 2005 & 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY. */

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define XTERM_BINARY "/usr/bin/xterm"
#define ASSUMPTION_ERROR -100
#define FUNDAMENTAL_ERROR -102
#define FORK_ERROR        -103

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAXSCREENS 16


int main(void)
{
    Display * dpy;
    XWindowAttributes attr;
    XineramaScreenInfo * xinf;
    pid_t xterm_pids[MAXITEMS];  
    int screens, ret;;
    XButtonEvent start;
    XEvent ev;


    /* If we can't open a display on which Xinerama is active, bail immediately. */

    if(!(dpy = XOpenDisplay(0x0)) && XineramaIsActive(dpy) ) return 1;

    /* Now, find out: How many actual physical monitors do we have here? */
    
    xinf = XineramaQueryScreens(dpy, * screens);

    /* XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    */ 

    start.subwindow = None;
    for(;;)
    {
        XNextEvent(dpy, &ev);

	/* here's where it goes - if you add a new monitor, manage it, if you remove it, kill whatever was shown in it.*/
        
        if(ev.type == XRRScreenEventNotify)
          frobAllTheTerms(dpy,  xinf, screens, xterm_pids);

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
            if(! (ret = frobAllTheTerms(dpy, screens))) return(ret);
        }

    }
}

int frobAllTheTerms(Display * dpy, int * prevscr, pid_t * xterm_pids)
{
   /* This is pretty straightforward. The program has a list of xterms that are running, 
    * and a list of phyusical screens it extracts from Xinerama. When monitors are attached 
    * add an appropriately-sized Xterm to each new monitor. The part you may like much less is
    * this: when you _de_tach a monitor, walk the list of xterminals (without even bothering to
    * figure out which one was on what monitor, resize the first N terminals it finds in its
    * list to fit the N remaining monitors and kills the rest, regardless of what they're doing.
    *
    * This behavior is not a bug; it's an explicit design decision, and if it's a problem for you,
    * I suggest you use screen, or dtach, or somebody else's window manaand kills the rest, regardless of what they're doing.
    *
    * This behavior is not a bug; it's an explicit design decision, and if it's a problem for you,
    * I suggest you use screen, or dtach, or somebody else's window manager. ger. 
    * 
    */
    int curscr, c, r, pid;
    char* xterm_opts[64]; /* This program will fail if we ever have sixteen-digit screen
                           resolutions, but if you're still using this program when pixel 
                           densities get to that point, um, hey bro wtf r u doin */

    XineramaScreenInfo * xsi = XineramaScreenInfo(dpy, * curscr); 

    // pseudocode: if screen numbers are equal, od nothing, if more
    current screens than pids, walk up and kill them, if smaller create //
    your list of new ones.

    if ( curscr == prevscr) {
      /* I'm assuming that "receiving an XRRScreenChangeNotifyEvent"
      is an atomic
       * event - that is, it could be a screen being added, moved or
       rotated, but * it is never more than one of those things at
       once. If I'm wrong about that * this program is horribly broken
       and you shouldn't use it.  * * We do nothing in this case, because
       we're going to resize all the screens * after all the options as
       a last step before leaving this function.  * */
    } else if ( curscr > prevscr) {
        if ( xterm_pids[curscr -1 ] > 1 ) {
          kill( xterm_pids[curscr - 1], 9) // Children shouldn't play
          with guns
        } else {
          syslog(LOG_USER, "Tried to kill the wrong process,
          exiting...") && return(ASSUMPTION_ERROR);
        } xterm_pids[curscr -1] = -1;
      }
    else if ( curscr < prevscr ) {
        /* Figure out where the new screen is, then fork an xTerminal
        out to it. */ 
        pid = fork();
        if (-1 == pid ) { 
            syslog(LOG_USER, "Can't fork, what is this I don't even. Exiting.");
            return(ASSUMPTION_ERROR); 
        }
        else if (pid = 0) 
        { //child process.....
            xterm_opts = strcat ( "-geometry ", 
                                itoa(xsi[curscr-1].width), "+", 
                                itoa(xsi[curscr-1].height), "+",  
                                itoa(xsi[curscr-1].x_org), "+", 
                                itoa(xsi[curscr-1].y_org) );
            syslog(LOG_USER, "Opening XTerm with options "  
            execv(XTERM_BINARY, xterm_opts);
            return(0); //child bails...
        }
        else {
          xterm_pids[curscr-1] = pid;
          return(0);
        }
    }

    /* We should never, ever get here. */

   return(FUNDAMENTAL_ERROR); 
}


