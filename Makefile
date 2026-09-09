# **************************************************************************** #
#                                                                              #
#                                                        ::::::::   ::::::::   #
#    Makefile			                               :+:    :+: :+:    :+:   #
#                                                     +:+        +:+           #
#    By: Ser Superior <marcioeduine@gmail.com>       +#++:++#++ +#++:++#++     #
#                                                          +#+        +#+      #
#    Created: 2026/07/09 10:25:00 by Ser Superior  #+#    #+# #+#    #+#       #
#    Updated: 2026/07/15 21:20:00 by Ser Superior  ########   ########         #
#                                                                              #
# **************************************************************************** #

NAME     = SSCodeIDE
LAUNCHER = sscode

GO ?= $(shell command -v go 2>/dev/null || which go 2>/dev/null || if [ -f ~/go_toolchain/bin/go ]; then echo ~/go_toolchain/bin/go; else echo go; fi)

all: $(NAME)

$(NAME): src/*.go
	@$(GO) build -o $(NAME) ./src
	@chmod +x $(LAUNCHER)
	@echo "[SSCodeIDE] Built successfully using $(GO)."

clean:
	@rm -f $(NAME)

fclean: clean

re: fclean all

run: all
	./$(NAME)

.PHONY: all clean fclean re run
