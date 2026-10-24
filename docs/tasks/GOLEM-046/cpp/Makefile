NAME=agnb_cpp
LDHAS_ARG_PROCESSING=0
LDHAS_EXIT_HANDLER=0
include $(shell agondev-config --makefile)
CXXFLAGS += -std=c++17 -I../../../../../agon-utils/examples/agnb/api -I../../../../runtime/agondev
OBJS += obj/golem_agnb_host.o
obj/golem_agnb_host.o: ../../../../runtime/agondev/agnb_host.cpp
	@mkdir -p obj
	$(CC) $(CXXFLAGS) -c $< -o $@
$(LINKBINARY): obj/golem_agnb_host.o
