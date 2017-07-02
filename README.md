# Original Source:
* https://github.com/pulp-platform/jtag_dpi
* https://github.com/rdiez/jtag_dpi/


# JTAG DPI Module for OpenOCD remote bitbang itnerface

This module emulates a JTAG port for a remote debug bridge following the OpenOCD
Remote bitbang definition: http://repo.or.cz/openocd.git/blob/HEAD:/doc/manual/jtag/drivers/remote_bitbang.txt

This has been developed using verilator[https://www.veripool.org/projects/verilator/wiki/Installing]
which does have support for DPI (https://en.wikipedia.org/wiki/SystemVerilog_DPI)
which has many benefits over the VPI (https://en.wikipedia.org/wiki/Verilog_Procedural_Interface)

# Tools needed:
* gcc, make, the usual build essentials
* new version of verilator -> if sudo apt-get install verilator version does'nt work, then
install off https://www.veripool.org/projects/verilator/wiki/Installing
 NOTE: on my Debian Jessie machine Verilator 3.864 2014-09-21 rev verilator_3_862-18-ge8edbad
  did not work, however a rebuild worked just fine.
  Verilator 3.906 2017-06-22 rev verilator_3_906
* if you are interested in viewing waveforms, sudo apt-get install gtkwave

# Build and test:
  See [OpenOCD testing](src/openocd/README.md) and for standalone: under src/client, is a simple
  test application that I use as standalone verification (useful for waveform analysis)

  * make -C src TRACE=1 view
  Should build the verilog, test bench, test client and start them up, and open gtkwave

  * make -C src run
  Should build the verilog, test bench and run (no view) -> you should get a spam on console.

# Why this?
  Why not? And was looking to understand openOCD better and wont be bothered to hook
  my sigork (it involves getting off my bed).
# Location of sources and a bit of design:
  ```

.
├── README.md
└── src
    ├── client
    │   ├── Makefile
    │   └── test_connection.c -> test connection to the remote bitbang server
    ├── dpi
    │   ├── jtag_dpi_remote_bit_bang.c (this is the network server)
    │   └── jtag_dpi_remote_bit_bang.sv (this is the rtl servicing client reqs)
    ├── Makefile
    └── verilator_tb
        └── jtag_dpi_tb.cpp (test bench for tesing generating clock and trace)

4 directories, 7 files

Overall it looks something like this: both the simulation env and test are basically
applications running on your box
+----------------------------+
|                            |
|                            |
|                            |
|                  +------+  |      +-------+
|                  |      |  |      |       |
|       +-------+  |      |  |      |       |
|       |       |  | Serv |  |      |       |
|       |       |  | (c)  |  | port |       |
|       |       +-->      |  | <----+  network
|       | RTL   |  |      |  | 9000 |  client
|       | (verilog)+------+  |      |       |
|       |       |            |      |       |
|       |       |            |      |       |
|       |       |            |      |  test |
|       +---^---+            |      +-------+
|           |                |
|           |                |
|           |                |
|           +----+-------+   |
|                |       |   |
|                |Test bench |
|                +---c+--+   |
|                            |
|                            |
|  Simulation en^ironment    |
+----------------------------+


inside the implementation, I have tried to make ticks as non-blocking
as possible every clk cycle, server's ticker is invoked to check if
there is any data, if there is then only 1 byte is read and processed.
KISS principle avoids any unrelated simualtion entities being starved
off simulation cycles.. but it also means that traffic over jtag is
constrained.. should be very trivial to update that.. but that is not
my personal design goal.

Have fun..
 ```
