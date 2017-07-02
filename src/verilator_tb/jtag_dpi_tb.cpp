////   Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ ////
////   Nishanth Menon                                             ////

#include "Vjtag_dpi_remote_bit_bang.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define TRACE 0
#ifdef VM_TRACE
#undef TRACE
#define TRACE VM_TRACE
#endif

int main(int argc, char **argv, char **env)
{
	int i = 0;
	int clk;

	Verilated::commandArgs(argc, argv);

	// init top verilog instance
	Vjtag_dpi_remote_bit_bang *top = new Vjtag_dpi_remote_bit_bang;
	// init trace dump
	Verilated::traceEverOn(true);
#if TRACE
	VerilatedVcdC *tfp = new VerilatedVcdC;

	tfp->spTrace()->set_time_unit("1ns");
	tfp->spTrace()->set_time_resolution("1ps");

	top->trace(tfp, 99);
	tfp->open("trace.vcd");
#endif
	// initialize simulation inputs
	top->clk_i = 1;
	top->enable_i = 1;
	top->jtag_tdo_i = 0;
	for (;;) {
		i++;
		// dump variables into VCD file and toggle clock
		for (clk = 0; clk < 2; clk++) {
#if TRACE
			tfp->dump(2 * i + clk);
#endif
			top->clk_i = !top->clk_i;
			top->eval();
		}
		if (Verilated::gotFinish())
			break;
	}
#if TRACE
	tfp->close();
#endif
	exit(0);
}
