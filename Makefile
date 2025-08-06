
# Color codes
X := \033[0;39m
BLACK := \033[0;30m
DARK_RED := \033[0;31m
DARK_GREEN := \033[0;32m
DARK_YELLOW := \033[0;33m
DARK_BLUE := \033[0;34m
DARK_MAGENTA := \033[0;35m
DARK_CYAN := \033[0;36m
DARK_GRAY := \033[0;90m
LIGHT_GRAY := \033[0;37m
RED := \033[0;91m
GREEN := \033[0;92m
YELLOW := \033[0;93m
BLUE := \033[0;94m
MAGENTA := \033[0;95m
CYAN := \033[0;96m
WHITE := \033[0;97m

# Project variables
RM := rm -rf
NAME := webserv
CXX = c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 $(DEPFLAGS)
LDFLAGS = 

SRCS_FILES := main.cpp

SRCS_DIR := srcs/
OBJS_DIR := objs/
INCL_DIR := includes/
DEPS_DIR := deps/

SRCS := $(addprefix $(SRCS_DIR), $(SRCS_FILES))
OBJS := $(addprefix $(OBJS_DIR), $(SRCS_FILES:.cpp=.o))
DEPS := $(OBJS:.o=.d)

DEPFLAGS := -MMD -MP
SANITIZERS := -fsanitize=address -fsanitize=leak -fsanitize=undefined
INCL := -I $(INCL_DIR)

.PHONY: all clean fclean re debug rebug

# Main target
all: $(NAME)
	@echo "$(GREEN)Run the program with ./$(NAME) $(X)"


# Linking
$(NAME): $(OBJS)
	@$(CXX) $(OBJS) $(CXXFLAGS) -o $@ $(LDFLAGS)
	@echo "$(DARK_YELLOW)$@ compiled $(X)"

# Object compilation
$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp
	@mkdir -p $(OBJS_DIR)
	@mkdir -p $(DEPS_DIR)
	@echo "$(DARK_CYAN)Compiling $<... $(X)"
	@$(CXX) $(CXXFLAGS) $(INCL) -c $< -o $@
	@echo "$(BLUE)Compiled: $@$(X)"

-include $(DEPS)

# Clean object and dep files
clean:
	@if [ -d "$(OBJS_DIR)" ]; then \
	$(RM) $(OBJS_DIR); \
	echo "$(DARK_RED)Objects cleaned $(X)"; \
	else \
	echo "No object files to clean"; \
	fi
	
	@if [ -d "$(DEPS_DIR)" ]; then \
	$(RM) $(DEPS_DIR); \
	echo "$(DARK_RED)Dependencies cleaned $(X)"; else \
	echo "No dependency files to clean"; \
	fi

# Full clean
fclean: clean
	@if [ -f "$(NAME)" ]; then \
	$(RM) $(NAME); \
	echo "$(DARK_RED)$(NAME) removed $(X)"; \
	else \
	echo "No executables to clean"; \
	fi

# Rebuild
re: fclean all
	@echo "$(GREEN)$(NAME) Cleaned and rebuilt $(X)"

# Debug build
debug: CXXFLAGS += -ggdb3 $(SANITIZERS)
debug: re

# Rebuild with debug
rebug:
	@$(MAKE) clean
	@$(MAKE) debug
