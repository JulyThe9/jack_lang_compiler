# === Paths ===
SRC_DIR := src
INC_DIR := inc
BUILD_DIR := build
MISC_DIR := misc
SAMPLES_DIR := samples
ROOT_DIR := .

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
EXEC := $(BUILD_DIR)/JackCompiler

# === Compiler ===
CXX := g++
CXXFLAGS := -I$(INC_DIR) -std=c++17

# === Targets ===

.PHONY: all clean clean_all run plot_ast

all: build_create $(EXEC)
	@rm -f $(BUILD_DIR)/*.o

build_create:
	@mkdir -p $(BUILD_DIR)

# Debug target
debug: CXXFLAGS += -g -Wall
debug: all

# Compile each object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link the final executable
$(EXEC): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJ_FILES) -o $(EXEC)

clean:
	rm -rf $(BUILD_DIR)

clean_all: clean
	@echo "Deleting files..."

	@find $(ROOT_DIR) $(SRC_DIR) $(INC_DIR) -maxdepth 1 -type f ! -name "Makefile" ! -name ".*" ! -name "*.cpp" ! -name "*.h" \
		! -name "README.md" -exec echo Deleting: {} \; -exec rm {} \;

	@find $(SAMPLES_DIR) -maxdepth 1 -type f ! -name "Makefile" ! -name "*.jack" ! -name "*.vm" \
		-exec echo Deleting: {} \; -exec rm {} \;

	@echo "Deletion complete"

list_clean_all:
	@find $(ROOT_DIR) $(SRC_DIR) $(INC_DIR) -maxdepth 1 -type f ! -name "Makefile" ! -name ".*" ! -name "*.cpp" ! -name "*.h" \
		! -name "README.md" -exec echo To be deleted: {} \;

	@find $(SAMPLES_DIR) -maxdepth 1 -type f ! -name "Makefile" ! -name "*.jack" ! -name "*.vm" \
		-exec echo To be deleted: {} \;

# Usage: make run src=<sources_path> libs=<libs_path> (optional)
run:
	@if [ -n "$src" ]; then \
		$(EXEC) "$(SAMPLES_DIR)/$$src" "$$libs"; \
	else \
		echo "Please provide src=<source_path>"; \
	fi

	mv *.vm $(SAMPLES_DIR)/
	

# Usage: make plot_ast file=<filename> (optional)
plot_ast:
	@file=$${file:-ast_nodes}; \
	python3 $(MISC_DIR)/drawAST.py $(BUILD_DIR)/$$file $(BUILD_DIR)/ast_tree.png
