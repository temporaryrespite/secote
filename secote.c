//src: https://github.com/jumper149/blugon/blob/baf79b5b19f3387b4186fc5dacd31a7c47e22144/backends/scg/scg.c#L1
//modified to act like: "blugon(v1.11.3) -r -o -b scg" (and/or "-b tty", if not ran inside X)  (where /home/user/.config/blugon/current returns eg. "3000.0" Kelvin - can be passed as arg)
//so now you can run it like this(kelvin color value):
// ./secote 3000
//or like this(the equivalent rgb gamma colors for that kelvin value):
// ./secote 1.0 0.6949030005552019 0.4310480202110507
// All I needed was something to set color to 3000 Kelvin, instead of all the functionality that blugon provides.
// note to self: although blugon doesn't exist on Gentoo(and only as AUR on archlinux), there exists something similar(and more well known) (also on archlinux) RedShift https://wiki.gentoo.org/wiki/Redshift and it can set color temperature (just like this very program whose source code you're reading is doing) like this: "$ redshift -P -O 3000" ("-O TEMP  One shot manual mode (set color temperature)" and "-P    Reset existing gamma ramps before applying new color effect")  that assumes "-m randr", for tty/console you should "-m drm"
#ifdef APPLY_TESTS
  #ifndef _POSIX_C_SOURCE
      //#define _POSIX_C_SOURCE >= 199309L  //operator '>=' has no left operand  (if placed at top of this c file, instead of just before the time.h include, which is seen below) - placed at top because other includes will include 'time.h' before our time.h include directive is reached, most likely.
  #define _POSIX_C_SOURCE 199309L
  #endif
#endif
//XXX all include directives should be placed below:
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include <stdlib.h>     /* others, strtoul */
#include <math.h>       /* pow */
#include <stdio.h>      /* printf */
#include <inttypes.h>   /* PRIx64 */
#ifdef APPLY_TESTS
  //#include <unistd.h>     /* usleep */ // not in c99
  //src: https://github.com/ewsi/dcstad/pull/6/files
  #include <time.h> /* nanosleep, timespec */ // nanosleep&timespec are not in c99, unless you set _POSIX_C_SOURCE to 199309L (already done at the top) src: https://ubuntuforums.org/showthread.php?t=1146543&p=7200708#post7200708
  //#include <sys/time.h> /* timespec */ //hmm "The <sys/time.h> header, included by <time.h>, defines various structures related to time and timers." man 3bsd timespec
  //#include <bits/types/struct_timespec.h> /* timespec */ // #ifdef __USE_GNU  ?
#endif

#ifdef APPLY_TESTS
  //#include <float.h>      /* DECIMAL_DIG */
  //^ don't need this anymore!
  #include <string.h>   /* strncmp */
#else //disable 'assert' if not already disabled!
  #ifndef NDEBUG
    //assert is disabled in release mode.
    #define NDEBUG
  #endif
#endif
//NDEBUG must be defined(if so), BEFORE including assert.h
#include <assert.h>     /* assert */

//global only when testing! so it can be accessed from within tty
#ifdef APPLY_TESTS
  double kelvin;
#endif

////https://stackoverflow.com/questions/58009680/convert-ascii-byte-char-array-to-hex-byte-array/58009844#58009844
//typedef unsigned char Byte;
//void byte_to_hex(Byte b, Byte dest[2]) {
//  const char *digits = "0123456789ABCDEF";
//}
//printf("blah '%02x' '%02x' '%02x'\n", 0x03,0x20, 0xF);

//src: https://github.com/jumper149/blugon/blob/master/blugon.py#L63-L79
//how to array of fixed length strings src: https://stackoverflow.com/questions/33587981/how-to-create-array-of-fixed-length-strings-in-c/33588021#33588021
#define NUM_COLORS 16
static const char COLOR_TABLE[NUM_COLORS][3*2+1] = {
  // VGA colors from https://en.wikipedia.org/wiki/ANSI_escape_code
 //RRGGBB
  "000000", //black
  "aa0000", //darkgrey
  "00aa00", //darkred
  "aa5500", //red
  "0000aa", //darkgreen
  "aa00aa", //green
  "00aaaa", //brown
  "aaaaaa", //yellow
  "555555", //darkblue
  "ff5555", //blue
  "55ff55", //darkmagenta
  "ffff55", //magenta
  "5555ff", //darkcyan
  "ff55ff", //cyan
  "55ffff", //lightgrey
  "ffffff"  //white
};

#ifdef APPLY_TESTS
static const char Kelvin_3000[NUM_COLORS][1+3*2+1] = {
  "0000000",
  "1aa0000",
  "2007600",
  "3aa3b00",
  "4000049",
  "5aa0049",
  "6007649",
  "7aa7649",
  "8553b24",
  "9ff3b24",
  "A55b124",
  "Bffb124",
  "C553b6d",
  "Dff3b6d",
  "E55b16d",
  "Fffb16d"
};
#endif

int8_t onehexchar2dec(const char c) {
  if ((c >= '0')&&(c <= '9')) {
    return c - '0';
  } else if ((c >= 'A') && (c <= 'F')) {
    return c - 'A' + 10;
  } else if ((c >= 'a') && (c <= 'f')) {
    return c - 'a' + 10;
  } else {
    fprintf(stderr, "Invalid hex char '%c'\n'",c);
    return -1;
  }
  //switch(c) {
    //case '0'...'9': // range expressions in switch statements are non-standard [-Wpedantic]
}

uint8_t twocharhex2dec(const char *ptr_to_first_char) {
  int16_t res = onehexchar2dec(ptr_to_first_char[0]) * 16 + onehexchar2dec(ptr_to_first_char[1]);
  assert(res<=255);
  assert(res>=0); // if onehexchar2dec failed!
  return (uint8_t)res;
}

void tty_gamma(double r, double g, double b) {
  //static const uint8_t esc=033; //033 in octal, 27 in decimal, or echo -ne '\e' in bash
  // for the console/tty aka TERM=linux (see `man 4 console_codes` search for "ESC ] P")
  const char *color_rgb; //a hex string of RRGGBB for any of the 16 colors from COLOR_TABLE
  double dr,dg,db;
  for (int color_num=0; color_num < NUM_COLORS; color_num++) {
    color_rgb=COLOR_TABLE[color_num];
    dr = r * twocharhex2dec(&color_rgb[0]);
    dg = g * twocharhex2dec(&color_rgb[2]);
    db = b * twocharhex2dec(&color_rgb[4]);
    if (dr>255) { dr = 255; }
    if (dg>255) { dg = 255; }
    if (db>255) { db = 255; }
    //note: vararg args are auto-converted to int if they're less than int (thanks TheBeastie on ##c freenode irc)
    printf("\033]P%X%02x%02x%02x", color_num, (int)dr, (int)dg, (int)db);//truncated r,g,b values!
#ifdef APPLY_TESTS
    //doneTODO: verify values are correct here for 3000 Kelvin
    if (3000.0 == kelvin) {
      printf("verifying 3000 kelvin tty correctness\n");
      char temp[1+2*3+1]; //the extra +1 is for \0
      //TODO: figure out how to tell ALE to not show the warning/suggestion for snprintf_s:
      snprintf(temp, 1+2*3+1, "%X%02x%02x%02x", color_num, (int)dr, (int)dg, (int)db);
      if (0 != strncmp(Kelvin_3000[color_num], temp, 1+2*3+1)){
        fprintf(stderr, "Failed: got '%s', expected '%s'\n", temp, Kelvin_3000[color_num]);
        assert(0);
      }
    } else {
      printf("NOT verifying 3000 kelvin tty correctness\n");
    }
#endif
  } // for
} // function

void rgb_to_gamma(double color, double *gamma) {
  // src: https://github.com/jumper149/blugon/blob/baf79b5b19f3387b4186fc5dacd31a7c47e22144/blugon.py#L234-L269
  if (color < 0) {
    color = 0;
  } else if (color > 255) {
    color = 255;
  }
  *gamma = color / 255;
}

void kelvin_to_gamma(double kelvin, double *gamma_r, double *gamma_g, double *gamma_b) {
  // Transforms temperature in Kelvin to Gamma values between 0 and 1.
  // src: https://github.com/jumper149/blugon/blob/baf79b5b19f3387b4186fc5dacd31a7c47e22144/blugon.py#L234-L269
  // eg. kelvin 3000.0
#ifdef APPLY_TESTS
  double savedkelvin=kelvin;
#endif
  kelvin = kelvin / 100;
  if (kelvin <= 66) {
    *gamma_r = 255; //red
    *gamma_g = kelvin; // green
    *gamma_g = 99.4708025861 * log(*gamma_g) - 161.1195681661;
  } else {
    //red:
    *gamma_r = kelvin - 60;
    *gamma_r = 329.698727446 * pow(*gamma_r, -0.1332047592);
    //green:
    *gamma_g = kelvin - 60;
    *gamma_g = 288.1221695283 * pow(*gamma_g, -0.0755148492);
  }
  //blue:
  if (kelvin <= 10) {
    *gamma_b = 0;
  } else if (kelvin >= 66) {
    *gamma_b = 255;
  } else {
    *gamma_b = kelvin - 10;
    *gamma_b = 138.5177312231 * log(*gamma_b) - 305.0447927307;
  }
#ifdef APPLY_TESTS
  static const int precision=16; //how many digits after the '.' in printf-ing the floats
  printf("before: r=%lf, g=%lf, b=%lf\n", *gamma_r, *gamma_g, *gamma_b); // 255 177.2002651415765 109.91724515381793
  //printf("before: r=%.*e, g=%.*e, b=%.*e\n", DECIMAL_DIG, *gamma_r, DECIMAL_DIG, *gamma_g, DECIMAL_DIG, *gamma_b); // 255 177.2002651415765 109.91724515381793
  printf("before: r=%.*f, g=%.*f, b=%.*f\n", precision, *gamma_r, precision, *gamma_g, precision, *gamma_b); // 255 177.2002651415765 109.91724515381793
  // DECIMAL_DIG 10, 21 (widest supported floating type)  (C99) src: https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value#19897395
  // DBL_DECIMAL_DIG  10, 17 (double)                          (C11)
  if (savedkelvin == 3000.0) {
    printf("expected: r=255 g=177.2002651415765 b=109.91724515381793\n");
    assert(255.0 == *gamma_r);
    assert(177.2002651415765 == *gamma_g);
    assert(109.91724515381793 == *gamma_b);
    printf("passed\n");
  }
#endif
  rgb_to_gamma(*gamma_r, gamma_r);
  rgb_to_gamma(*gamma_g, gamma_g);
  rgb_to_gamma(*gamma_b, gamma_b);
#ifdef APPLY_TESTS
  printf("after : r=%lf, g=%lf, b=%lf\n", *gamma_r, *gamma_g, *gamma_b); // 1.0 0.6949030005552019 0.4310480202110507
  //printf("after : r=%.*e, g=%.*e, b=%.*e\n", DECIMAL_DIG, *gamma_r, DECIMAL_DIG, *gamma_g, DECIMAL_DIG, *gamma_b); // 1.0 0.6949030005552019 0.4310480202110507
  printf("after : r=%.*f, g=%.*f, b=%.*f\n", precision, *gamma_r, precision, *gamma_g, precision, *gamma_b); // 1.0 0.6949030005552019 0.4310480202110507
  if (savedkelvin == 3000.0) {
    printf("expected: r=1.0 g=0.6949030005552019 b=0.4310480202110507\n");
    assert( 1.0 == *gamma_r );
    assert(  0.6949030005552019== *gamma_g );
    assert(  0.4310480202110507== *gamma_b );
    printf("passed\n");
  }
#endif
}

// trap code from: https://stackoverflow.com/questions/57715866/have-x11-c-program-compiled-getting-undefined-reference-errors-what-libs-are
static int trapped_error_code = 0;
static int (*old_error_handler) (Display *, XErrorEvent *);

static int
error_handler(Display     *display, XErrorEvent *error) {
    trapped_error_code = error->error_code;
    return 0;
}

void
trap_errors(void) {
    trapped_error_code = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

int
untrap_errors(void) {
    XSetErrorHandler(old_error_handler);
    return trapped_error_code;
}


int main(int argc, char **argv) {
#ifdef APPLY_TESTS
  printf("%s %s\n",
#ifdef DEBUG
      "DEBUG"
#else
      ""
#endif
      ,
#ifdef NDEBUG
      "NDEBUG"
#else
      ""
#endif
      );
#endif

  double gamma_r;
  double gamma_g;
  double gamma_b;
#ifndef APPLY_TESTS
  double kelvin;
#endif

#ifdef APPLY_TESTS
    printf("argc=%d\n", argc);
#else
    assert(1==2);//shouldn't happen(ie. asserts are disabled in release mode), else bad coding thus far on my part!
#endif
  /* parsing */
  if (argc == 4) {
    gamma_r = atof(argv[1]);
    gamma_g = atof(argv[2]);
    gamma_b = atof(argv[3]);
  }else if (argc == 2) {
    kelvin = atof(argv[1]); // eg. 3000.0

//    printf("Hi1\n");
//    assert(1 == 2); // assert works if NDEBUG isn't defined, regardless of DEBUG
//    printf("Hi2\n");
#if !defined(NDEBUG)
    //sanity check, only executes if 'assert' is enabled!
    kelvin_to_gamma(3000.0, &gamma_r, &gamma_g, &gamma_b);
    assert( 1.0 == gamma_r );
    assert( 0.6949030005552019 == gamma_g );
    assert( 0.4310480202110507 == gamma_b );
#endif

    kelvin_to_gamma(kelvin, &gamma_r, &gamma_g, &gamma_b);
    //tty_gamma(gamma_r, gamma_g, gamma_b); return 2;//FIXME: remove this
  } else {
    printf("Usage: %s { <Kelvin_temperature_value> | <red_gamma_value> <green_gamma_value> <blue_gamma_value> }\n", argv[0]); // https://stackoverflow.com/questions/9725675/is-there-a-standard-format-for-command-line-shell-help-text/47026846#47026846
    printf("The gamma values are between 0 and 1, as a float/double value, like blugon v1.11.3 shows as \"Calculated RGB Gamma values:\" with -V arg.\n");
    printf("Example: %s 3000\n", argv[0]);
    printf("Example: %s 3000.0\n", argv[0]);
    printf("Example: %s 1.0 0.6949030005552019 0.4310480202110507\n", argv[0]);
    printf("Example: %s 1  0.6949030005552019  0.4310480202110507\n", argv[0]);
    printf("This program is based on blugon v1.11.3 https://github.com/jumper149/blugon\n");
    printf("You're supposed to run this under X, or on console aka TERM=linux\n");
    return 1;
  }

  Display *dpy = XOpenDisplay(NULL);
  //If XOpenDisplay does not succeed, it returns NULL.
  if (NULL == dpy) {
    fprintf(stderr, "X is not running? or cannot open a connection to it. Is DISPLAY env var set? Trying tty/console(aka TERM=linux) version...\n");
    //doneTODO: try tty backend here, TODO: maybe unless TERM isn't "linux" ?
    // The console/tty aka TERM=linux (see `man 4 console_codes` search for "ESC ] P") version:
    tty_gamma(gamma_r, gamma_g, gamma_b);
    return 0; //we can't know if the above worked or failed, so returning success.
  }
  // The X version:
  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root); // available in RandR 1.3 or higher
  int num_crtcs = res->ncrtc;
  for (int c = 0; c < num_crtcs; c++) {
    assert(res->ncrtc == num_crtcs); //doneTODO: find out if this can change by the below XRRGetCrtcInfo call! it can't, only resources->configTimestamp is accessed in XRRGetCrtcInfo as per src/XrrCrtc.c
    int crtcxid = res->crtcs[c];
    // //XRRCrtcInfo *crtc_info =  //no point in saving this?!
    //   XRRGetCrtcInfo(dpy, res, crtcxid); //wait, is this useless? seems so
    //   XRRFreeCrtcInfo(crtc_info); //should free it eventually

    int size = XRRGetCrtcGammaSize(dpy, crtcxid);
    XRRCrtcGamma *crtc_gamma = XRRAllocGamma(size);

    for (int i = 0; i < size; i++) {
      double g = 65535.0 * i / size;
      crtc_gamma->red[i]   = g * gamma_r;
      crtc_gamma->green[i] = g * gamma_g;
      crtc_gamma->blue[i]  = g * gamma_b;
    }
    XRRSetCrtcGamma(dpy, crtcxid, crtc_gamma);

    XFree(crtc_gamma);
  }
#ifdef APPLY_TESTS
  trap_errors();
#endif
  XRRFreeScreenResources(res);
  XCloseDisplay(dpy); //calls XSync which calls XFlush
#ifdef APPLY_TESTS
  const static struct timespec slp = { 0, 10 * 1000 * 1000};
//  static struct timespec slp;
//  slp.tv_sec = 0;
//  slp.tv_nsec= 10 * 1000 * 1000;
  //usleep(10000); // implicit declaration of function ‘usleep’; did you mean ‘sleep’?  https://github.com/ewsi/dcstad/issues/5
  nanosleep(&slp, NULL);
  if (untrap_errors()) {
    /* Handle errors */
    fprintf(stderr, "Errors detected. TODO: handle or report them.\n"); // this isn't reached in our case! even without calling XRRFreeScreenResources but calling XCloseDisplay
  }
#endif
  return 0; //returning success
}

// in retrospect, I probably should've at least looked at these links(I still haven't):
/*
 * This program(blugon's scg.c file) is based on 'sct'.
 * Source: https://https.www.google.com.tedunangst.com/flak/post/sct-set-color-temperature
 *         https://https.www.google.com.tedunangst.com/flak/files/sct.c
 */
