// Test of -Wformat for GCC-specific formats
// Copy of gcc.dg/format/gcc_diag-10.c, with class pp_element added.
// { dg-do compile }
// { dg-options "-Wformat" }

/* Magic identifiers must be set before the attribute is used.  */

typedef long long __gcc_host_wide_int__;

typedef struct location_s
{
  const char *file;
  int line;
} location_t;

union tree_node;
typedef union tree_node *tree;

/* Define gimple as a dummy type.  The typedef must be provided for
   the C test to find the symbol.  */
typedef struct gimple gimple;

/* Likewise for gimple.  */
typedef struct cgraph_node cgraph_node;

/* Likewise for diagnostic_event_id_t.  */
typedef struct diagnostic_event_id_t diagnostic_event_id_t;

namespace pp_markup { class element; }
typedef pp_markup::element pp_element;

typedef class string_slice string_slice;

#define FORMAT(kind) __attribute__ ((format (__gcc_## kind ##__, 1, 2)))

void diag (const char*, ...) FORMAT (diag);
void cdiag (const char*, ...) FORMAT (cdiag);
void tdiag (const char*, ...) FORMAT (tdiag);
void cxxdiag (const char*, ...) FORMAT (cxxdiag);
void dump (const char*, ...) FORMAT (dump_printf);

void test_diag (tree t, gimple *gc, diagnostic_event_id_t *event_id_ptr,
		pp_element *elem)
{
  diag ("%<");   /* { dg-warning "unterminated quoting directive" } */
  diag ("%>");   /* { dg-warning "unmatched quoting directive " } */
  diag ("%<foo%<bar%>%>");   /* { dg-warning "nested quoting directive" } */

  diag ("%G", gc); /* { dg-warning "format" } */
  diag ("%K", t); /* { dg-warning "format" } */
  diag ("%@", event_id_ptr);

  diag ("%R");       /* { dg-warning "unmatched color reset directive" } */
  diag ("%r", "");   /* { dg-warning "unterminated color directive" } */
  diag ("%r%r", "", "");   /* { dg-warning "unterminated color directive" } */
  diag ("%r%R", "");
  diag ("%r%r%R", "", "");
  diag ("%r%R%r%R", "", "");

  diag ("%<%R%>");      /* { dg-warning "unmatched color reset directive" } */
  diag ("%<%r%>", "");  /* { dg-warning "unterminated color directive" } */
  diag ("%<%r%R%>", "");

  diag ("%e", elem);
  diag ("%e", 42); /* { dg-warning "format" } */
}

void test_cdiag (tree t, gimple *gc, string_slice *s)
{
  cdiag ("%<");   /* { dg-warning "unterminated quoting directive" } */
  cdiag ("%>");   /* { dg-warning "unmatched quoting directive " } */
  cdiag ("%<foo%<bar%>%>");   /* { dg-warning "nested quoting directive" } */

  cdiag ("%D", t);       /* { dg-warning ".D. conversion used unquoted" } */
  cdiag ("%E", t);
  cdiag ("%F", t);       /* { dg-warning ".F. conversion used unquoted" } */
  cdiag ("%G", gc);      /* { dg-warning "format" } */
  cdiag ("%K", t);       /* { dg-warning "format" } */
  cdiag ("%B", s);

  cdiag ("%R");       /* { dg-warning "unmatched color reset directive" } */
  cdiag ("%r", "");   /* { dg-warning "unterminated color directive" } */
  cdiag ("%r%r", "", "");   /* { dg-warning "unterminated color directive" } */
  cdiag ("%r%R", "");
  cdiag ("%r%r%R", "", "");
  cdiag ("%r%R%r%R", "", "");

  cdiag ("%T", t);       /* { dg-warning ".T. conversion used unquoted" } */
  cdiag ("%V", t);       /* { dg-warning ".V. conversion used unquoted" } */

  cdiag ("%<%D%>", t);
  cdiag ("%<%E%>", t);
  cdiag ("%<%F%>", t);
  cdiag ("%<%G%>", gc);  /* { dg-warning "format" } */
  cdiag ("%<%K%>", t);   /* { dg-warning "format" } */
  cdiag ("%<%B%>", s);

  cdiag ("%<%R%>");      /* { dg-warning "unmatched color reset directive" } */
  cdiag ("%<%r%>", "");  /* { dg-warning "unterminated color directive" } */
  cdiag ("%<%r%R%>", "");

  cdiag ("%<%T%>", t);
  cdiag ("%<%V%>", t);

  cdiag ("%<%qD%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  cdiag ("%<%qE%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  cdiag ("%<%qT%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  cdiag ("%<%qB%>", s);  /* { dg-warning ".q. flag used within a quoted sequence" } */
}

void test_tdiag (tree t, gimple *gc, string_slice *s)
{
  tdiag ("%<");       /* { dg-warning "unterminated quoting directive" } */
  tdiag ("%>");       /* { dg-warning "unmatched quoting directive " } */
  tdiag ("%<foo%<bar%>%>");   /* { dg-warning "nested quoting directive" } */

  tdiag ("%D", t);       /* { dg-warning ".D. conversion used unquoted" } */
  tdiag ("%E", t);
  tdiag ("%G", gc);     /* { dg-warning "format" } */
  tdiag ("%K", t);      /* { dg-warning "format" } */
  tdiag ("%B", s);

  tdiag ("%R");          /* { dg-warning "unmatched color reset directive" } */
  tdiag ("%r", "");   /* { dg-warning "unterminated color directive" } */
  tdiag ("%r%r", "", "");   /* { dg-warning "unterminated color directive" } */
  tdiag ("%r%R", "");
  tdiag ("%r%R", "");
  tdiag ("%r%r%R", "", "");
  tdiag ("%r%R%r%R", "", "");

  tdiag ("%T", t);       /* { dg-warning ".T. conversion used unquoted" } */

  tdiag ("%<%D%>", t);
  tdiag ("%<%E%>", t);
  tdiag ("%<%G%>", gc);  /* { dg-warning "format" } */
  tdiag ("%<%K%>", t);   /* { dg-warning "format" } */

  tdiag ("%<%R%>");      /* { dg-warning "unmatched color reset directive" } */
  tdiag ("%<%r%>", "");  /* { dg-warning "unterminated color directive" } */
  tdiag ("%<%r%R%>", "");

  tdiag ("%<%T%>", t);

  tdiag ("%<%qD%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  tdiag ("%<%qE%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  tdiag ("%<%qT%>", t);  /* { dg-warning ".q. flag used within a quoted sequence" } */
  tdiag ("%<%qB%>", s);  /* { dg-warning ".q. flag used within a quoted sequence" } */
}

void test_cxxdiag (tree t, gimple *gc, string_slice *s)
{
  cxxdiag ("%A", t);     /* { dg-warning ".A. conversion used unquoted" } */
  cxxdiag ("%D", t);     /* { dg-warning ".D. conversion used unquoted" } */
  cxxdiag ("%E", t);
  cxxdiag ("%F", t);     /* { dg-warning ".F. conversion used unquoted" } */
  cxxdiag ("%G", gc);    /* { dg-warning "format" } */
  cxxdiag ("%K", t);     /* { dg-warning "format" } */
  cxxdiag ("%B", s);

  cxxdiag ("%R");        /* { dg-warning "unmatched color reset directive" } */
  cxxdiag ("%r", "");    /* { dg-warning "unterminated color directive" } */
  cxxdiag ("%r%r", "", "");   /* { dg-warning "unterminated color directive" } */
  cxxdiag ("%r%R", "");
  cxxdiag ("%r%R", "");
  cxxdiag ("%r%r%R", "", "");
  cxxdiag ("%r%R%r%R", "", "");

  cxxdiag ("%S", t);     /* { dg-warning ".S. conversion used unquoted" } */
  cxxdiag ("%T", t);     /* { dg-warning ".T. conversion used unquoted" } */
  cxxdiag ("%V", t);     /* { dg-warning ".V. conversion used unquoted" } */
  cxxdiag ("%X", t);     /* { dg-warning ".X. conversion used unquoted" } */

  cxxdiag ("%<%A%>", t);
  cxxdiag ("%<%D%>", t);
  cxxdiag ("%<%E%>", t);
  cxxdiag ("%<%F%>", t);
  cxxdiag ("%<%R%>");    /* { dg-warning "unmatched color reset" } */
  cxxdiag ("%<%r%R%>", "");
  cxxdiag ("%<%S%>", t);
  cxxdiag ("%<%T%>", t);
  cxxdiag ("%<%V%>", t);
  cxxdiag ("%<%X%>", t);
  cxxdiag ("%<%B%>", s);
}

void test_dump (tree t, gimple *stmt, cgraph_node *node, string_slice *s)
{
  dump ("%<");   /* { dg-warning "unterminated quoting directive" } */
  dump ("%>");   /* { dg-warning "unmatched quoting directive " } */
  dump ("%<foo%<bar%>%>");   /* { dg-warning "nested quoting directive" } */

  dump ("%R");       /* { dg-warning "unmatched color reset directive" } */
  dump ("%r", "");   /* { dg-warning "unterminated color directive" } */
  dump ("%r%r", "", "");   /* { dg-warning "unterminated color directive" } */
  dump ("%r%R", "");
  dump ("%r%r%R", "", "");
  dump ("%r%R%r%R", "", "");

  dump ("%<%R%>");      /* { dg-warning "unmatched color reset directive" } */
  dump ("%<%r%>", "");  /* { dg-warning "unterminated color directive" } */
  dump ("%<%r%R%>", "");

  dump ("%E", stmt);
  dump ("%T", t);
  dump ("%G", stmt);
  dump ("%C", node);
  dump ("%f", 1.0);
  dump ("%4.2f", 1.0); /* { dg-warning "format" } */
  dump ("%B", s);
}
