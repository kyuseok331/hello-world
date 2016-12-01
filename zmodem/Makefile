# Makefile
# 

#--------------------------------------------------
# Variables
#--------------------------------------------------
UNAME		:= $(shell uname -s)
ARCH		:= $(shell uname -m)

TARGET_SZ	= sz
TARGET_RZ	= rz

OBJDIR 		= obj

# Executables
CC		= gcc
LD		= gcc
AR		= ar
RANLIB		= ranlib
CP		= /bin/cp -fp
RM		= /bin/rm -f

# C or assembly sources which are archived in the library
C_SRCS_SZ	= 
#C_SRCS_SZ	+= uart.c
C_SRCS_SZ	+= zm.c
C_SRCS_SZ	+= rbsb.c
C_SRCS_SZ	+= rzsz.c
C_SRCS_SZ	+= sz.c
C_SRCS_SZ	+= sz_main.c
A_SRCS_SZ	= 
SRCS_SZ		= $(C_SRCS_SZ) $(A_SRCS_SZ)

# Ojbect files
C_OBJS_SZ	= $(C_SRCS_SZ:%.c=$(OBJDIR)/%.o)
A_OBJS_SZ	= $(A_SRCS_SZ:%.S=$(OBJDIR)/%.o)
OBJS_SZ		= $(C_OBJS_SZ) $(A_OBJS_SZ)

# C or assembly sources which are archived in the library
C_SRCS_RZ	= 
#C_SRCS_RZ	+= uart.c
C_SRCS_RZ	+= zm.c
C_SRCS_RZ	+= rbsb.c
C_SRCS_RZ	+= rzsz.c
C_SRCS_RZ	+= rz.c
C_SRCS_RZ	+= rz_main.c
A_SRCS_RZ	= 
SRCS_RZ		= $(C_SRCS_RZ) $(A_SRCS_RZ)

# Ojbect files
C_OBJS_RZ	= $(C_SRCS_RZ:%.c=$(OBJDIR)/%.o)
A_OBJS_RZ	= $(A_SRCS_RZ:%.S=$(OBJDIR)/%.o)
OBJS_RZ		= $(C_OBJS_RZ) $(A_OBJS_RZ)

# C or assembly sources which are archived in the library
C_SRCS_ZM	= 
#C_SRCS_ZM	+= uart.c
C_SRCS_ZM	+= zm.c
C_SRCS_ZM	+= rbsb.c
C_SRCS_ZM	+= rzsz.c
C_SRCS_ZM	+= sz.c
C_SRCS_ZM	+= rz.c
A_SRCS_ZM	= 
SRCS_ZM		= $(C_SRCS_ZM) $(A_SRCS_ZM)

# Ojbect files
C_OBJS_ZM	= $(C_SRCS_ZM:%.c=$(OBJDIR)/%.o)
A_OBJS_ZM	= $(A_SRCS_ZM:%.S=$(OBJDIR)/%.o)
OBJS_ZM		= $(C_OBJS_ZM) $(A_OBJS_ZM)

# Compile flags
C_FLAGS		=
ifeq "$(ARCH)" "x86_64"
C_FLAGS		+= -m32
endif
C_FLAGS		+= -Wall -W -Wstrict-prototypes
# C_FLAGS	+= -Wmissing-prototypes
#C_FLAGS	+= -nostdlib
#C_FLAGS	+= -mhard-div
#C_FLAGS		+= -pg
C_FLAGS		+= -g

# Include paths
INC_PATH	=
INC_PATH	+= -I.
#INC_PATH	+= -I/usr/include/
#INC_PATH	+= -I/usr/include/linux

DEFS		= $(GLOBAL_DEFS)
#DEFS		+= -D_PRINT_MSG
#DEFS		+= -DEN_SC

COMPILE		= $(CC) $(C_FLAGS) $(DEFS) $(INC_PATH)

# Linking loader flags
LD_FLAGS	=
ifeq "$(ARCH)" "x86_64"
LD_FLAGS	+= -m32
endif

LD_PATH		= 
LD_PATH		+= -L/usr/local/lib

LD_LIBS		=
 LD_LIBS	+= -lwiringPi -lwiringPiDev
#LD_LIBS	+= -lpthread
#LD_LIBS	+= -lm

#--------------------------------------------------
# Rules
#--------------------------------------------------

all: obj tags $(TARGET_SZ) $(TARGET_RZ)

$(TARGET_SZ): $(OBJS_SZ)
	$(LD) $(LD_FLAGS) -o $@ $(OBJS_SZ) $(LD_LIBS)
#	$(AR) cru $@ $(OBJS_SZ)
#	$(RANLIB) $@

$(TARGET_RZ): $(OBJS_RZ)
	$(LD) $(LD_FLAGS) -o $@ $(OBJS_RZ) $(LD_LIBS)
#	$(AR) cru $@ $(OBJS_RZ)
#	$(RANLIB) $@

libzmodem.a: $(OBJS_ZM)
	$(AR) cru $@ $(OBJS_ZM)
	$(RANLIB) $@

clean:
	@echo ">>> Cleaning cmd_parser ..."
	$(RM) *.a .depend tags
	$(RM) $(OBJDIR)/*.o

clean_all:
	@echo ">>> Cleaning all cmd_parser ..."
	$(RM) *.a .depend tags
	$(RM) $(OBJDIR)/*.o

$(OBJDIR)/%.o: %.s
	$(COMPILE) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	$(COMPILE) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	$(COMPILE) -c -o $@ $<

tags: *.h ${SRCS_SZ} ${SRCS_RZ}
	@echo Updating tag table...
	@ctags ${SRCS_SZ} ${SRCS_RZ} *.h

$(OBJDIR):
	mkdir $@

#--------------------------------------------------
# Make Dependency List
#--------------------------------------------------
MAKE_DEPEND	= makedepend $(DEFS) $(INC_PATH) $(SRCS_SZ) $(SRCS_RZ)

.depend: $(OBJDIR) Makefile
	@$(MAKE_DEPEND) -f- -p$(OBJDIR)/ > $@  

sinclude .depend

