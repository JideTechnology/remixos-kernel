#
# Makefile for USBC PD policy and System policy
#

pd_policy-y	:= devpolicy_mgr.o
pd_policy-y	+= message.o protocol.o

obj-$(CONFIG_USBC_PD_POLICY)		+= pd_policy.o
obj-$(CONFIG_USBC_SYSTEM_POLICY)	+= system_policy.o
