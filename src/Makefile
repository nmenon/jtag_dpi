
OUT=output

DPI_MODULES=
VERILATOR_FLAGS:=--cc -sv --exe -Wall -Wno-fatal -O3 --assert --Mdir $(OUT)
ifneq ("$(TRACE)","")
VERILATOR_FLAGS+=--trace
endif
VERILATOR_EXE ?= verilator

PROJECT_NAME :=jtag_dpi

VERILATOR_FILES:= \
		  dpi/$(PROJECT_NAME).sv\
		  dpi/$(PROJECT_NAME).c\
		  verilator_tb/jtag_dpi_tb.cpp

OUTPUT_BIN:= $(PROJECT_NAME)

all: clean
	$(MAKE) -C . build_v
	$(MAKE) -C . build_c
	$(MAKE) -C . run
ifneq ("$(TRACE)","")
	$(MAKE) -C . view
endif


build_v:
	$(VERILATOR_EXE) $(VERILATOR_FLAGS) $(VERILATOR_FILES) -o $(OUTPUT_BIN)


build_c:
	make -j -C $(OUT)/ -f V$(PROJECT_NAME).mk

run:
	$(OUT)/$(OUTPUT_BIN)

view:
	gtkwave trace.vcd trace.sav

clean:
	rm -rf $(OUT) *.sav *.vcd

