# === Colors (use printf) ===
X          := \033[0;39m
GREEN      := \033[0;92m
DARK_RED   := \033[0;31m
DARK_YELLOW:= \033[0;33m
DARK_CYAN  := \033[0;36m
BLUE       := \033[0;94m

# === Project variables ===
NAME      := webserv
CXX       := g++
RM        := rm -rf

SRCS_FILES := /Buffer.cpp /CGIHandler.cpp /Config.cpp /Cookie.cpp /EpollHelper.cpp /ErrorResponse.cpp \
			  /HttpRequests.cpp /HttpResponse.cpp /main.cpp /MethodHandler.cpp /RaiiFd.cpp \
			  /RedirectHandler.cpp /Server.cpp /SharedTypes.cpp /signalHandler.cpp /TinyJson.cpp \
			  /urlHelper.cpp /utils.cpp /WebServ.cpp /WebServErr.cpp

SRCS_DIR  := srcs
OBJS_DIR  := objs
INCL_DIR  := includes

# Sources (supports nested folders; change to a static list if you prefer)
SRCS	  := $(addprefix $(SRCS_DIR), $(SRCS_FILES))
OBJS 	  := $(addprefix $(OBJS_DIR), $(SRCS_FILES:.cpp=.o))
DEPS      := $(OBJS:.o=.d)

# Flags
DEPFLAGS  := -MMD -MP
CXXWARN   := -Wall -Wextra -Werror
STD       := -std=c++20
INCL      := -I$(INCL_DIR)

# Sanitizers (add to both compile and link)
SAN       := -fsanitize=address -fsanitize=leak -fsanitize=undefined

CXXFLAGS  := $(CXXWARN) $(STD) -fPIE
LDFLAGS   :=
LDLIBS    :=

# Phony targets
.PHONY: all clean fclean re debug rebug

# Default
all: $(NAME)
	@printf "$(GREEN)Run the program with ./$(NAME) $(X)\n"

# Link
$(NAME): $(OBJS)
	@$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	@printf "$(DARK_YELLOW)%s compiled %s\n" "$@" "$(X)"

# Ensure objs dir exists (order-only prerequisite)
$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJS_DIR)
	@printf "$(DARK_CYAN)Compiling %s... %s\n" "$<" "$(X)"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCL) -c $< -o $@
	@printf "$(BLUE)Compiled: %s%s\n" "$@" "$(X)"

$(OBJS_DIR):
	@mkdir -p $@

-include $(DEPS)

# Clean
clean:
	@$(RM) $(OBJS_DIR)
	@printf "$(DARK_RED)Objects cleaned %s\n" "$(X)"

fclean: clean
	@$(RM) $(NAME)
	@printf "$(DARK_RED)%s removed %s\n" "$(NAME)" "$(X)"

re: fclean all
	@printf "$(GREEN)%s Cleaned and rebuilt %s\n" "$(NAME)" "$(X)"

# Debug build (adds sanitizers to both compile and link)
debug: CXXFLAGS += -ggdb3 $(SAN)
debug: LDFLAGS  += $(SAN)
debug: re

# Rebuild with debug
rebug: clean debug
