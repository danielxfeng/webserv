# Color codes
X := \033[0;39m
GREEN := \033[0;92m
DARK_RED := \033[0;31m
DARK_YELLOW := \033[0;33m
DARK_CYAN := \033[0;36m
BLUE := \033[0;94m

# Project variables
RM := rm -rf
NAME := webserv
CXX := g++
CXXFLAGS := -Wall -Wextra -Werror -std=c++20 $(DEPFLAGS) -fPIE
LDFLAGS :=
DEPFLAGS := -MMD -MP
SANITIZERS := -fsanitize=address -fsanitize=leak -fsanitize=undefined

SRCS_DIR := srcs/
OBJS_DIR := objs/
INCL_DIR := includes/

# TODO: remove all the wildcards
SRCS := $(wildcard $(SRCS_DIR)*.cpp)
OBJS := $(patsubst $(SRCS_DIR)%.cpp,$(OBJS_DIR)%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

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
	@echo "$(DARK_CYAN)Compiling $<... $(X)"
	@$(CXX) $(CXXFLAGS) $(INCL) -c $< -o $@
	@echo "$(BLUE)Compiled: $@$(X)"

-include $(DEPS)

# Clean object and dep files
clean:
	@$(RM) $(OBJS_DIR)
	@echo "$(DARK_RED)Objects cleaned $(X)"

# Full clean
fclean: clean
	@$(RM) $(NAME)
	@echo "$(DARK_RED)$(NAME) removed $(X)"

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
