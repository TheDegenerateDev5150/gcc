/* Concrete classes for selftests involving diagnostic paths.
   Copyright (C) 2019-2025 Free Software Foundation, Inc.
   Contributed by David Malcolm <dmalcolm@redhat.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


#include "config.h"
#define INCLUDE_VECTOR
#include "system.h"
#include "coretypes.h"
#include "version.h"
#include "diagnostic.h"
#include "diagnostics/selftest-paths.h"

#if CHECKING_P

namespace diagnostics {
namespace paths {
namespace selftest {

/* class test_path : public diagnostics::paths::path.  */

test_path::
test_path (logical_locations::selftest::test_manager &logical_loc_mgr,
	   pretty_printer *event_pp)
: path (logical_loc_mgr),
  m_test_logical_loc_mgr (logical_loc_mgr),
  m_event_pp (event_pp)
{
  add_thread ("main");
}

/* Implementation of path::num_events vfunc for
   test_path: simply get the number of events in the vec.  */

unsigned
test_path::num_events () const
{
  return m_events.length ();
}

/* Implementation of path::get_event vfunc for
   test_path: simply return the event in the vec.  */

const event &
test_path::get_event (int idx) const
{
  return *m_events[idx];
}

unsigned
test_path::num_threads () const
{
  return m_threads.length ();
}

const thread &
test_path::get_thread (thread_id_t idx) const
{
  return *m_threads[idx];
}

bool
test_path::same_function_p (int event_idx_a,
			    int event_idx_b) const
{
  return (m_events[event_idx_a]->get_logical_location ()
	  == m_events[event_idx_b]->get_logical_location ());
}

thread_id_t
test_path::add_thread (const char *name)
{
  m_threads.safe_push (new test_thread (name));
  return m_threads.length () - 1;
}

/* Add an event to this path at LOC within function FNDECL at
   stack depth DEPTH.

   Use m_context's printer to format FMT, as the text of the new
   event.

   Return the id of the new event.  */

event_id_t
test_path::add_event (location_t loc,
		      const char *funcname,
		      int depth,
		      const char *fmt, ...)
{
  pretty_printer *pp = m_event_pp;
  pp_clear_output_area (pp);

  rich_location rich_loc (line_table, UNKNOWN_LOCATION);

  va_list ap;

  va_start (ap, fmt);

  /* No localization is done on FMT.  */
  text_info ti (fmt, &ap, 0, nullptr, &rich_loc);
  pp_format (pp, &ti);
  pp_output_formatted_text (pp);

  va_end (ap);

  test_event *new_event
    = new test_event (loc,
		      logical_location_from_funcname (funcname),
		      depth,
		      pp_formatted_text (pp));
  m_events.safe_push (new_event);

  pp_clear_output_area (pp);

  return event_id_t (m_events.length () - 1);
}

event_id_t
test_path::add_thread_event (thread_id_t thread_id,
			     location_t loc,
			     const char *funcname,
			     int depth,
			     const char *fmt, ...)
{
  pretty_printer *pp = m_event_pp;
  pp_clear_output_area (pp);

  rich_location rich_loc (line_table, UNKNOWN_LOCATION);

  va_list ap;

  va_start (ap, fmt);

  /* No localization is done on FMT.  */
  text_info ti (fmt, &ap, 0, nullptr, &rich_loc);

  pp_format (pp, &ti);
  pp_output_formatted_text (pp);

  va_end (ap);

  test_event *new_event
    = new test_event (loc,
		      logical_location_from_funcname (funcname),
		      depth,
		      pp_formatted_text (pp),
		      thread_id);
  m_events.safe_push (new_event);

  pp_clear_output_area (pp);

  return event_id_t (m_events.length () - 1);
}

/* Mark the most recent event on this path (which must exist) as being
   connected to the next one to be added.  */

void
test_path::connect_to_next_event ()
{
  gcc_assert (m_events.length () > 0);
  m_events[m_events.length () - 1]->connect_to_next_event ();
}

void
test_path::add_entry (const char *callee_name,
		      int stack_depth,
		      thread_id_t thread_id)
{
  add_thread_event (thread_id, UNKNOWN_LOCATION, callee_name, stack_depth,
		    "entering %qs", callee_name);
}

void
test_path::add_return (const char *caller_name,
		       int stack_depth,
		       thread_id_t thread_id)
{
  add_thread_event (thread_id, UNKNOWN_LOCATION, caller_name, stack_depth,
		    "returning to %qs", caller_name);
}

void
test_path::add_call (const char *caller_name,
		     int caller_stack_depth,
		     const char *callee_name,
		     thread_id_t thread_id)
{
  add_thread_event (thread_id, UNKNOWN_LOCATION,
		    caller_name, caller_stack_depth,
		    "calling %qs", callee_name);
  add_entry (callee_name, caller_stack_depth + 1, thread_id);
}

logical_locations::key
test_path::logical_location_from_funcname (const char *funcname)
{
  return m_test_logical_loc_mgr.logical_location_from_funcname (funcname);
}

/* struct test_event.  */

/* test_event's ctor.  */

test_event::
test_event (location_t loc,
	    logical_location logical_loc,
	    int depth,
	    const char *desc,
	    thread_id_t thread_id)
: m_loc (loc),
  m_logical_loc (logical_loc),
  m_depth (depth), m_desc (xstrdup (desc)),
  m_connected_to_next_event (false),
  m_thread_id (thread_id)
{
}

/* test_event's dtor.  */

test_event::~test_event ()
{
  free (m_desc);
}

} // namespace diagnostics::paths::selftest
} // namespace diagnostics::paths
} // namespace diagnostics

#endif /* #if CHECKING_P */
