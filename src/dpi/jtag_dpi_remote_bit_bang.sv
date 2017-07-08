//////////////////////////////////////////////////////////////////////
////                                                              ////
////  jtag_dpi.sv, former dbg_comm_vpi.v                          ////
////                                                              ////
////   Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ ////
////   Nishanth Menon                                             ////
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

module jtag_dpi_remote_bit_bang
#(
  parameter TCP_PORT      = 9000 // XXX: we must get rid of static assignment
)
(
  input     clk_i,
  input     enable_i,

  output    jtag_tms_o,
  output    jtag_tck_o,
  output    jtag_trst_o,
  output    jtag_srst_o,
  output    jtag_tdi_o,
  input     jtag_tdo_i,

  output    blink_o
  );

  import "DPI-C" function int jtag_server_init(input int port);
  import "DPI-C" function void jtag_server_deinit();
  import "DPI-C" function int jtag_server_tick(output bit s_tms,
                                               output bit s_tck,
                                               output bit s_trst,
                                               output bit s_srst,
                                               output bit s_tdi,
                                               output bit s_blink,
                                               output bit s_new_b_data,
                                               output bit s_new_wr_data,
                                               output bit s_new_rst_data,
                                               output bit client_con,
                                               output bit send_tdo);
  import "DPI-C" function int jtag_server_send(input  bit i_tdo);

  logic     tdo_o;
  logic     tms_o;
  logic     tck_o;
  logic     trst_o;
  logic     srst_o;
  logic     tdi_o;
  logic     blink;
  logic     server_ready;

  wire      read_tdo;
  reg       rx_jtag_tms;
  reg       rx_jtag_tck;
  reg       rx_jtag_trst;
  reg       rx_jtag_srst;
  reg       rx_jtag_tdi;
  reg       rx_jtag_blink;
  reg       rx_jtag_new_b_data_available;
  reg       rx_jtag_new_wr_data_available;
  reg       rx_jtag_new_rst_data_available;
  /* verilator lint_off UNUSED */
  reg       rx_jtag_client_connection_status;
  /* verilator lint_off UNUSED */
  reg       tx_read_tdo;

  // Handle commands from the upper level
  initial
    begin

    rx_jtag_client_connection_status = 0;
    rx_jtag_new_b_data_available = 0;
    rx_jtag_new_wr_data_available = 0;
    rx_jtag_new_rst_data_available = 0;
    rx_jtag_tms = 0;
    rx_jtag_tck = 0;
    rx_jtag_trst = 0;
    rx_jtag_srst = 0;
    rx_jtag_tdi = 0;
    rx_jtag_blink = 0;
    tx_read_tdo = 0;
    tms_o = 0;
    tck_o = 0;
    trst_o = 0;
    srst_o = 0;
    tdi_o = 0;
    blink = 0;
    tdo_o = 0;
    server_ready = 0;
  end

  // On clk input, check if we are enabled first
  always @(posedge clk_i)
  begin
    if (enable_i && server_ready)
    begin
        if ( 0 != jtag_server_tick(rx_jtag_tms, rx_jtag_tck,
                                   rx_jtag_trst, rx_jtag_srst,
                                   rx_jtag_tdi, rx_jtag_blink,
                                   rx_jtag_new_b_data_available,
                                   rx_jtag_new_wr_data_available,
                                   rx_jtag_new_rst_data_available,
                                   rx_jtag_client_connection_status,
                                   tx_read_tdo) )
        begin
          $display("Error recieved from the JTAG DPI module.");
          $finish;
        end

        if (rx_jtag_new_b_data_available)
        begin
         blink  <= rx_jtag_blink;
        end //rx_jtag_new_b_data_available

        if (rx_jtag_new_wr_data_available)
        begin
         tms_o  <= rx_jtag_tms;
         tck_o  <= rx_jtag_tck;
         tdi_o  <= rx_jtag_tdi;
        end //rx_jtag_new_wr_data_available

        if (rx_jtag_new_rst_data_available)
        begin
          trst_o  <= rx_jtag_trst;
          srst_o  <= rx_jtag_srst;
        end //rx_jtag_new_rst_data_available

        if (tx_read_tdo)
        begin
          tdo_o = 1;
        end // tx_read_tdo

    end //enable_i && server_ready

    // Start the jtag server on assertion of enable
    if (enable_i && server_ready == 0)
    begin
      if (0 != jtag_server_init(TCP_PORT))
      begin
        $display("Error initiazing port");
        $finish;
      end // server init
      server_ready <= 1;
      $display("JTAG Bitbang port=%d: Open", TCP_PORT);
    end //enable_i && !server_ready

    // Stop the server on deassertion of enable
    if (server_ready && enable_i == 0)
    begin
      server_ready <= 0;
      jtag_server_deinit();
      $display("JTAG Bitbang port=%d: Closed", TCP_PORT);
    end //server_ready && !enable_i

  end // posedge clk_i

  always @(negedge clk_i)
  begin
    if (enable_i && server_ready && read_tdo)
      begin
        tdo_o = 0;
        if (0 != jtag_server_send(jtag_tdo_i))
        begin
          $display("Error recieved from the JTAG DPI module while Tx.");
          $finish;
        end //server_send
    end //enable_i && read_tdo
  end //neg edge clk_i

assign jtag_tms_o = tms_o;
assign jtag_tck_o = tck_o;
assign jtag_trst_o = trst_o;
assign jtag_srst_o = srst_o;
assign jtag_tdi_o = tdi_o;
assign blink_o = blink;
assign read_tdo = tdo_o;

endmodule
// vim: set ai ts=2 sw=2 et:
