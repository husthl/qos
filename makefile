##############################################################################
#
# Total makefile to compile the project and
# then yield the three process:sysc bill etrade.
# etrade is one of the three methods:fsk sms brow
# which decided by the variable :B600_TYPE
# 2006.11.13 created by Li-jiakai
#
###############################################################################

CURDIR	:= $(shell pwd)

MODULES := test1 test2 test3 test4 test5 test6


all : 
	for module in $(MODULES); do make -w -C $$module clean all ; done
	
	
clean :  
	find . -name "*.d" -exec rm {} \;
	for module in $(MODULES); do make -w -C $$module clean; done
	
		 
distclean: 
	for module in $(MODULES); do make -w -C $$module distclean; done
	$(RM) $(DEPS) TAGS

	
ctags: 
	ctags -R .
 
