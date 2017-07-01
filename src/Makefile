
OUT=output

DPI_MODULES=
VERILATOR_FLAGS:=--cc -sv --exe -Wall -Wno-fatal -O3  --assert --Mdir $(OUT)
ifneq ("$(TRACE)","")
VERILATOR_FLAGS+=--trace
endif
VERILATOR_EXE ?= verilator

PROJECT_NAME :=jtag_dpi_remote_bit_bang

VERILATOR_FILES:= \
		  dpi/$(PROJECT_NAME).sv\
		  dpi/$(PROJECT_NAME).c\
		  verilator_tb/jtag_dpi_tb.cpp

OUTPUT_BIN:= $(PROJECT_NAME)

all: clean client_make
	$(MAKE) -C . build_v
	$(MAKE) -C . build_c
	@echo "make run to test"
ifneq ("$(TRACE)","")
	@echo "make view to see results"
endif

build_v:
	$(VERILATOR_EXE) $(VERILATOR_FLAGS) $(VERILATOR_FILES) -o $(OUTPUT_BIN)

build_c:
	make -j -C $(OUT)/ -f V$(PROJECT_NAME).mk

run: all
	($(OUT)/$(OUTPUT_BIN) &)
	sleep 5
	client/test

ifneq ("$(TRACE)","")
view: run
	gtkwave trace.vcd trace.sav
endif

clean:
	rm -rf $(OUT) *.sav *.vcd

client_make: client_clean
	$(MAKE) -C client

client_clean:
	$(MAKE) -C client clean
