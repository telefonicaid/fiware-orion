EXEC          = ssClient
DFLAGS        = -DANSI
CFLAGS        = -g -Wall -fPIC -Wno-unused-function $(DFLAGS) $(INCLUDE)
SOURCES       = ssClient.c
OBJS          = $(SOURCES:c=o)
CC            = g++

$(EXEC):		$(OBJS)
						$(CC) -o $(EXEC) $(OBJS)

%.o: %.cpp
						$(CC) $(CFLAGS) -c $^ -o $@
