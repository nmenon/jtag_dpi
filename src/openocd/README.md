# Basic openocd configuration

If you are hand building openocd, then build:
	./configure --enable-remote-bitbang

# Adapter configuration
Bare minimum adapter configuration is provided:
http://openocd.org/doc/html/Debug-Adapter-Configuration.html

See provided [testing_remote_bitbang.tcl](testing_remote_bitbang.tcl)

# Startup
	openocd -f testing_remote_bitbang.tcl

# signalling:
When built with TRACE=1, the captured waveform looks like this:

![OpenOcd wave](openocd_connect_wave.png "gtkwave of openocd connect")

