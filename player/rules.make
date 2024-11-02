# Common build rules.
#
# Don't invoke this file directly. It is meant to be included in other files.

BINARIES=$(BIN)analyzer $(BIN)player

COMMON_HDRS=$(SRC)analysis.h $(SRC)logging.h $(SRC)first-move.h $(SRC)first-move-table.h $(SRC)options.h $(SRC)random.h $(SRC)state.h
COMMON_SRCS=$(SRC)analysis.cc $(SRC)first-move.cc $(SRC)first-move-table.cc $(SRC)options.h $(SRC)random.cc $(SRC)state.cc
COMMON_OBJS=$(OBJ)analysis.o $(OBJ)first-move.o $(OBJ)first-move-table.o $(OBJ)options.o $(OBJ)random.o $(OBJ)state.o
ANALYZER_OBJS=$(OBJ)analyzer.o $(COMMON_OBJS)
PLAYER_OBJS=$(OBJ)player.o $(COMMON_OBJS)

# Note that headers must be included in dependency order.
COMBINED_SRCS=\
	$(SRC)options.h $(SRC)options.cc \
	$(SRC)random.h $(SRC)random.cc \
	$(SRC)state.h $(SRC)state.cc \
	$(SRC)logging.h \
  $(SRC)analysis.h $(SRC)analysis.cc \
  $(SRC)first-move.h $(SRC)first-move.cc \
  $(SRC)first-move-table.h $(SRC)first-move-table.cc \
	$(SRC)player.cc

all: $(BINARIES)

$(OBJ)analysis.o: $(SRC)analysis.cc $(SRC)analysis.h $(SRC)state.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)first-move.o: $(SRC)first-move.cc $(SRC)first-move.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)first-move-table.o: $(SRC)first-move-table.cc $(SRC)first-move-table.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)options.o: $(SRC)options.cc $(SRC)options.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)random.o: $(SRC)random.cc $(SRC)random.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)state.o: $(SRC)state.cc $(SRC)state.h $(SRC)random.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)analyzer.o: $(SRC)analyzer.cc $(COMMON_HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)player.o: $(SRC)player.cc $(COMMON_HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN)analyzer: $(ANALYZER_OBJS)
	$(CXX) $(CXXFLAGS) $(ANALYZER_OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN)player: $(PLAYER_OBJS)
	$(CXX) $(CXXFLAGS) $(PLAYER_OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OUT)combined-player.cc: $(COMBINED_SRCS) combine-sources.sh
	./combine-sources.sh $(COMBINED_SRCS) > $@

$(BIN)combined-player: $(OUT)combined-player.cc
	$(CXX) $(CXXFLAGS) -ULOCAL_BUILD -o $@ $<  $(LDFLAGS) $(LDLIBS)

analyzer: $(BIN)analyzer

player: $(BIN)player

combined: $(BIN)combined-player

clean:
	rm -f $(BINARIES) $(OBJ)*.o $(OUT)combined-player.cc $(BIN)combined-player

.DELETE_ON_ERROR:

.PHONY: all clean analyzer player combined
