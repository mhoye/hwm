/* Hwm 0.1 by Mike Hoye <mhoye@bespokeio.com>, written in mid-2012 out of pure spite.
 *
 * Hwm is intended to be opened as a secondary X session in addition to your WM of
 * choice. The design goals for Hwm were correctly-sized fullscreen terminals, 
 * an equivalent of multimonitor support for consoles and having power management
 * work on laptops, because it's 2012 and that's still a thing.
 *
 * It's derived from TinyWM, written by Nick Welch <nick@incise.org> in 2005 & 2011.
 * Thank you, Mr. Welch; your annotated version of tinywm.c was an excellent guide.
 * 
 * History: 
 *          0.1 - Initial release. Seems to sort of work kind of. Assumptions are
 *                documented in the code, and may be mistaken.
 *
 * Known issues: 
 *
 *          - This is prerelease software. If it breaks, please enjoy both pieces.
 *          - Power management isn't working properly yet for sure.
 *          - Many monitor configurations (hardware and rotation) are untested.
 *
 * Requirements:  gcc, the C standard libraries, Xlib, the Xinerama and XRandR libraries. 
 *
 * License:    
 *          Hwm is distributed unter the GNU Public License, version 3 or later.   
 *
 *          This program is free software: you can redistribute it and/or modify
 *          it under the terms of the GNU General Public License as published by
 *          the Free Software Foundation, either version 3 of the License, or
 *          (at your option) any later version.
 *
 *          This program is distributed in the hope that it will be useful,
 *          but WITHOUT ANY WARRANTY; without even the implied warranty of
 *          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 *          GNU General Public License for more details.
 *
 *          For more information, see <http://www.gnu.org/licenses/>.
 */

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

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
    * this: when you detach a monitor, walk the list of xterminals (without even bothering to
    * figure out which one was on what monitor, resize the first N terminals it finds in its
    * list to fit the N remaining monitors and kills the rest, regardless of what they're doing.
    *
    * This is not a bug, it's an design decision. If it's a problem for you,
    * I encourage you to use screen, or dtach, indeed anyone else's software but mine.
    */

    int curscr;
    pid_t pid;
    char xterm_cmd[64]; /* This program will fail if we ever have sixteen-digit screen
                           resolutions, but if you're still using this program when pixel 
                           densities get to that point then hey bro wtf r u doin */
    char tmp[12];

    xterm_cmd[0] = tmp[0] = NULL;
    XineramaScreenInfo * xsi = XineramaQueryScreens(dpy, &curscr); 
    
    if ( curscr == *prevscr) {
      /* I'm assuming that "receiving an XRRScreenChangeNotifyEvent" is atomic - 
       * that is, it could be a screen being added, moved or rotated, but it is 
       * never more than one of those things at once. If I'm wrong about that 
       * this program is horribly broken and you shouldn't use it.  We do 
       * nothing in this case now, but later we're going to refit all the screens 
       * in this step on the assumption that one of them's been rotated. 
       */
       return (0);

    } 
    else if ( curscr < *prevscr) {
        if ( xterm_pids[curscr -1 ] > 1 ) {
          kill( xterm_pids[curscr - 1], 9); /* Children shouldn't play with guns. */
        } else {
          return(ASSUMPTION_ERROR);
        } 
        xterm_pids[curscr -1] = -1;
        return (0);
      }
    else if ( curscr > *prevscr ) {
        pid = fork();
        if (-1 == pid ) { 
            return(ASSUMPTION_ERROR); 
        }
        else if (pid == 0) 
        { 
             strcat(xterm_cmd, XTERM_BINARY);
             strcat(xterm_cmd, " -geometry "); /* spaces important */
             sprintf(tmp, "%d", xsi[curscr-1].width);
             strcat(xterm_cmd, tmp);
             strcat(xterm_cmd, "x");
             sprintf(tmp, "%d", xsi[curscr-1].height);
             strcat(xterm_cmd,tmp);
             strcat(xterm_cmd, "+");
             sprintf(tmp, "%d", xsi[curscr-1].x_org);
             strcat(xterm_cmd,tmp);
             strcat(xterm_cmd, "+");
             sprintf(tmp, "%d", xsi[curscr-1].y_org);
             strcat(xterm_cmd,tmp);
  
            system(xterm_cmd); 
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


int main(int argc, char* argv[], char* envp[])
{
    Display * dpy;
    XineramaScreenInfo * xinf;
    pid_t xterm_pids[MAXSCREENS];  
    int screens=0;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if(!(dpy = XOpenDisplay(NULL))) { return 1; }

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F9")), Mod1Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

    frobAllTheTerms(dpy, &screens, xterm_pids);

    for(;;)
    {
        XNextEvent(dpy, &ev);

        if(ev.type == RRScreenChangeNotify) {
          frobAllTheTerms(dpy, &screens, xterm_pids);
        }
        else if(ev.type == KeyPress && ev.xkey.subwindow != None){
            XRaiseWindow(dpy, ev.xkey.subwindow);
            return(0);
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

