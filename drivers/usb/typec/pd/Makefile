#
# Makefile for USBC PD policy and System policy
#

pd_policy-y	:= devpolicy_mgr.o
pd_policy-y	+= message.o protocol.o
pd_policy-y	+= policy_engine.o
pd_policy-y	+= sink_port_pe.o
pd_policy-y	+= src_port_pe.o
pd_policy-y	+= display_pe.o

obj-$(CONFIG_USBC_PD_POLICY)		+= pd_policy.o
obj-$(CONFIG_USBC_SYSTEM_POLICY)	+= system_policy.o
