/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_flat_flowgraph.h>
#include <gr_block_detail.h>
#include <gr_io_signature.h>
#include <gr_buffer.h>
#include <volk/volk.h>
#include <iostream>
#include <map>
#include <boost/format.hpp>

using namespace boost::assign;

#define GR_FLAT_FLOWGRAPH_DEBUG  0

// 32Kbyte buffer size between blocks
#define GR_FIXED_BUFFER_SIZE (32*(1L<<10))

static const unsigned int s_fixed_buffer_size = GR_FIXED_BUFFER_SIZE;

gr_flat_flowgraph_sptr
gr_make_flat_flowgraph()
{
  return gr_flat_flowgraph_sptr(new gr_flat_flowgraph());
}

gr_flat_flowgraph::gr_flat_flowgraph()
{
  number_of_blocks = 0;
  number_of_edges  = 0;
  this->alloc_policy = ALLOC_DEF;
  d_token_size     = GR_FIXED_BUFFER_SIZE;
}

gr_flat_flowgraph::~gr_flat_flowgraph()
{
}
void
gr_flat_flowgraph::set_blocks_io() {
  this->blocks_io.resize(this->blocks_top.size());
  for (int i=0; i < this->blocks_top.size(); i++) {
    // TODO: This assumes a single output port ... need to generalize
    // to multiple output ports
    this->blocks_io[i] = blocks_top[i]->output_signature()->sizeof_stream_item(0);
    //std::cout << this->blocks_top[i]->symbol_name() << " item_size= " << this->blocks_io[i] << std::endl;
  }
}
int
gr_flat_flowgraph::get_block_io(int index) {
  return this->blocks_io[index];
}
void
gr_flat_flowgraph::setup_connections()
{
  gr_basic_block_vector_t blocks = calc_used_blocks();

  // Assign block details to blocks
  for (gr_basic_block_viter_t p = blocks.begin(); p != blocks.end(); p++)
    cast_to_block_sptr(*p)->set_detail(allocate_block_detail(*p));

  // Connect inputs to outputs for each block
  for(gr_basic_block_viter_t p = blocks.begin(); p != blocks.end(); p++) {
    connect_block_inputs(*p);

    gr_block_sptr block = cast_to_block_sptr(*p);
    if (GR_FLAT_FLOWGRAPH_DEBUG) {
      std::cout << "2Relative Rate " << block->relative_rate() << std::endl;
      std::cout << "2Fixed Rate    " << block->fixed_rate() << std::endl;
    }
    block->set_unaligned(0);
    block->set_is_unaligned(false);
  }
  // Connect message ports connetions
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "Start Message Ports" << std::endl;
  for(gr_msg_edge_viter_t i = d_msg_edges.begin(); i != d_msg_edges.end(); i++){
    if(GR_FLAT_FLOWGRAPH_DEBUG)
        std::cout << boost::format("flat_fg connecting msg primitives: (%s, %s)->(%s, %s)\n") %
                    i->src().block() % i->src().port() %
                    i->dst().block() % i->dst().port();
    std::cout << "IN Message Ports" << std::endl;
    i->src().block()->message_port_sub( i->src().port(), pmt::pmt_cons(i->dst().block()->alias_pmt(), i->dst().port()) );
    }
  //////////////////////////////////////////////////////
  /// GET LIST OF BLOCKS USED IN TOPOLOGICAL ORDER  ////
  //////////////////////////////////////////////////////
  //gr_basic_block_vector_t used_blocks = this->calc_used_blocks();
  //used_blocks = this->topological_sort(used_blocks);
  //gr_block_vector_t blocks_temp = gr_flat_flowgraph::make_block_vector(used_blocks);
  // Ensure that the done flag is clear on all blocks
  //for (size_t i = 0; i < blocks_temp.size(); i++){
  //  std::cout << "HEY BLOCK= " << cast_to_block_sptr(blocks_temp[i]) << std::endl;
  //}
}
////////////////////////
// Fayez
void 
gr_flat_flowgraph::set_blocks_list() {
  std::string block;
  gr_edge_vector_t in_edges;
  int dst_port;
  int src_port;
  gr_basic_block_sptr src_block;
  gr_basic_block_sptr dst_block;
  gr_block_sptr src_grblock;
  gr_block_sptr dst_grblock;

  int src_id=-1;
  int dst_id=-1;
  double cur_rate_src = 1;
  double cur_rate_dst = 1;
  double block_rate_src = 1;
  double block_rate_dst = 1;

  int it1=0;
  int it2=0;

  //////////////////////////////////////////////////////
  /// GET LIST OF BLOCKS USED IN TOPOLOGICAL ORDER  ////
  //////////////////////////////////////////////////////
  gr_basic_block_vector_t used_blocks = this->calc_used_blocks();
  used_blocks = this->topological_sort(used_blocks);
  //this->blocks_list = gr_flat_flowgraph::make_block_vector(used_blocks);
  this->blocks_top = gr_flat_flowgraph::make_block_vector(used_blocks);
  
  if (GR_FLAT_FLOWGRAPH_DEBUG)
    std::cout << "Before filling boost vector" << std::endl;
  // set vector size to the number of blocks
  this->blocks_list.resize(this->blocks_top.size());
  this->blocks_firing.resize(this->blocks_top.size());
  this->blocks_noutput.resize(this->blocks_top.size());
  this->blocks_noutput_var.resize(this->blocks_top.size());
  this->blocks_nproduced.resize(this->blocks_top.size());
  this->blocks_nproduced_var.resize(this->blocks_top.size());
  this->blocks_output_buff.resize(this->blocks_top.size());
  this->blocks_output_buff_var.resize(this->blocks_top.size());
  this->blocks_work_time.resize(this->blocks_top.size());
  this->blocks_work_time_var.resize(this->blocks_top.size());


  for (size_t i = 0; i < this->blocks_top.size(); i++){
    //this->blocks_list[i] = blocks_temp[i]->symbol_name();
    //this->blocks_list.resize(i);
    this->blocks_list[i] = this->blocks_top[i]->symbol_name();
    in_edges = calc_upstream_edges(this->blocks_top[i]);
    for (gr_edge_viter_t e = in_edges.begin(); e != in_edges.end(); e++) {
      this->number_of_edges = this->number_of_edges + 1;
    }
  }
  this->number_of_blocks = this->blocks_list.size();
  this->top_matrix.resize(this->number_of_edges, this->number_of_blocks);
  this->chan_list.resize(this->number_of_edges, 2);
  // Initiailize the newly created Matrix row to 0's
  for (int j=0; j < this->number_of_edges; j++)
    for (int k=0; k < this->number_of_blocks; k++) {
      this->top_matrix(j,k) = 0;
      this->chan_list(j,0)  = "None";
      this->chan_list(j,1)  = "None";
    }
  //std::cout << "Empty Matrix= ";
  //std::cout << this->top_matrix << std::endl;
  it1=0;
  for (size_t i = 0; i < this->blocks_top.size(); i++){
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "HEY BLOCK= " << this->blocks_top[i]->name() << " Rel Rate= " << this->blocks_top[i]->relative_rate() << std::endl;    
    in_edges = calc_upstream_edges(this->blocks_top[i]);
    for (gr_edge_viter_t e = in_edges.begin(); e != in_edges.end(); e++) {
      // Set the buffer reader on the destination port to the output
      // buffer on the source port
      dst_port = e->dst().port();
      src_port = e->src().port();
      src_block = e->src().block();
      dst_block = e->dst().block();
      src_grblock = cast_to_block_sptr(src_block);
      dst_grblock = cast_to_block_sptr(dst_block);
      src_id = this->return_block_id(src_block->symbol_name());
      dst_id = this->return_block_id(dst_block->symbol_name());
      if (GR_FLAT_FLOWGRAPH_DEBUG)
	std::cout << "number of edges= " << this->number_of_edges << " number of blocks= " << this->number_of_blocks << std::endl;


      block_rate_src = src_grblock->relative_rate();
      block_rate_dst = dst_grblock->relative_rate();
      // I disagree with the fact that packed_to_unpacked rate=8
      // seeing that it's converting data formats
      //if (src_grblock->name() == "packed_to_unpacked_bb")
      //cur_rate_src = 1;
      //if (src_grblock->name() == "packed_to_unpacked_bb")
      //cur_rate_dst = 1;
      // framer sink is a general special case of a sink decimator
      // block so its rate change should propagate to the previous block
      //if (dst_grblock->name() == "framer_sink_1") {
      //if (block_rate_dst < block_rate_src) {
      //block_rate_src = block_rate_dst;
      // }
      //else {
	if ((dst_grblock->name() != "vector_to_stream")&&(dst_grblock->name() != "stream_to_vector"))
	  block_rate_dst = block_rate_src;
	else
	  std::cout << "VECTOR STREAM BLOCK" << std::endl;
	//}
      //this->top_matrix(it1,dst_id) = -block_rate_dst;

      this->top_matrix(it1,src_id) = block_rate_src;
      this->top_matrix(it1,dst_id) = -block_rate_dst;
      this->chan_list(it1,0) = src_block->symbol_name();
      this->chan_list(it1,1) = dst_block->symbol_name();
      //this->top_matrix(this->number_of_edges,src_id) = i;
      //this->top_matrix(this->number_of_edges,dst_id) = i+1;
      if (GR_FLAT_FLOWGRAPH_DEBUG) {
	std::cout << "M[" << it1 << "," << src_id << "]= " << this->top_matrix(it1,src_id) << " " << src_grblock->name() << std::endl;
	std::cout << "M[" << it1 << "," << dst_id << "]= " << this->top_matrix(it1,dst_id) << " " << dst_grblock->name() << std::endl;
      }
      //std::cout << "Matrix dimension= " << this->top_matrix.size1() << "x" << this->top_matrix.size2() << std::endl;
      //std::cout << "Matrix= " << std::endl << this->top_matrix << std::endl;
      it1++;
    }
  }
  //this->print_chan_list();
}
void
gr_flat_flowgraph::print_chan_list() {
  for (int i=0; i < this->number_of_edges; i++) {
    std::cout << "Src= " << this->chan_list(i,0) << " Dst= " << this->chan_list(i,1) << std::endl;
  }
}

void
gr_flat_flowgraph::print_top_matrix() {
  //std::cout << "Gnuradio Topology Matrix= " << std::endl << top_matrix << std::endl;
  std::cout << "Gnuradio Topology Matrix= " << std::endl;
  for (size_t i=0; i < this->number_of_edges; i++) {
    for (size_t j=0; j < this->number_of_blocks; j++) {
      std::cout << top_matrix(i,j) << "  ";
      }
    std::cout << std::endl;
  }
}
void
gr_flat_flowgraph::print_blocks_firing() {
  std::cout << "Gnuradio Firing= " << std::endl;
  for (size_t i = 0; i < this->blocks_firing.size(); i++){
    std::cout << this->blocks_firing[i] << std::endl;
  }
}
int gr_flat_flowgraph::return_number_of_blocks() {
  return this->number_of_blocks;
}
int gr_flat_flowgraph::return_number_of_edges() {
  return this->number_of_edges;
}
int gr_flat_flowgraph::return_block_id(std::string block_find) {
  for (size_t i = 0; i < this->blocks_list.size(); i++){
    if (block_find == this->blocks_list[i])
      return i;
  }
  return -1;
}
int gr_flat_flowgraph::return_chan_id(std::string block_out, std::string block_in) {
  for (size_t i = 0; i < this->number_of_edges; i++){
    if (block_out == this->chan_list(i,0))
      return i;
  }
  return -1;
}
void 
gr_flat_flowgraph::set_top_matrix(int index1, int index2, double value) {
  //////////////////////////////////////////////////////
  /// SET THE CONTENTS OF THE TOPOLOGY MATRIX       ////
  //////////////////////////////////////////////////////
  top_matrix(index1, index2) = value;
}
void
gr_flat_flowgraph::set_blocks_firing(int index, int value) {
  this->blocks_firing[index] = value;
}
std::string gr_flat_flowgraph::return_blocks_list(int index) {
  return blocks_list[index];
}
double
gr_flat_flowgraph::return_top_matrix(int index1, int index2) {
  return top_matrix(index1, index2);
}
void
gr_flat_flowgraph::setup_token_size(int token_size) {
  d_token_size = token_size;
}
int
gr_flat_flowgraph::return_token_size() {
  return d_token_size;
}
gr_block_detail_sptr
gr_flat_flowgraph::allocate_block_detail(gr_basic_block_sptr block)
{
  int ninputs = calc_used_ports(block, true).size();
  int noutputs = calc_used_ports(block, false).size();
  gr_block_detail_sptr detail = gr_make_block_detail(ninputs, noutputs);

  gr_block_sptr grblock = cast_to_block_sptr(block);
  if(!grblock)
    throw std::runtime_error("allocate_block_detail found non-gr_block");

  if (GR_FLAT_FLOWGRAPH_DEBUG) {
    std::cout << "Creating block detail for " << block << std::endl;
    std::cout << "Relative Rate " << grblock->relative_rate() << std::endl;
    std::cout << "Fixed Rate    " << grblock->fixed_rate() << std::endl;
  }

  for (int i = 0; i < noutputs; i++) {
    grblock->expand_minmax_buffer(i);

    gr_buffer_sptr buffer = allocate_buffer(block, i);
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "Allocated buffer for output " << block << ":" << i << std::endl;
    detail->set_output(i, buffer);

    // Update the block's max_output_buffer based on what was actually allocated.
    grblock->set_max_output_buffer(i, buffer->bufsize());
  }

  return detail;
}

gr_buffer_sptr
gr_flat_flowgraph::allocate_buffer(gr_basic_block_sptr block, int port)
{
  gr_block_sptr grblock = cast_to_block_sptr(block);
  if (!grblock)
    throw std::runtime_error("allocate_buffer found non-gr_block");
  int item_size = block->output_signature()->sizeof_stream_item(port);
  if (GR_FLAT_FLOWGRAPH_DEBUG) {
    std::cout << "BLOCK= " << grblock << std::endl;
    std::cout << "item_size= " << item_size << std::endl;
    std::cout << "unique_id = " << grblock->unique_id() << " symbolic_id=  " << grblock->symbolic_id() << " name= " << grblock->name() << " symbol_name= " << grblock->symbol_name() << " alias= " <<  grblock->alias() << std::endl;
  }
  // *2 because we're now only filling them 1/2 way in order to
  // increase the available parallelism when using the TPB scheduler.
  // (We're double buffering, where we used to single buffer)
  /////////////////////////////////////////////////////
  // FAYEZ - New buffer allocation policy
  int nitems = 0;
  int nitems_temp=0;
  int block_id = -1;
  int chan_id  = -1;
  int buff_size = 0;
  std::string block_name_temp="None";
  std::string temp_str="None";
  
  if (this->alloc_policy == ALLOC_DEF) {
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "ALLOC_DEF Policy" << std::endl;
    nitems = s_fixed_buffer_size * 2 / item_size;
    // Make sure there are at least twice the output_multiple no. of items
    if (nitems < 2*grblock->output_multiple())	// Note: this means output_multiple()
      nitems = 2*grblock->output_multiple();	// can't be changed by block dynamically
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "nitem2= " << nitems << " output_relrate= " << grblock->relative_rate() << " max_noutput= " << grblock->max_noutput_items() << std::endl;
  }
  else if (this->alloc_policy == ALLOC_TOP){
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "ALLOC_TOP Policy" << std::endl;

    block_id = this->return_block_id(grblock->symbol_name());
    chan_id  = this->return_chan_id(grblock->symbol_name(), temp_str);
    if (0) {
      std::cout << "Block name = " << grblock->symbol_name();
      std::cout << " id= "         << block_id;
      std::cout << " chan_id= "    << chan_id;
      std::cout << " top_entry= " << this->top_matrix(chan_id, block_id);
      std::cout << " firing entry= " << this->blocks_firing[block_id];
      std::cout << " token_size= " << this->d_token_size;
      std::cout << std::endl;
    }
    // if we're allocating a uhd buffer allocate the full 32KB to avoid libusb error
    buff_size = this->top_matrix(chan_id, block_id)*this->blocks_firing[block_id]*this->d_token_size;
    block_name_temp = grblock->symbol_name();
    if(!this->is_uhd_channel(block_name_temp))
    	nitems = buff_size;
    else {
	nitems_temp = ceil(log(buff_size)/log(2));
    	//nitems = s_fixed_buffer_size*2/item_size;
        nitems = pow(2,nitems_temp)/item_size;
	std::cout << "UHD BUFFER TEMP=" << nitems_temp << " NAME= " << block_name_temp << " SIZE= " << nitems << std::endl;
	}
    if (0) {
    std::cout << "nitems= " << nitems << " nitems_temp= " << nitems_temp << std::endl;
    }
  }
  else
    std::cout << "UNKNOWN_ALLOC= " << this->alloc_policy << std::endl;
  if (GR_FLAT_FLOWGRAPH_DEBUG) {
    std::cout << "s_fixed_buffer_size= " << s_fixed_buffer_size << std::endl;
    std::cout << "nitem1= " << nitems << " item_size= " << item_size << std::endl;
  }
  // If any downstream blocks are decimators and/or have a large output_multiple,
  // ensure we have a buffer at least twice their decimation factor*output_multiple
  gr_basic_block_vector_t blocks = calc_downstream_blocks(block, port);

  //////////////////////////////////////////////////////////////////////////////////////////
  // limit buffer size if indicated
  if(grblock->max_output_buffer(port) > 0) {
    //    std::cout << "constraining output items to " << block->max_output_buffer(port) << "\n";
    nitems = std::min((long)nitems, (long)grblock->max_output_buffer(port));
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "restric max nitem1= " << nitems << std::endl;
    nitems -= nitems%grblock->output_multiple();
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "restric max nitem2= " << nitems << std::endl;
    if( nitems < 1 )
      throw std::runtime_error("problems allocating a buffer with the given max output buffer constraint!");
  }
  else if(grblock->min_output_buffer(port) > 0) {
    nitems = std::max((long)nitems, (long)grblock->min_output_buffer(port));
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "restrict min nitem1= " << nitems << std::endl;
    nitems -= nitems%grblock->output_multiple();
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "restric nitem2= " << nitems << std::endl;
    if( nitems < 1 )
      throw std::runtime_error("problems allocating a buffer with the given min output buffer constraint!");
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  /* TODO: unnecessary if we have topology matrix since we already the
  decimation and interpolation rates for each channel

  Not really, the topology matrix contains the relative rates for the
  hierarchical components but not the hierarchical composition of an actor
  */
  //////////////////////////////////////////////////////////////////////////////////////////  
  if (this->alloc_policy == ALLOC_DEF) {
    for (gr_basic_block_viter_t p = blocks.begin(); p != blocks.end(); p++) {
      gr_block_sptr dgrblock = cast_to_block_sptr(*p);
      if (!dgrblock)
	throw std::runtime_error("allocate_buffer found non-gr_block");

      double decimation = (1.0/dgrblock->relative_rate());
      int multiple      = dgrblock->output_multiple();
      int history       = dgrblock->history();
      //std::cout << "dgrblock= " << dgrblock << " decimation " <<
      //decimation << " multiple=" << multiple << " history= " << history << " max_noutput= " << dgrblock->detail()->output(0)->d_bufsize << std::endl;
      nitems = std::max(nitems, static_cast<int>(2*(decimation*multiple+history)));
      if (GR_FLAT_FLOWGRAPH_DEBUG)      
	std::cout << "final nitem= " << nitems << std::endl;
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////
//  std::cout << "gr_make_buffer(" << nitems << ", " << item_size << ", " << grblock << "\n";
  return gr_make_buffer(nitems, item_size, grblock);
}
bool
gr_flat_flowgraph::is_uhd_channel(std::string block_name) {
    int chan_id=-1;
    std::string temp_str = "None";
    if(block_name.find("uhd") != std::string::npos)
    	return 1;
    else {
	chan_id = this->return_chan_id(block_name, temp_str);
	if (this->chan_list(chan_id, 1).find("uhd") != std::string::npos)
		return 1;
    }
    return 0;
}

void
gr_flat_flowgraph::connect_block_inputs(gr_basic_block_sptr block)
{
  gr_block_sptr grblock = cast_to_block_sptr(block);
  if (!grblock)
    throw std::runtime_error("connect_block_inputs found non-gr_block");

  // Get its detail and edges that feed into it
  gr_block_detail_sptr detail = grblock->detail();
  gr_edge_vector_t in_edges = calc_upstream_edges(block);

  // For each edge that feeds into it
  for (gr_edge_viter_t e = in_edges.begin(); e != in_edges.end(); e++) {
    // Set the buffer reader on the destination port to the output
    // buffer on the source port
    int dst_port = e->dst().port();
    int src_port = e->src().port();
    gr_basic_block_sptr src_block = e->src().block();
    gr_basic_block_sptr dst_block = e->dst().block();
    gr_block_sptr src_grblock = cast_to_block_sptr(src_block);
    gr_block_sptr dst_grblock = cast_to_block_sptr(dst_block);
    
    if (!src_grblock)
      throw std::runtime_error("connect_block_inputs found non-gr_block");
    gr_buffer_sptr src_buffer = src_grblock->detail()->output(src_port);

    if (GR_FLAT_FLOWGRAPH_DEBUG) {
      std::cout << "Setting input " << dst_port << " from edge " << (*e) << std::endl;
      std::cout << "Src Block= " << src_grblock << " Dst Block= " << dst_grblock << std::endl;
    }
    //this->number_of_edges = this->number_of_edges + 1;
    //this->top_matrix.resize(this->number_of_edges, this->number_of_blocks);
    //this->set_top_matrix();
    detail->set_input(dst_port, gr_buffer_add_reader(src_buffer, grblock->history()-1, grblock));
  }
}

void
gr_flat_flowgraph::merge_connections(gr_flat_flowgraph_sptr old_ffg)
{
  // Allocate block details if needed.  Only new blocks that aren't pruned out
  // by flattening will need one; existing blocks still in the new flowgraph will
  // already have one.
  for (gr_basic_block_viter_t p = d_blocks.begin(); p != d_blocks.end(); p++) {
    gr_block_sptr block = cast_to_block_sptr(*p);

    if (!block->detail()) {
      if (GR_FLAT_FLOWGRAPH_DEBUG)
        std::cout << "merge: allocating new detail for block " << (*p) << std::endl;
        block->set_detail(allocate_block_detail(block));
    }
    else
      if (GR_FLAT_FLOWGRAPH_DEBUG)
        std::cout << "merge: reusing original detail for block " << (*p) << std::endl;
  }

  // Calculate the old edges that will be going away, and clear the buffer readers
  // on the RHS.
  for (gr_edge_viter_t old_edge = old_ffg->d_edges.begin(); old_edge != old_ffg->d_edges.end(); old_edge++) {
    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "merge: testing old edge " << (*old_edge) << "...";

    gr_edge_viter_t new_edge;
    for (new_edge = d_edges.begin(); new_edge != d_edges.end(); new_edge++)
      if (new_edge->src() == old_edge->src() &&
	  new_edge->dst() == old_edge->dst())
	break;

    if (new_edge == d_edges.end()) { // not found in new edge list
      if (GR_FLAT_FLOWGRAPH_DEBUG)
	std::cout << "not in new edge list" << std::endl;
      // zero the buffer reader on RHS of old edge
      gr_block_sptr block(cast_to_block_sptr(old_edge->dst().block()));
      int port = old_edge->dst().port();
      block->detail()->set_input(port, gr_buffer_reader_sptr());
    }
    else {
      if (GR_FLAT_FLOWGRAPH_DEBUG)
	std::cout << "found in new edge list" << std::endl;
    }
  }

  // Now connect inputs to outputs, reusing old buffer readers if they exist
  for (gr_basic_block_viter_t p = d_blocks.begin(); p != d_blocks.end(); p++) {
    gr_block_sptr block = cast_to_block_sptr(*p);

    if (GR_FLAT_FLOWGRAPH_DEBUG)
      std::cout << "merge: merging " << (*p) << "...";

    if (old_ffg->has_block_p(*p)) {
      // Block exists in old flow graph
      if (GR_FLAT_FLOWGRAPH_DEBUG)
	std::cout << "used in old flow graph" << std::endl;
      gr_block_detail_sptr detail = block->detail();

      // Iterate through the inputs and see what needs to be done
      int ninputs = calc_used_ports(block, true).size(); // Might be different now
      for (int i = 0; i < ninputs; i++) {
	if (GR_FLAT_FLOWGRAPH_DEBUG)
	  std::cout << "Checking input " << block << ":" << i << "...";
	gr_edge edge = calc_upstream_edge(*p, i);

	// Fish out old buffer reader and see if it matches correct buffer from edge list
	gr_block_sptr src_block = cast_to_block_sptr(edge.src().block());
	gr_block_detail_sptr src_detail = src_block->detail();
	gr_buffer_sptr src_buffer = src_detail->output(edge.src().port());
	gr_buffer_reader_sptr old_reader;
	if (i < detail->ninputs()) // Don't exceed what the original detail has
	  old_reader = detail->input(i);

	// If there's a match, use it
	if (old_reader && (src_buffer == old_reader->buffer())) {
	  if (GR_FLAT_FLOWGRAPH_DEBUG)
	    std::cout << "matched, reusing" << std::endl;
	}
	else {
	  if (GR_FLAT_FLOWGRAPH_DEBUG)
	    std::cout << "needs a new reader" << std::endl;

	  // Create new buffer reader and assign
	  detail->set_input(i, gr_buffer_add_reader(src_buffer, block->history()-1, block));
	}
      }
    }
    else {
      // Block is new, it just needs buffer readers at this point
      if (GR_FLAT_FLOWGRAPH_DEBUG)
	std::cout << "new block" << std::endl;
      connect_block_inputs(block);

      // Make sure all buffers are aligned
      setup_buffer_alignment(block);
    }

    // Now deal with the fact that the block details might have changed numbers of
    // inputs and outputs vs. in the old flowgraph.
  }
}

void
gr_flat_flowgraph::setup_buffer_alignment(gr_block_sptr block)
{
  const int alignment = volk_get_alignment();
  std::cout << "in buffer alignment" << std::endl;
  for(int i = 0; i < block->detail()->ninputs(); i++) {
    void *r = (void*)block->detail()->input(i)->read_pointer();
    unsigned long int ri = (unsigned long int)r % alignment;
    //std::cerr << "reader: " << r << "  alignment: " << ri << std::endl;
    if(ri != 0) {
      size_t itemsize = block->detail()->input(i)->get_sizeof_item();
      block->detail()->input(i)->update_read_pointer(alignment-ri/itemsize);
    }
    block->set_unaligned(0);
    block->set_is_unaligned(false);
  }

  for(int i = 0; i < block->detail()->noutputs(); i++) {
    void *w = (void*)block->detail()->output(i)->write_pointer();
    unsigned long int wi = (unsigned long int)w % alignment;
    //std::cerr << "writer: " << w << "  alignment: " << wi << std::endl;
    if(wi != 0) {
      size_t itemsize = block->detail()->output(i)->get_sizeof_item();
      block->detail()->output(i)->update_write_pointer(alignment-wi/itemsize);
    }
    block->set_unaligned(0);
    block->set_is_unaligned(false);
  }
}

void gr_flat_flowgraph::dump()
{
  for (gr_edge_viter_t e = d_edges.begin(); e != d_edges.end(); e++)
     std::cout << " edge: " << (*e) << std::endl;

  for (gr_basic_block_viter_t p = d_blocks.begin(); p != d_blocks.end(); p++) {
    std::cout << " block: " << (*p) << std::endl;
    gr_block_detail_sptr detail = cast_to_block_sptr(*p)->detail();
    std::cout << "  detail @" << detail << ":" << std::endl;

    int ni = detail->ninputs();
    int no = detail->noutputs();
    for (int i = 0; i < no; i++) {
      gr_buffer_sptr buffer = detail->output(i);
      std::cout << "   output " << i << ": " << buffer << std::endl;
    }

    for (int i = 0; i < ni; i++) {
      gr_buffer_reader_sptr reader = detail->input(i);
      std::cout << "   reader " <<  i << ": " << reader
                << " reading from buffer=" << reader->buffer() << std::endl;
    }
  }

}

gr_block_vector_t
gr_flat_flowgraph::make_block_vector(gr_basic_block_vector_t &blocks)
{
  gr_block_vector_t result;
  for (gr_basic_block_viter_t p = blocks.begin(); p != blocks.end(); p++) {
    result.push_back(cast_to_block_sptr(*p));
  }

  return result;
}


void gr_flat_flowgraph::clear_endpoint(const gr_msg_endpoint &e, bool is_src){
    for(size_t i=0; i<d_msg_edges.size(); i++){
        if(is_src){
            if(d_msg_edges[i].src() == e){
                d_msg_edges.erase(d_msg_edges.begin() + i);
                i--;
            }
        } else {
            if(d_msg_edges[i].dst() == e){
                d_msg_edges.erase(d_msg_edges.begin() + i);
                i--;
            }
        }
    }
}

void gr_flat_flowgraph::replace_endpoint(const gr_msg_endpoint &e, const gr_msg_endpoint &r, bool is_src){
    size_t n_replr(0);
    if(GR_FLAT_FLOWGRAPH_DEBUG)
        std::cout << boost::format("gr_flat_flowgraph::replace_endpoint( %s, %s, %d )\n") % e.block()% r.block()% is_src;
    for(size_t i=0; i<d_msg_edges.size(); i++){
        if(is_src){
            if(d_msg_edges[i].src() == e){
                if(GR_FLAT_FLOWGRAPH_DEBUG)
                    std::cout << boost::format("gr_flat_flowgraph::replace_endpoint() flattening to ( %s, %s )\n") % r.block()% d_msg_edges[i].dst().block();
                d_msg_edges.push_back( gr_msg_edge(r, d_msg_edges[i].dst() ) );
                n_replr++;
            }
        } else {
            if(d_msg_edges[i].dst() == e){
                if(GR_FLAT_FLOWGRAPH_DEBUG)
                    std::cout << boost::format("gr_flat_flowgraph::replace_endpoint() flattening to ( %s, %s )\n") % r.block()% d_msg_edges[i].dst().block();
                d_msg_edges.push_back( gr_msg_edge(d_msg_edges[i].src(), r ) );
                n_replr++;
            }
        }
    }
}

