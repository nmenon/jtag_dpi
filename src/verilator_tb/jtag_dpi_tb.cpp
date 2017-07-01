#include "Vjtag_dpi.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define TRACE 0
#ifdef VM_TRACE
#undef TRACE
#define TRACE VM_TRACE
#endif

int main(int argc, char **argv, char **env)
{
	int i;
	int clk;

	Verilated::commandArgs(argc, argv);

	// init top verilog instance
	Vjtag_dpi *top = new Vjtag_dpi;
	// init trace dump
	Verilated::traceEverOn(true);
#if TRACE
	VerilatedVcdC *tfp = new VerilatedVcdC;
	top->trace(tfp, 99);
	tfp->open("trace.vcd");
#endif
	// initialize simulation inputs
	top->clk_i = 1;
	top->enable_i = 1;
	top->jtag_tms_o = 0;
	top->jtag_tck_o = 0;
	top->jtag_trst_o = 0;
	top->jtag_tdi_o = 0;
	top->jtag_tdo_i = 0;
	for (;;) {
		// dump variables into VCD file and toggle clock
		for (clk = 0; clk < 2; clk++) {
#if TRACE
			tfp->dump(2 * i + clk);
#endif
			top->clk_i = !top->clk_i;
			top->eval();
		}
		if (Verilated::gotFinish())
			exit(0);
	}
#if TRACE
	tfp->close();
#endif
	exit(0);
}
