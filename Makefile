# $Id$
# 
# Makefile

#--------------------------------------------------
# Variables
#--------------------------------------------------
UNAME		:= $(shell uname -s)
ARCH		:= $(shell uname -m)

TARGET		= cmd_parser

OBJDIR 		= obj

# Executables
CC		= gcc
LD		= gcc
AR		= ar
RANLIB		= ranlib
CP		= /bin/cp -fp
RM		= /bin/rm -f

# C or assembly sources which are archived in the library
C_SRCS		= 
C_SRCS		+= getch.c
C_SRCS		+= msg_queue.c
C_SRCS		+= cmd_lexan.c
C_SRCS		+= channel_db.c
C_SRCS		+= cmd_operation.c
C_SRCS		+= cmd_parser.c
C_SRCS		+= io_handler.c
C_SRCS		+= main.c
A_SRCS		= 
SRCS		= $(C_SRCS) $(A_SRCS)

# Ojbect files
C_OBJS		= $(C_SRCS:%.c=$(OBJDIR)/%.o)
A_OBJS		= $(A_SRCS:%.S=$(OBJDIR)/%.o)
OBJS		= $(C_OBJS) $(A_OBJS)

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
INC_PATH	+= -I/usr/local/include
INC_PATH	+= -I.

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
 LD_LIBS	+= -lpthread
#LD_LIBS	+= -lm

#--------------------------------------------------
# Rules
#--------------------------------------------------

all: obj tags $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LD_FLAGS) -o $@ $(OBJS) $(LD_LIBS)
#	$(AR) cru $@ $(OBJS)
#	$(RANLIB) $@

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

tags: *.h ${SRCS}
	@echo Updating tag table...
	@ctags ${SRCS} *.h

$(OBJDIR):
	mkdir $@

#--------------------------------------------------
# Make Dependency List
#--------------------------------------------------
MAKE_DEPEND	= makedepend $(DEFS) $(INC_PATH) $(SRCS)

.depend: $(OBJDIR) Makefile
	@$(MAKE_DEPEND) -f- -p$(OBJDIR)/ > $@  

sinclude .depend

