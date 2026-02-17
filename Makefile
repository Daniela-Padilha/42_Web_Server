NAME			:= webserver.out
#NAME-BONUS		:= $(NAME)_bonus

FORMATABLE		= $(HEADERS) $(SRCS)

TEST_RUN		= $(NAME)

########################################################## Objects and Headers #
HEADERS			:=															\
	inc/webserver.hpp														\
	\
	inc/HTTPRequest.hpp														\
	inc/HTTPResponse.hpp													\
	inc/HTTPHandler.hpp														\
	inc/utils_print.hpp														\
	inc/tests.hpp															\
	inc/Client.hpp															\
	inc/Server.hpp															\
	inc/signals.hpp															\

SRCS			:=															\
	src/main.cpp															\
	\
	src/HTTPRequest.cpp														\
	src/HTTPResponse.cpp													\
	src/HTTPHandler.cpp														\
	src/test_http_parser.cpp												\
	src/Client.cpp															\
	src/Server.cpp															\
	src/signals.cpp															\


OBJS 			:= $(SRCS:/%.cpp=$(BUILD_DIR)/%.o)
#SRCS-BONUS		:=
#
#OBJS-BONUS		:= $(SRCS-BONUS:/%.cpp=$(BUILD_DIR)/%.o)
#INCLUDES		:= -I./inc
BUILD_DIR		:= build
##################################################################### Compiler #
CC				:= c++
CFLAGS			+= -std=c++98
CFLAGS			+= -Wall -Wextra
CFLAGS			+= -Werror
CFLAGS			+= -Wshadow
#CFLAGS			+= -Wno-shadow
GPROF_FLAGS		+= -pg
DEBUG_FLAGS		+= -g
DEBUG_FLAGS		+= -DDEBUG
########################################################### Intermediate steps #
RM				:= rm -f
AR				:= ar rcs
###################################################################### Targets #
all: $(NAME)

$(NAME): $(OBJS)
	@\
	echo "$(GRAY)Compiled with:$(COR)	$(CC)"							; \
	echo "$(GRAY)Compile flags:$(COR)	$(CFLAGS)"						; \
	$(CC) $(OBJS) $(INCLUDES)   $(CFLAGS) -o $(NAME)					&&	\
	echo "$(GRAY)File compiled:$(COR)	./$(NAME)"
#	echo "$(GRAY)Linking flags:$(COR)	$(INCLUDES)"					; \

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@\
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@								&&	\
	echo "$(GRAY)File compiled:$(COR)	$<"

#bonus: $(NAME-BONUS)
#
#$(NAME-BONUS): $(OBJS-BONUS)
#	@\
#	echo "$(GRAY)Compiler:     $(COR)	$(CC)"							; \
#	echo "$(GRAY)Compile flags:$(COR)	$(CFLAGS)"						; \
#	$(CC) $(OBJS-BONUS) $(INCLUDES)   $(CFLAGS) -o $(NAME-BONUS)		&&	\
#	echo "$(GRAY)File compiled:$(COR)	./$(NAME-BONUS)"
#	echo "$(GRAY)Linking flags:$(COR)	$(INCLUDES)"					; \

clean:
	@\
	$(RM) -r $(BUILD_DIR)												; \
	rm -fr *.o *.gch 							 						; \
	echo "$(GRAY)Files cleaned.$(COR)"

fclean: clean
	@\
	$(RM) $(NAME) $(NAME-BONUS)											&&	\
	echo "$(GRAY)File fcleaned:$(COR)	./$(NAME)"

re: fclean all
	@echo "$(GRAY)redone$(COR)"

.PHONY: all clean fclean re clang-check line
####################################################################### Format #
.clang-format:
	@echo "\
	Language: Cpp\n\
	\n\
	AlignConsecutiveDeclarations:\n\
	  Enabled: true\n\
	  AcrossEmptyLines: true\n\
	  AcrossComments: true\n\
	  AlignCompound: true\n\
	  AlignFunctionPointers: false\n\
	  PadOperators: true\n\
	AlignConsecutiveMacros:\n\
	  Enabled: true\n\
	  AcrossEmptyLines: false\n\
	  AcrossComments: false\n\
	  AlignCompound: false\n\
	  PadOperators: true\n\
	AlignAfterOpenBracket: Align\n\
	AlignConsecutiveAssignments: true\n\
	AlignEscapedNewlinesLeft: true\n\
	AllowAllConstructorInitializersOnNextLine: false\n\
	AllowAllParametersOfDeclarationOnNextLine: false\n\
	AllowShortBlocksOnASingleLine: false\n\
	AllowShortIfStatementsOnASingleLine: false\n\
	AllowShortFunctionsOnASingleLine: None\n\
	AlwaysBreakAfterReturnType: None\n\
	AlwaysBreakBeforeMultilineStrings: false\n\
	BinPackArguments: false\n\
	BinPackParameters: false\n\
	BreakBeforeBraces: Allman\n\
	BreakBeforeBinaryOperators: All\n\
	BreakBeforeTernaryOperators: false\n\
	BreakConstructorInitializers: AfterColon\n\
	PackConstructorInitializers: CurrentLine\n\
	ColumnLimit: 80\n\
	ConstructorInitializerIndentWidth: 4\n\
	IndentPPDirectives: AfterHash\n\
	IndentWidth: 4\n\
	KeepEmptyLinesAtTheStartOfBlocks: false\n\
	MaxEmptyLinesToKeep: 1\n\
	PointerAlignment: Right\n\
	PenaltyBreakBeforeFirstCallParameter: 100\n\
	PenaltyBreakString: 100\n\
	PenaltyExcessCharacter: 1000000\n\
	PPIndentWidth: 1\n\
	RemoveBracesLLVM: false\n\
	SeparateDefinitionBlocks: Always\n\
	SpaceAfterCStyleCast: true\n\
	SpaceBeforeAssignmentOperators: true\n\
	SpaceBeforeParens: ControlStatements\n\
	SpaceInEmptyParentheses: false\n\
	SpacesInCStyleCastParentheses: false\n\
	SpacesInParentheses: false\n\
	SpacesInSquareBrackets: false\n\
	TabWidth: 4\n\
	UseTab: Always\n\
	" > .clang-format

format: .clang-format
	@\
	for file in $(FORMATABLE); do 											\
		if ! clang-format "$$file" | diff -q "$$file" - > /dev/null 2>&1; then \
			clang-format -i "$$file" 									&&	\
			echo "$(GRAY)File formated:$(COR)	$$file"; 					\
		fi; 																\
	done; 																	\
# 	rm -f .clang-format

#	  -cppcoreguidelines-avoid-magic-numbers
.clang-tidy:
	@\
	echo "\
	Checks: |\n\
	  readability-*,\n\
	  -readability-magic-numbers,\n\
	  bugprone-*,\n\
	  performance-*,\n\
	  clang-analyzer-*,\n\
	  -modernize-*,\n\
	  cppcoreguidelines-*,\n\
	  -cppcoreguidelines-avoid-magic-numbers,\n\
	  -cppcoreguidelines-pro-type-member-init,\n\
	  -cppcoreguidelines-avoid-const-or-ref-data-members\n\
	\n\
	CheckOptions:\n\
	  - key:   readability-identifier-naming.ClassCase\n\
	    value: CamelCase\n\
	  - key:   readability-identifier-naming.FunctionCase\n\
	    value: lower_case\n\
	  - key:   readability-identifier-naming.PrivateMemberSuffix\n\
	    value: '_'\n\
	  - key: readability-identifier-length.IgnoredParameterNames\n\
	    value: 'fd'\n\
	  - key: readability-identifier-length.IgnoredVariableNames\n\
	    value: 'fd'\n\
	\n\
	HeaderFilterRegex: '.*'\n\
	" > .clang-tidy

style: .clang-format .clang-tidy
	@\
	clang-format --verbose --dry-run $(FORMATABLE)						; \
	clang-tidy --quiet -extra-arg=-std=c++98 $(FORMATABLE)				\
	-- $(CFLAGS)														; \
	make check-guards --silent
# 	rm -f .clang-format .clang-tidy										; \

fix-style: .clang-tidy format
	@\
	clang-tidy --quiet --fix --use-color --extra-arg=-std=c++98 $(FORMATABLE) -- $(CFLAGS) -DHARL=0 ; \
	make check-guards --silent ;\
	make format --silent
# 	rm -f .clang-format .clang-tidy

check-guards:
	@fail=0; \
	for file in $$(find . -name "*.hpp"); do \
		filename=$$(basename $$file); \
		guard=$$(echo $$filename | tr 'a-z' 'A-Z' | sed 's/\./_/g'); \
		if ! grep -q "#ifndef $$guard" $$file\
			|| ! grep -q "#define $$guard" $$file; then \
			echo "$(YELLOW)$$file  is missing header guard:$(COR)\
			\n#ifndef $$guard\n#define $$guard\n\n#endif"; \
			fail=1; \
		fi; \
	done;

clang-check:
	@\
	clang-check --analyze $(FORMATABLE) -- $(CFLAGS)					; \
	rm -f *.plist
############################################################# ANSI Escape Code #
COR			:= \033[0m

PURPLE		:= \033[1;35m
GRAY		:= \033[1;90m
YELLOW		:= \033[1;93m
BLUE		:= \033[1;96m
GREEN		:= \033[32m
ORANGE		:= \033[33m

BG_GREEN	:= \033[1;30m\033[102m
BG_RED		:= \033[1;30m\033[101m
######################################################################### Time #
time: $(NAME)
	@\
	trap '' INT TERM														; \
	$(TIMED_RUN) ./$(TEST_RUN)												; \
	trap - INT TERM

define TIMED_RUN
time --quiet --format "==CRONO== Total time: %E "
endef
##################################################################### Valgrind #
valgrind: $(NAME)
	@\
	echo "$(GRAY)Executing arg:$(COR)	time valgrind ./$(TEST_RUN)"		; \
	trap '' INT TERM														; \
	$(TIMED_RUN) $(VALGRIND_CMD) ./$(TEST_RUN)								; \
	trap - INT TERM

VALGRIND_CMD = valgrind \
	--track-fds=yes \
	--show-error-list=yes \
	--leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--max-stackframe=4200000

######################################################################### Test #
test: CFLAGS += $(DEBUG_FLAGS)
test: check-guards clang-check fclean $(NAME) 
	@\
	echo "\
	$(COR)$(GRAY)========================================== $(NAME) START\
	$(COR)" && \
	\
	make valgrind --silent												; \
	\
	echo "\
	$(COR)$(GRAY)========================================== $(NAME) END\n\
	$(COR)RETURN VALUE: $$?"											; \
	make style --silent

upload-delete: $(NAME)
	@\
	echo "$(GRAY)Starting Integration Check...$(COR)"					; \
	mkdir -p uploads													; \
	./$(NAME)  & echo $$! > server.pid					; \
	sleep 4																; \
	echo "$(GRAY)Creating test file...$(COR)"							; \
	echo "This is a test file" > test_upload.txt						; \
	echo "$(GRAY)Uploading file...$(COR)"								; \
	if curl -s -o /dev/null -w "%{http_code}" -X POST -F \
		"file=@test_upload.txt" http://localhost:8080/ | grep -q "201"; then \
		echo "$(GREEN)Upload successful.$(COR)"							; \
	else \
		echo "$(ORANGE)Upload failed$(COR)"								; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	if [ -f uploads/test_upload.txt ]; then \
		echo "$(GREEN)File verified on server.$(COR)"					; \
	else \
		echo "$(ORANGE)File not found in uploads/$(COR)"				; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	echo "$(GRAY)Waiting a moment...$(COR)"								; \
	sleep 7																; \
	echo "$(GRAY)Deleting file...$(COR)"								; \
	if curl --path-as-is -s -o /dev/null -w "%{http_code}" -X DELETE \
		http://localhost:8080/../uploads/test_upload.txt | grep -q "200"; then \
		echo "$(GREEN)Delete successful.$(COR)"							; \
	else \
		echo "$(ORANGE)Delete failed$(COR)"								; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	if [ ! -f uploads/test_upload.txt ]; then \
		echo "$(GREEN)File deletion verified.$(COR)"					; \
	else \
		echo "$(ORANGE)File still exists in uploads/$(COR)"				; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	echo "$(GREEN)Integration Check Passed!$(COR)"						; \
	kill $$(cat server.pid) && rm -f server.pid test_upload.txt

exe: format fclean $(NAME) 
	@\
	echo '$(GRAY)Executing arg:$(COR)	time ./$(TEST_RUN)'				; \
	trap '' INT TERM													; \
	$(TIMED_RUN) ./$(TEST_RUN)											; \
	trap - INT TERM

run: $(NAME)
	@\
	echo '$(GRAY)Executing arg:$(COR)	./$(TEST_RUN)'					; \
	trap '' INT TERM													; \
	./$(TEST_RUN)														; \
	trap - INT TERM

debug: CFLAGS += $(DEBUG_FLAGS)
debug: fclean format $(NAME) 
	@\
	echo '$(GRAY)Executing arg:$(COR)	time ./$(TEST_RUN)'				; \
	trap '' INT TERM													; \
	$(TIMED_RUN) ./$(TEST_RUN) 											; \
	trap - INT TERM

gprof: CFLAGS += $(GPROF_FLAGS)
gprof: fclean $(NAME)
	@\
	echo '$(GRAY)Executing arg:$(COR)	gprof ./$(TEST_RUN)'			; \
	trap '' INT TERM													; \
	./$(TEST_RUN)														; \
	trap - INT TERM
	gprof $(NAME) gmon.out > gmon-ignoreme.txt							; \
	cat gmon-ignoreme.txt												; \
	rm -f gmon-ignoreme.txt gmon.out

