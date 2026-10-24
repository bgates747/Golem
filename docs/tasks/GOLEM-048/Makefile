NAME=lines_review
LDHAS_ARG_PROCESSING=0
LDHAS_EXIT_HANDLER=0
include $(shell agondev-config --makefile)
CXXFLAGS += -std=c++17 -I../../../../agon-utils/examples/agnb/api -DREVIEW_BENCHMARK=$(or $(REVIEW_BENCHMARK),0) -DBENCH_END_LINES=$(or $(BENCH_END_LINES),1)
