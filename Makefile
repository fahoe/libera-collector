src=/home/Data/Projects/XBPM/Libera/progs/fa-collector/
project=fa_collect

#CFLAGS = -g -Wall -DDEBUG -pthread
CFLAGS = -Wall -pthread
DFLAGS = -g -Wall
TARGETS = $(project) 


all: $(project) $(test)

$(project): $(project).c
	gcc $< $(CFLAGS) -o $@

clean:
	rm $(TARGETS)
