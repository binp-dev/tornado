TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
# Create and install (or just install) into <top>/db
# databases, templates, substitutions like this
#DB += xxx.db
DB += devPSC.db
DB += inp_waveform.db
DB += inp_waveform.substitutions
DB += out_waveform.db
DB += out_waveform.substitutions
DB += inp_register.db
DB += inp_register.substitutions
DB += out_register.db
DB += out_register.substitutions

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

