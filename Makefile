src=/home/Data/Projects/XBPM/Libera/progs/fa-collector/
project=fa_collect
test=test_server

#CFLAGS = -g -Wall -DDEBUG -pthread
CFLAGS = -Wall -pthread
DFLAGS = -g -Wall
TARGETS = $(project) $(test)


all: $(project) $(test)

$(project): $(project).c
	gcc $< $(CFLAGS) -o $@

$(test): $(test).c
	gcc $< $(DFLAGS) -o $@

clean:
	rm $(TARGETS)
