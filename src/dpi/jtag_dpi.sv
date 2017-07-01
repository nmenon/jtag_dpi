//////////////////////////////////////////////////////////////////////
////                                                              ////
////  jtag_dpi.sv, former dbg_comm_vpi.v                          ////
////                                                              ////
////                                                              ////
////  This file is part of the SoC/OpenRISC Development Interface ////
////  http://www.opencores.org/cores/DebugInterface/              ////
////                                                              ////
////                                                              ////
////  Author(s):                                                  ////
////       Igor Mohor (igorm@opencores.org)                       ////
////       Gyorgy Jeney (nog@sdf.lonestar.net)                    ////
////       Nathan Yawn (nathan.yawn@opencores.org)                ////
////       Andreas Traber (atraber@iis.ee.ethz.ch)                ////
////                                                              ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2000-2015 Authors                              ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////

module jtag_dpi
#(
  parameter TCP_PORT      = 9000 // XXX: we must get rid of static assignment
)
(
  input     clk_i,
  input     enable_i,

  output    jtag_tms_o,
  output    jtag_tck_o,
  output    jtag_trst_o,
  output    jtag_tdi_o,
  input     jtag_tdo_i
  );

  import "DPI-C" function int jtag_server_init(input int port);
  import "DPI-C" function int jtag_server_tick(output bit s_tms,
                                               output bit s_tck,
                                               output bit s_trst,
                                               output bit s_tdi,
                                               output bit s_new_data,
                                               input  bit i_tdo);
  reg       rx_jtag_tms;
  reg       rx_jtag_tck;
  reg       rx_jtag_trst;
  reg       rx_jtag_tdi;
  reg       rx_jtag_new_data_available;

  // Handle commands from the upper level
  initial
    begin
    if (0 != jtag_server_init(TCP_PORT))
    begin
      $display("Error initiazing port");
      $finish;
    end
    $display("port=%d", TCP_PORT);

	end

	// On clk input, check if we are enabled first
  always @(posedge clk_i)
  begin
		if (enable_i)
		begin
      if ( 0 != jtag_server_tick(rx_jtag_tms, rx_jtag_tck,
                                 rx_jtag_trst, rx_jtag_tdi,
                                 rx_jtag_new_data_available,
                                 jtag_tdo_i) )
		  begin
        $display("Error receiving from the JTAG DPI module.");
        $finish;
		  end

		  if (rx_jtag_new_data_available)
			begin
        $display( "Rx: TCK: %0d, TMS: %0d, TDI: %0d, TRST: %0d.",
				          rx_jtag_tck, rx_jtag_tms, rx_jtag_tdi, rx_jtag_trst );

	     jtag_tms_o  <= rx_jtag_tms;
	     jtag_tck_o  <= rx_jtag_tck;
	     jtag_trst_o <= rx_jtag_trst;
	     jtag_tdi_o  <= rx_jtag_tdi;
      end //rx_jtag_new_data_available
    end //enable_i
  end //clk_i

endmodule
// vim: set ai ts=2 sw=2 et:
