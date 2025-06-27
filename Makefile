# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gpellech <gpellech@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/05/20 06:27:04 by gpellech          #+#    #+#              #
#    Updated: 2025/06/19 07:29:02 by gpellech         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #
X = \033[0;39m
BLACK = \033[0;30m
DARK_RED = \033[0;31m
DARK_GREEN = \033[0;32m
DARK_YELLOW = \033[0;33m
DARK_BLUE = \033[0;34m
DARK_MAGENTA = \033[0;35m
DARK_CYAN = \033[0;36m
DARK_GRAY = \033[0;90m
LIGHT_GRAY = \033[0;37m
RED = \033[0;91m
GREEN = \033[0;92m
YELLOW = \033[0;93m
BLUE = \033[0;94m
MAGENTA = \033[0;95m
CYAN = \033[0;96m
WHITE = \033[0;97m

NAME := RPN
CC := c++
RM := rm -rf
FLAGS := -Wall -Wextra -Werror -std=c++17

SRCS_FILES = RPN.cpp main.cpp

OBJS_FILES = $(SRCS_FILES:.cpp=.o)

SRCS_DIR := srcs/
OBJS_DIR := objs/
INCL_DIR := includes/

INCL := -I $(INCL_DIR)

SRCS = $(addprefix $(SRCS_DIR), $(SRCS_FILES))
OBJS = $(addprefix $(OBJS_DIR), $(OBJS_FILES))


all: $(NAME)
	@echo "$(GREEN)Run the program with ./$(NAME)"

$(NAME): $(OBJS)
	@$(CC) $(FLAGS) $(OBJS) -o $(NAME)
	@echo "$(DARK_YELLOW) $(NAME) compiled"

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp
	@echo "$(DARK_CYAN) Creating objects folder"
	@mkdir -p $(OBJS_DIR)
	@$(CC) $(FLAGS) $(INCL) -o $@ -c $<
	echo "$(DARK BLUE) Compiled: $@"

clean:
	@if [ -d "$(OBJS_DIR)" ]; then \
	rm -rf objs/; \
	echo "$(DARK_RED) Objects cleaned"; else \
	echo "No object files to clean"; \
	fi;

fclean: clean
	@if [ -f "$(NAME)" ]; then \
	rm -f $(NAME); \
	echo "$(DARK_RED) $(NAME) cleaned"; else \
	echo "No executables to clean"; \
	fi;

re: fclean all
	@echo "$(GREEN) $(NAME) Cleaned and rebuilt"

.PHONY: all clean fclean re
