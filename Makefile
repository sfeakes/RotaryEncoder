# define the C compiler to use
CC = gcc

ADD_CURL := 1

#LOG := -D GPIO_LOG_ENABLED -D LOGGING_ENABLED
LOG := -D LOGGING_ENABLED
#LOG := 
#LOG := -D GPIO_ERR_LOG_DISABELED

#SYSFS := -D GPIO_SYSFS_MODE     # Use file system for everything
#SYSFS := -D GPIO_SYSFS_INTERRUPT # Use filesystem for interrupts
SYSFS :=

# debug of not
#DBG = -g -O0 -fsanitize=address -static-libasan
#DBG = -g
#DBG = -D GPIO_DEBUG
DBG =

ifeq ($(ADD_CURL),)
  LIBS := -lm -lpthread
else
  LIBS := -lm -lpthread -lcurl
  DAAPD = -D WITH_DAAPD
endif

# define any compile-time flags
GCCFLAGS = -Wall -O3

MONGOOSE_CFLAGS = -D MG_ENABLE_MQTT=1 -D MG_ENABLE_MQTT_BROKER=0 -D MG_ENABLE_HTTP=0 -D MG_ENABLE_SSL=0 -D MG_ENABLE_HTTP_WEBSOCKET0 -D MG_ENABLE_DNS_SERVER=0 -D MG_ENABLE_COAP=0 -D MG_ENABLE_BROADCAST=0 -D MG_ENABLE_GETADDRINFO=0 -D MG_ENABLE_THREADS=0 -D MG_DISABLE_HTTP_DIGEST_AUTH -D CS_DISABLE_MD5

# NSF Below is for HTTP, remove if don;t get working
#MONGOOSE_CFLAGS = -D MG_DISABLE_MD5 -D MG_DISABLE_HTTP_DIGEST_AUTH -D MG_DISABLE_MD5 -D MG_DISABLE_JSON_RPC -D INI_READONLY

# define any compile-time flags
CFLAGS = -Wall -O3 $(LOG) $(DBG) $(LIBS) $(MONGOOSE_CFLAGS) $(DAAPD)
#CFLAGS = -Wall -O3 $(LOG) $(DBG) $(LIBS)

# add it all together
#CFLAGS = $(GCCFLAGS) $(LIBS)

# define the C source files
SRCS = RotaryEncoder.c GPIO_Pi.c MQTT.c mongoose.c log.c

OBJS = $(SRCS:.c=.o)

# define the executable file
MAIN = ./release/rotaryencoder
#CURL = ./release/rotaryencodercurl

#curl: $(CURL)

all:    $(MAIN) 
  @echo: $(MAIN) have been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS) 

install: $(MAIN)
	./release/install.sh

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN) $(MAIN_U)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

#################################################################################
# GPIO Compile options

GPIO_CFLAGS = -Wall -O3 -lm -lpthread
GPIO_SRC = GPIO_Pi.c
GPIO_OUT_DIR = ./gpio_tools
#GPIO_LOG := -D GPIO_LOG_ENABLED
#GPIO_LOG := -D GPIO_ERR_LOG_DISABELED
#GPIO_SYSFS := -D GPIO_SYSFS_MODE      # Use file system for everything
#GPIO_SYSFS := -D GPIO_SYSFS_INTERRUPT # Use filesystem for interrupts
GPIO_SYSFS :=
GPIO_LOG :=


GPIO_MON = $(GPIO_OUT_DIR)/gpio_monitor
GPIO = $(GPIO_OUT_DIR)/gpio

#gpio_tools: gpio_tool gpio_monitor
gpio: gpio_tool gpio_monitor
gpio_tool: $(GPIO)
gpio_monitor: $(GPIO_MON)

$(GPIO): $(GPIO_SRC) | $(GPIO_OUT_DIR)
	$(CC) -o $(GPIO) $(GPIO_SRC) $(GPIO_CFLAGS) -D GPIO_TOOL $(GPIO_LOG) $(GPIO_SYSFS)

$(GPIO_MON): $(GPIO_SRC) | $(GPIO_OUT_DIR)
	$(CC) -o $(GPIO_MON) $(GPIO_SRC) $(GPIO_CFLAGS) -D GPIO_MONITOR $(GPIO_LOG) $(GPIO_SYSFS)
	
$(GPIO_OUT_DIR):
	@mkdir -p $(GPIO_OUT_DIR)

gpio_clean:
	$(RM) $(GPIO) $(GPIO_MON)