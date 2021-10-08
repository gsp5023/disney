/*
 * libwebsockets - small server side websockets and web server implementation
 *
 * Copyright (C) 2010 - 2020 Andy Green <andy@warmcat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#if defined(LWS_WITH_GLIB)
#if defined(__APPLE__)
#include <glib.h>
#else
#include <glib-2.0/glib.h>
#endif
#endif /* LWS_WITH_GLIB */

struct lws_pt_eventlibs_glib {
	GMainLoop	*loop;
	guint		hrtimer_tag;
	guint		sigint_tag;
	guint		idle_tag;
};

struct lws_io_watcher_glib_subclass {
	GSource		base;
	struct lws	*wsi;
	gpointer	tag;
};

/*
 * One of these is embedded in each wsi
 */

struct lws_io_watcher_glib {
	struct lws_io_watcher_glib_subclass *source;	/* these are created and destroyed by glib */
};

struct lws_context_eventlibs_glib {
	//int placeholder;
};

extern struct lws_event_loop_ops event_loop_ops_glib;
