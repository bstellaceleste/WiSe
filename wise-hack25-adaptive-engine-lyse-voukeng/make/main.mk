# --- CONFIGURATION ---
# PIN_ROOT doit pointer vers votre installation d'Intel Pin.
# Modifiez cette ligne si nécessaire.
PIN_ROOT ?= /path/to/your/pin/directory

# FICHIERS SOURCES
TOOL_SRC = ../src/adaptive_engine.cpp
TOOL_OBJ = $(TOOL_SRC:.cpp=.o)
TOOL_SO = obj-intel64/adaptive_engine.so

# COMPILATION
# Flags pour le compilateur C++
CXXFLAGS = -I$(PIN_ROOT)/source/include/pin -I$(PIN_ROOT)/source/include/pin/gen -I$(PIN_ROOT)/extras/xed-intel64/include/xed -O3 -std=c++17 -Wall -Werror -fPIC
# Flags pour l'éditeur de liens
LDFLAGS = -L$(PIN_ROOT)/intel64/lib -L$(PIN_ROOT)/intel64/lib-ext -lpin -lxed

# RÈGLES
all: $(TOOL_SO)

$(TOOL_SO): $(TOOL_OBJ)
	@mkdir -p obj-intel64
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)
	@echo "==> Outil Pin créé : $@"

$(TOOL_OBJ): $(TOOL_SRC)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf obj-intel64 *.o

.PHONY: all clean
