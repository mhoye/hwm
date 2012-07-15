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
#include <X11/extensions/Xrandr.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define LOGFILE "/tmp/hwmlog"

#define XTERM_BINARY "/usr/bin/xterm"
#define ASSUMPTION_ERROR -100
#define FUNDAMENTAL_ERROR -102
#define FORK_ERROR        -103

#define MAXSCREENS 16


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
    int curscr, pid;
    char xterm_opts[64]; /* This program will fail if we ever have sixteen-digit screen
                           resolutions, but if you're still using this program when pixel 
                           densities get to that point, um, hey bro wtf r u doin */
    char tmp[12];
   
   
    XineramaScreenInfo * xsi = XineramaQueryScreens(dpy, &curscr); 

    if ( curscr == (int) &prevscr) {
      /* I'm assuming that "receiving an XRRScreenChangeNotifyEvent"
      is an atomic
       * event - that is, it could be a screen being added, moved or
       rotated, but * it is never more than one of those things at
       once. If I'm wrong about that * this program is horribly broken
       and you shouldn't use it.  * * We do nothing in this case, because
       we're going to resize all the screens * after all the options as
       a last step before leaving this function.  * */
    } else if ( curscr > (int) &prevscr) {
        if ( xterm_pids[curscr -1 ] > 1 ) {
          kill( xterm_pids[curscr - 1], 9); /* Children shouldn't 
                                               play with guns. */
        } else {
          return(ASSUMPTION_ERROR);
        } xterm_pids[curscr -1] = -1;
      }
    else if ( curscr < (int) &prevscr ) {
        /* Figure out where the new screen is, then fork an xTerminal
        out to it. */ 
        pid = fork();
        if (-1 == pid ) { 
            return(ASSUMPTION_ERROR); 
        }
        else if (pid == 0) 
        { //child process.....
             xterm_opts[0] = NULL;
             strcat( xterm_opts, " -geometry ");
             sprintf(tmp, "%d", xsi[curscr-1].width);
             strcat(xterm_opts, tmp);
             strcat(xterm_opts, "+");
             sprintf(tmp, "%d", xsi[curscr-1].height);
             strcat(xterm_opts,tmp);
             strcat(xterm_opts, "+");
             sprintf(tmp, "%d", xsi[curscr-1].x_org);
             strcat(xterm_opts,tmp);
             strcat(xterm_opts, "+");
             sprintf(tmp, "%d", xsi[curscr-1].y_org);
             strcat(xterm_opts,tmp);

            execv(XTERM_BINARY, xterm_opts);
            return(0); //child bails...
        }
        else {
          xterm_pids[curscr-1] = pid;
          *prevscr = curscr;
          return(0);
        }
    }

    /* We should never, ever get here. */

   return(FUNDAMENTAL_ERROR); 
}


int main(void)
{
    Display * dpy;
    XineramaScreenInfo * xinf;
    pid_t xterm_pids[MAXSCREENS];  
    int screens;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    FILE * log;

    log = fopen(LOGFILE, "a+");
    
    fprintf(log, "Starting Hwm.\n");

    if(!(dpy = XOpenDisplay(0x0))) return 1;

    /* Now, find out: How many actual physical monitors do we have here? */
      
    xinf = XineramaQueryScreens(dpy, &screens);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F2")), Mod1Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    fprintf(log, "Entering event loop...\n");

    for(;;)
    {
        XNextEvent(dpy, &ev);

        if(ev.type == RRScreenChangeNotify) {
          frobAllTheTerms(dpy, &screens, xterm_pids);
        }
        else if(ev.type == KeyPress && ev.xkey.subwindow != None){
            XRaiseWindow(dpy, ev.xkey.subwindow);
        }
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        else if(ev.type == ButtonRelease)
        {
            start.subwindow = None;
        }

    }
}



