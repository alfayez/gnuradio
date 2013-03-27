/* -*- c++ -*- */
/*
 * Copyright 2007,2008,2010 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

class gr_top_block;
typedef boost::shared_ptr<gr_top_block> gr_top_block_sptr;
%template(gr_top_block_sptr) boost::shared_ptr<gr_top_block>;

// Hack to have a Python shim implementation of gr.top_block
// that instantiates one of these and passes through calls
%rename(top_block_swig) gr_make_top_block;
gr_top_block_sptr gr_make_top_block(const std::string name)
  throw (std::logic_error);

class gr_top_block : public gr_hier_block2
{
private:
  gr_top_block(const std::string &name);

public:
  ~gr_top_block();

  void start(int max_noutput_items=100000) throw (std::runtime_error);
  void stop();
  //void wait();
  //void run() throw (std::runtime_error);
  void lock();
  void unlock() throw (std::runtime_error);
  void dump();

  int max_noutput_items();
  void set_max_noutput_items(int nmax);

  gr_top_block_sptr to_top_block(); // Needed for Python type coercion

  // Get the list of processes
  std::string blocks_list_top(int index);
  // Get the topology matrix
  double top_matrix_top(int index1, int index2);
  int top_get_number_of_edges();
  int top_get_number_of_blocks();
  void set_blocks_firing(int index, int value);
  void set_top_matrix(int index1, int index2, double value);
  void print_top_matrix();
  void print_blocks_firing();
  void prealloc(int max_noutput_items=100000);
  void alloc(int token_size, int alloc_policy=0, int max_noutput_items=100000);
  double get_performance_measure(int index, int measure);
  void go();

};

#ifdef SWIGPYTHON

%inline %{
void top_block_run_unlocked(gr_top_block_sptr r) throw (std::runtime_error)
{
    Py_BEGIN_ALLOW_THREADS;		// release global interpreter lock
    r->run();
    Py_END_ALLOW_THREADS;		// acquire global interpreter lock
}

void top_block_wait_unlocked(gr_top_block_sptr r) throw (std::runtime_error)
{
    Py_BEGIN_ALLOW_THREADS;		// release global interpreter lock
    r->wait();
    Py_END_ALLOW_THREADS;		// acquire global interpreter lock
}
%}

#endif
