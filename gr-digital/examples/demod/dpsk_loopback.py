#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: DPSK Loopback
# Author: GNU Radio
# Description: Encode a signal into a packet, modulate, demodulate, decode and show it's the same data.
# Generated: Sun Feb 24 13:33:20 2013
##################################################

from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.gr import firdes
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from grc_gnuradio import blks2 as grc_blks2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import wx

class dpsk_loopback(grc_wxgui.top_block_gui):

	def __init__(self):
		grc_wxgui.top_block_gui.__init__(self, title="DPSK Loopback")
		_icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
		self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

		##################################################
		# Variables
		##################################################
		self.samp_rate = samp_rate = 10000
		self.freq = freq = 500

		##################################################
		# Blocks
		##################################################
		_freq_sizer = wx.BoxSizer(wx.VERTICAL)
		self._freq_text_box = forms.text_box(
			parent=self.GetWin(),
			sizer=_freq_sizer,
			value=self.freq,
			callback=self.set_freq,
			label="Frequency (Hz)",
			converter=forms.float_converter(),
			proportion=0,
		)
		self._freq_slider = forms.slider(
			parent=self.GetWin(),
			sizer=_freq_sizer,
			value=self.freq,
			callback=self.set_freq,
			minimum=0,
			maximum=samp_rate/2,
			num_steps=100,
			style=wx.SL_HORIZONTAL,
			cast=float,
			proportion=1,
		)
		self.Add(_freq_sizer)
		self.wxgui_scopesink2_0 = scopesink2.scope_sink_f(
			self.GetWin(),
			title="Scope Plot",
			sample_rate=samp_rate,
			v_scale=0,
			v_offset=0,
			t_scale=1./freq,
			ac_couple=False,
			xy_mode=False,
			num_inputs=1,
			trig_mode=gr.gr_TRIG_MODE_AUTO,
			y_axis_label="Counts",
		)
		self.Add(self.wxgui_scopesink2_0.win)
		self.gr_throttle_0_0 = gr.throttle(gr.sizeof_float*1, samp_rate)
		self.gr_sig_source_x_0 = gr.sig_source_f(samp_rate, gr.GR_COS_WAVE, freq, 1, 0)
		self.digital_dxpsk_mod_1 = digital.dbpsk_mod(
			samples_per_symbol=2,
			excess_bw=0.35,
			gray_coded=True,
			verbose=False,
			log=False)
			
		self.digital_dxpsk_demod_1 = digital.dbpsk_demod(
			samples_per_symbol=2,
			excess_bw=0.35,
			freq_bw=6.28/100.0,
			phase_bw=6.28/100.0,
			timing_bw=6.28/100.0,
			gray_coded=True,
			verbose=False,
			log=False
		)
		self.blks2_packet_encoder_0 = grc_blks2.packet_mod_f(grc_blks2.packet_encoder(
				samples_per_symbol=2,
				bits_per_symbol=1,
				access_code="",
				pad_for_usrp=True,
			),
			payload_length=0,
		)
		self.blks2_packet_decoder_0 = grc_blks2.packet_demod_f(grc_blks2.packet_decoder(
				access_code="",
				threshold=-1,
				callback=lambda ok, payload: self.blks2_packet_decoder_0.recv_pkt(ok, payload),
			),
		)

		##################################################
		# Connections
		##################################################
		self.connect((self.blks2_packet_decoder_0, 0), (self.wxgui_scopesink2_0, 0))
		self.connect((self.blks2_packet_encoder_0, 0), (self.digital_dxpsk_mod_1, 0))
		self.connect((self.digital_dxpsk_mod_1, 0), (self.digital_dxpsk_demod_1, 0))
		self.connect((self.digital_dxpsk_demod_1, 0), (self.blks2_packet_decoder_0, 0))
		self.connect((self.gr_throttle_0_0, 0), (self.blks2_packet_encoder_0, 0))
		self.connect((self.gr_sig_source_x_0, 0), (self.gr_throttle_0_0, 0))


	def get_samp_rate(self):
		return self.samp_rate

	def set_samp_rate(self, samp_rate):
		self.samp_rate = samp_rate
		self.gr_sig_source_x_0.set_sampling_freq(self.samp_rate)
		self.gr_throttle_0_0.set_sample_rate(self.samp_rate)
		self.wxgui_scopesink2_0.set_sample_rate(self.samp_rate)

	def get_freq(self):
		return self.freq

	def set_freq(self, freq):
		self.freq = freq
		self._freq_slider.set_value(self.freq)
		self._freq_text_box.set_value(self.freq)
		self.gr_sig_source_x_0.set_frequency(self.freq)

if __name__ == '__main__':
	parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
	(options, args) = parser.parse_args()
	tb = dpsk_loopback()
	tb.Run(True)

