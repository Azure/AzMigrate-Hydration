# -*- makefile -*-
#----------------------------------------------------------------------------
#       GNU ACE Workspace
#
# 
#
# This file was generated by MPC.  Any changes made directly to
# this file will be lost the next time it is generated.
#
# MPC Command:
# /home/johnny/DOC_ROOT/stage-4724/ACE_wrappers/bin/mwc.pl -type gnuace -exclude TAO/TAO_ACE.mwc -workers 8 -recurse -hierarchy -relative ACE_ROOT=/home/johnny/DOC_ROOT/stage-4724/ACE_wrappers -relative TAO_ROOT=/home/johnny/DOC_ROOT/stage-4724/ACE_wrappers/TAO
#
#----------------------------------------------------------------------------

MAKEFILE = GNUmakefile

ifeq ($(findstring k,$(MAKEFLAGS)),k)
  KEEP_GOING = -
endif

include $(ACE_ROOT)/include/makeinclude/macros.GNU

all: atm_sap_client atm_sap_server

depend: atm_sap_client-depend atm_sap_server-depend

REMAINING_TARGETS := $(filter-out all depend,$(TARGETS_NESTED:.nested=)) $(CUSTOM_TARGETS)
.PHONY: $(REMAINING_TARGETS)

$(REMAINING_TARGETS):
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_client $(@)
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_server $(@)

.PHONY: atm_sap_client
atm_sap_client:
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_client all

.PHONY: atm_sap_client-depend
atm_sap_client-depend:
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_client depend

.PHONY: atm_sap_server
atm_sap_server:
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_server all

.PHONY: atm_sap_server-depend
atm_sap_server-depend:
	$(KEEP_GOING)@$(MAKE) -f GNUmakefile.atm_sap_server depend

project_name_list:
	@echo atm_sap_client
	@echo atm_sap_server
