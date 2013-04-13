/* -*- c++ -*- */
/*
 * Copyright 2006,2007 Free Software Foundation, Inc.
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

#ifndef INCLUDED_GR_FLAT_FLOWGRAPH_H
#define INCLUDED_GR_FLAT_FLOWGRAPH_H
#define ALLOC_DEF 0
#define ALLOC_TOP 1

#include <gr_core_api.h>
#include <gr_flowgraph.h>
#include <gr_block.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/assign/std/vector.hpp>
#include <string>
// Create a shared pointer to a heap allocated gr_flat_flowgraph
// (types defined in gr_runtime_types.h)
GR_CORE_API gr_flat_flowgraph_sptr gr_make_flat_flowgraph();

/*!
 *\brief Class specializing gr_flat_flowgraph that has all nodes
 * as gr_blocks, with no hierarchy
 * \ingroup internal
 */
class GR_CORE_API gr_flat_flowgraph : public gr_flowgraph
{
public:
  friend GR_CORE_API gr_flat_flowgraph_sptr gr_make_flat_flowgraph();

  // Destruct an arbitrary gr_flat_flowgraph
  ~gr_flat_flowgraph();

  // Wire gr_blocks together in new flat_flowgraph
  void setup_connections();
  
  // Merge applicable connections from existing flat flowgraph
  void merge_connections(gr_flat_flowgraph_sptr sfg);

  void dump();

  /*!
   * Make a vector of gr_block from a vector of gr_basic_block
   */
  static gr_block_vector_t make_block_vector(gr_basic_block_vector_t &blocks);

  void replace_endpoint(const gr_msg_endpoint &e, const gr_msg_endpoint &r, bool is_src);
  void clear_endpoint(const gr_msg_endpoint &e, bool is_src);

  /////////////////////
  // Al
  void set_blocks_list();
  void set_top_matrix(int index1, int index2, double value);
  void set_blocks_firing(int index, int value);
  //void set_blocks_firing();
  int return_block_id(std::string block_find);

  int return_chan_id(std::string, std::string);
  void print_chan_list();

  std::string return_blocks_list(int index);
  double return_top_matrix(int index1, int index2);
  int return_number_of_blocks();
  int return_number_of_edges();
  void setup_token_size(int token_size);
  int  return_token_size();
  void print_top_matrix();
  void print_blocks_firing();
  int alloc_policy;
  void set_blocks_io();
  int get_block_io(int index);
  // Fayez
  //std::vector<std::string> blocks_list;
  std::vector<std::string> blocks_list;
  std::vector<double> blocks_noutput;
  std::vector<double> blocks_noutput_var;
  std::vector<double> blocks_nproduced;
  std::vector<double> blocks_nproduced_var;
  //std::vector<double> blocks_input_buff;
  //std::vector<double> blocks_input_buff_var;
  std::vector<double> blocks_output_buff;
  std::vector<double> blocks_output_buff_var;
  std::vector<double> blocks_work_time;
  std::vector<double> blocks_work_time_var;
  std::vector<int> blocks_io;
  gr_block_vector_t blocks_top;
private:
  gr_flat_flowgraph();

  gr_block_detail_sptr allocate_block_detail(gr_basic_block_sptr block);
  gr_buffer_sptr allocate_buffer(gr_basic_block_sptr block, int port);
  void connect_block_inputs(gr_basic_block_sptr block);

  /* When reusing a flowgraph's blocks, this call makes sure all of the
   * buffer's are aligned at the machine's alignment boundary and tells
   * the blocks that they are aligned.
   *
   * Called from both setup_connections and merge_connections for
   * start and restarts.
   */
  void setup_buffer_alignment(gr_block_sptr block);

  std::vector<int> blocks_firing;
  int number_of_blocks;
  int number_of_edges;
  int d_token_size;
  boost::numeric::ublas::matrix<double> top_matrix;
  boost::numeric::ublas::matrix<std::string> chan_list;
  // List of blocks in topological order
};

#endif /* INCLUDED_GR_FLAT_FLOWGRAPH_H */
