CUDA_INSTALL_PATH := /opt/cuda

CXX      := g++
CXXFLAGS := -Wall -fopenmp -std=c++11 -O3

NVCC	 := nvcc
NVCCFLAGS:= -std=c++11

# CUDA library directory:
CUDA_LIB_DIR= -L$(CUDA_INSTALL_PATH)/lib64
# CUDA include directory:
CUDA_INC_DIR= -I$(CUDA_INSTALL_PATH)/include
# CUDA linking libraries:
CUDA_LINK_LIBS= -lcudart

SRC_DIR   = src
OBJ_DIR   = build
INCL_DIR  = include

#Windows
#REMOVEDIR := rmdir /S /Q $(OBJ_DIR)
#CREATEDIR := @mkdir
#NVCCFLAGS += –foreign

# Linux
REMOVEDIR := -@rm -rvf $(OBJ_DIR)/*
CREATEDIR := @mkdir -p

.SUFFIXES: .c .cpp .h .cu .cuh .o

INCLUDE  := -I$(INCL_DIR)/
TARGET   := rambo

CPP_SRC_FILES := $(subst $(SRC_DIR)/,,$(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.cpp)))
CUDA_SRC_FILES := $(subst $(SRC_DIR)/,,$(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.cu)))

HDR_FILES := $(wildcard $(INCL_DIR)/*.h)
CUH_FILES := $(wildcard $(INCL_DIR)/*.cuh)

CPP_OBJECTS := $(CPP_SRC_FILES:%.cpp=$(OBJ_DIR)/%.cpp.o)
CUDA_OBJECTS := $(CUDA_SRC_FILES:%.cu=$(OBJ_DIR)/%.cu.o)

ALL_OBJECTS := $(CPP_OBJECTS) $(CUDA_OBJECTS)

all: clean build $(OBJ_DIR)/$(TARGET)

$(OBJ_DIR)/$(TARGET): $(ALL_OBJECTS)
	@echo "Link everything together  ----------------------------------------------------"
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(ALL_OBJECTS) -o $(OBJ_DIR)/$(TARGET) $(CUDA_INC_DIR) $(CUDA_LIB_DIR) $(CUDA_LINK_LIBS)

$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@echo "Compile cpp files to objects -------------------------------------------------"
	$(NVCC) $(NVCCFLAGS) $(INCLUDE) $(CUDA_INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.cu.o: $(SRC_DIR)/%.cu
$(OBJ_DIR)/%.cu.o: $(SRC_DIR)/%.cu $(INCL_DIR)/%.cuh
	@echo "Compile cuda files to objects -------------------------------------------------"
	$(NVCC) $(NVCCFLAGS) $(INCLUDE) $(CUDA_INC_DIR) -c $< -o $@

# creates the OBJ_DIR folder
build:
	$(CREATEDIR) $(OBJ_DIR)

Debug: CXXFLAGS += -DDEBUG -g
Debug: all

Release: CXXFLAGS += -O2
Release: all

# removes the OBJ_DIR folder with all the content
clean:
	$(REMOVEDIR)
