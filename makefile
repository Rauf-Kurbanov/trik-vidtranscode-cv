
# your various installation directories
DEPOT_DSP?=/opt/trik-dsp

XDC_INSTALL_DIR?=$(DEPOT_DSP)/xdctools_3_24_07_73
CODEGEN_INSTALL_DIR?=$(DEPOT_DSP)/cgt_c6000_7.4.2
CE_INSTALL_DIR?=$(DEPOT_DSP)/codec_engine_3_23_00_07
XDAIS_INSTALL_DIR?=$(DEPOT_DSP)/xdais_7_23_00_06
DSPLIB_INSTALL_DIR?=$(DEPOT_DSP)/dsplib_3_1_1_1
IMGLIB_INSTALL_DIR?=$(DEPOT_DSP)/imglib_2_02_00_00

CGTOOLS_C674?=$(CODEGEN_INSTALL_DIR)

#uncomment this for verbose builds
#XDCOPTIONS=v

XDCARGS?=CGTOOLS_C674=\"$(CGTOOLS_C674)\"

XDCPATH?=$(CE_INSTALL_DIR)/packages;$(XDAIS_INSTALL_DIR)/packages;$(DSPLIB_INSTALL_DIR)/packages;$(IMGLIB_INSTALL_DIR)/packages

XDC?=$(XDC_INSTALL_DIR)/xdc

all:
	$(XDC) XDCOPTIONS=$(XDCOPTIONS) XDCARGS="$(XDCARGS)" --xdcpath="$(XDCPATH)" release

clean:
	$(XDC) XDCOPTIONS=$(XDCOPTIONS) clean

