BINDIR = bin/
SRCDIR = src/
INCLUDE_PATH =	-I core \
		-I algorithms \
		-I objs

# Compiler
CC = g++

# Compiler flags
ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --glibs)
CFLAGS = -fPIC -w -g -W ${ROOTCFLAGS} -O3 #-std=c++0x
LFLAGS = ${ROOTLIBS} -g -lGenVector -lMinuit -O3 -lMinuit2

# Automatically decide what to compile
CORE = $(notdir $(wildcard core/*.C))
OBJS = $(CORE:.C=.o)
OBJS := $(addprefix ${BINDIR}, ${OBJS})

ALGORITHMS = $(notdir $(wildcard algorithms/*.C))
ALGOBJS = $(ALGORITHMS:.C=.ao)
ALGOBJS := $(addprefix ${BINDIR}, ${ALGOBJS})

DATAOBJS = $(wildcard ${PWD}/objs/*.h) 

DATACLASS = $(notdir $(wildcard ${PWD}/objs/*.C))
DATACLASSOBJS = $(DATACLASS:.C=.do)
DATACLASSOBJS := $(addprefix ${BINDIR}, ${DATACLASSOBJS})

EXE = ${BINDIR}tbAnalysis

# Compile core, user algorithms and make an executable for each user algorithm
all :				${OBJS} ${ALGOBJS} ${DATACLASSOBJS} ${EXE}
				@echo "Done"

# Compile core algorithms (everything .C)
${BINDIR}%.o :			core/%.C core/%.h
				@echo "Compiling $(notdir $<)"
				@$(CC) $(CFLAGS) ${INCLUDE_PATH} -c $< -o $@

${BINDIR}%.ao :			algorithms/%.C algorithms/%.h
				@echo "Compiling $(notdir $<)"
				@$(CC) $(CFLAGS) ${INCLUDE_PATH} -c $< -o $@

${BINDIR}%.do :			objs/%.C objs/%.h
				@echo "Compiling $(notdir $<)"
				@$(CC) $(CFLAGS) ${INCLUDE_PATH} -c $< -o $@

${BINDIR}Steering.o :		core/Steering.C
				@echo "Compiling steering file"
				@$(CC) $(CFLAGS) ${INCLUDE_PATH} -c core/Steering.C -o ${BINDIR}Steering.o

${BINDIR}EventDict.o :		${DATAOBJS}
				@echo "Making event dictionary"
				@rm -f core/EventDict.C core/EventDict.h
				@rootcint core/EventDict.C -c ${DATAOBJS} 
				@$(CC) $(CFLAGS) ${INCLUDE_PATH} -c core/EventDict.C -o ${BINDIR}EventDict.o

${EXE} :			${OBJS} ${DATACLASSOBJS} ${ALGOBJS} ${BINDIR}EventDict.o
				@echo "Making executable tbAnalysis"
				@$(CC) -o ${BINDIR}tbAnalysis $(OBJS) ${DATACLASSOBJS} ${ALGOBJS} $(LFLAGS) 

# Remove all executables and object files
clean:
				@rm -f ${BINDIR}*
				@echo "Cleaned"


