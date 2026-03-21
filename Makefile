NAME			:= webserver
#NAME-BONUS		:= $(NAME)_bonus

FORMATABLE		= $(HEADERS) $(SRCS)

TEST_RUN		= $(NAME)

########################################################## Objects and Headers #
HEADERS			:=															\
	inc/webserver.hpp														\
	\
	inc/Config.hpp															\
	inc/HTTPRequest.hpp														\
	inc/HTTPResponse.hpp													\
	inc/HTTPHandler.hpp														\
	inc/utils_print.hpp														\
	inc/tests.hpp															\
	inc/Client.hpp															\
	inc/Server.hpp															\
	inc/signals.hpp															\
	inc/init.hpp															\
	inc/CGI.hpp																\

SRCS			:=															\
	src/main.cpp															\
	\
	src/Config.cpp															\
	src/init.cpp															\
	src/HTTPRequest.cpp														\
	src/HTTPResponse.cpp													\
	src/HTTPHandler.cpp														\
	src/test_http_parser.cpp												\
	src/test_http_response.cpp												\
	src/test_http_features.cpp												\
	src/Client.cpp															\
	src/Server.cpp															\
	src/signals.cpp															\
	src/CGI.cpp																\


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

.PHONY: all clean fclean re clang-check line cgi-test
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
	echo "$(GRAY)Executing arg:$(COR)   time valgrind ./$(TEST_RUN)"		; \
	trap '' INT TERM														; \
	$(TIMED_RUN) $(VALGRIND_CMD) ./$(TEST_RUN)								; \
	RET=$$?																	; \
	trap - INT TERM															; \
	exit $$RET

VALGRIND_CMD = valgrind \
	--track-fds=yes \
	--show-error-list=yes \
	--leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--max-stackframe=4200000

######################################################################### Test #
TEST_FLAGS		:= -DTESTING

test: CFLAGS += $(TEST_FLAGS)
test: check-guards fclean $(NAME)
	@\
	echo "$(COR)$(GRAY)=========================================="\
	" $(NAME) UNIT TESTS$(COR)"											; \
	trap '' INT TERM													; \
	$(TIMED_RUN) $(VALGRIND_CMD) ./$(TEST_RUN)							; \
	RET1=$$?															; \
	trap - INT TERM														; \
	echo "$(COR)$(GRAY)Unit tests return value:$(COR) $$RET1"			; \
	if [ $$RET1 -ne 0 ]; then exit $$RET1; fi							; \
	echo "$(COR)$(GRAY)=========================================="\
	" $(NAME) UPLOAD-DELETE TEST$(COR)"									; \
	make re --silent													; \
	trap '' INT TERM													; \
	make upload-delete --silent											; \
	RET2=$$?															; \
	trap - INT TERM														; \
	echo "$(COR)$(GRAY)=========================================="\
	" $(NAME) TESTER$(COR)"												; \
	chmod +x cgi_tester 2>/dev/null || true								; \
	./$(NAME) tester.conf & echo $$! > server.pid						; \
	sleep 2																; \
	trap '' INT TERM													; \
	yes "" | ./tester http://localhost:8080								; \
	RET3=$$?															; \
	trap - INT TERM														; \
	kill $$(cat server.pid) 2>/dev/null									; \
	rm -f server.pid													; \
	echo "$(COR)$(GRAY)=========================================="\
	" $(NAME) TESTS END$(COR)"											; \
	echo "$(COR)$(GREEN)       unit tests return value:$(COR) $$RET1"	; \
	echo "$(COR)$(GREEN)up-download tests return value:$(COR) $$RET2"	; \
	echo "$(COR)$(GREEN)   provided tests return value:$(COR) $$RET3"

upload-delete: $(NAME)
	@\
	echo "==TESTS== $(GRAY)Starting upload and delete test...$(COR)"	; \
	pkill -x $(NAME) || true											; \
	pkill -x $(NAME).out || true										; \
	sleep 1																; \
	rm -f server.pid													; \
	mkdir -p uploads													; \
	./$(NAME)  & echo $$! > server.pid					; \
	sleep 2																; \
	echo "==TESTS== $(GRAY)Creating test file...$(COR)"					; \
	echo "This is a test file" > test_upload.txt						; \
	echo "==TESTS== $(GRAY)Uploading file...$(COR)"						; \
	if curl -s -o /dev/null -w "%{http_code}" -X POST -F \
		"file=@test_upload.txt" http://localhost:8080/ | grep -q "201"; then \
		echo "==TESTS== $(GREEN)Upload successful.$(COR)"				; \
	else \
		echo "==TESTS== $(ORANGE)Upload failed$(COR)"					; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	if [ -f uploads/test_upload.txt ]; then \
		echo "==TESTS== $(GREEN)File verified on server.$(COR)"			; \
	else \
		echo "==TESTS== $(ORANGE)File not found in uploads/$(COR)"		; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	echo "==TESTS== $(GRAY)Waiting a moment...$(COR)"					; \
	sleep 7																; \
	echo "==TESTS== $(GRAY)Deleting file...$(COR)"						; \
	if curl -s -o /dev/null -w "%{http_code}" -X DELETE \
	http://localhost:8080/uploads/test_upload.txt | grep -q "200"; then \
		echo "==TESTS== $(GREEN)Delete successful.$(COR)"				; \
	else \
		echo "==TESTS== $(ORANGE)Delete failed$(COR)"					; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	if [ ! -f uploads/test_upload.txt ]; then \
		echo "==TESTS== $(GREEN)File deletion verified.$(COR)"			; \
	else \
		echo "==TESTS== $(ORANGE)File still exists in uploads/$(COR)"	; \
		kill $$(cat server.pid) && rm -f server.pid test_upload.txt		; \
		exit 1															; \
	fi																	; \
	echo "==TESTS== $(GREEN)Integration Check Passed!$(COR)"			; \
	kill $$(cat server.pid) && rm -f server.pid test_upload.txt

cgi-test: $(NAME)
	@\
	echo "$(COR)$(GRAY)=========================================="\
	" $(NAME) CGI TESTS$(COR)"											; \
	pkill -x $(NAME) || true											; \
	sleep 1																; \
	rm -f server.pid													; \
	chmod +x files/cgi/*.py												; \
	./$(NAME) & echo $$! > server.pid									; \
	sleep 2																; \
	FAIL=0																; \
	echo -n "Test 1:  GET basic CGI ... "								; \
	RESP=$$(curl -s http://localhost:8080/files/cgi/hello.py)				; \
	if echo "$$RESP" | grep -q "Hello from CGI"; then						\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 2:  GET with query string ... "						; \
	RESP=$$(curl -s "http://localhost:8080/files/cgi/hello.py?foo=bar&baz=42"); \
	if echo "$$RESP" | grep -q "foo=bar"; then								\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 3:  POST with body ... "								; \
	RESP=$$(curl -s -X POST -d "test_body=hello"						\
		http://localhost:8080/files/cgi/hello.py)							; \
	if echo "$$RESP" | grep -q "test_body=hello"; then						\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 4:  POST Content-Type forwarded ... "					; \
	RESP=$$(curl -s -X POST											\
		-H "Content-Type: application/x-www-form-urlencoded"			\
		-d "key=value" http://localhost:8080/files/cgi/hello.py)			; \
	if echo "$$RESP" | grep -q "key=value"; then							\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 5:  CGI custom headers ... "							; \
	RESP=$$(curl -s -D - http://localhost:8080/files/cgi/headers.py)		; \
	if echo "$$RESP" | grep -qi "X-Custom: test123"; then					\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 6:  CGI custom status code ... "						; \
	CODE=$$(curl -s -o /dev/null -w "%{http_code}"						\
		http://localhost:8080/files/cgi/status.py)							; \
	if [ "$$CODE" = "404" ]; then											\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR) (got $$CODE)"							; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 7:  CGI large output (~100KB) ... "					; \
	RESP=$$(curl -s http://localhost:8080/files/cgi/large_output.py)		; \
	if echo "$$RESP" | grep -q "CGI_LARGE_OUTPUT_END"; then					\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 8:  CGI timeout (server stays responsive) ... "		; \
	curl -s -o /dev/null --max-time 3									\
		http://localhost:8080/files/cgi/timeout.py &						\
	TIMEOUT_PID=$$!														; \
	sleep 1																; \
	RESP=$$(curl -s --max-time 5 http://localhost:8080/files/cgi/hello.py); \
	if echo "$$RESP" | grep -q "Hello from CGI"; then						\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	wait $$TIMEOUT_PID 2>/dev/null										; \
	echo -n "Test 9:  Non-existent CGI script ... "						; \
	CODE=$$(curl -s -o /dev/null -w "%{http_code}"						\
		http://localhost:8080/files/cgi/nonexistent.py)					; \
	if [ "$$CODE" = "404" ] || [ "$$CODE" = "500" ]; then					\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR) (got $$CODE)"							; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo -n "Test 10: CGI PATH_INFO and cwd ... "						; \
	RESP=$$(curl -s http://localhost:8080/files/cgi/pathinfo.py)			; \
	if echo "$$RESP" | grep -q "PATH_INFO="								\
		&& echo "$$RESP" | grep -q "CWD="; then							\
		echo "$(GREEN)PASS$(COR)"										; \
	else																	\
		echo "$(ORANGE)FAIL$(COR)"										; \
		FAIL=$$((FAIL + 1))												; \
	fi																	; \
	echo "$(GRAY)------------------------------------------$(COR)"		; \
	kill $$(cat server.pid) 2>/dev/null									; \
	rm -f server.pid													; \
	if [ $$FAIL -ne 0 ]; then												\
		echo "$(ORANGE)==CGI-TEST== $$FAIL test(s) FAILED$(COR)"		; \
		exit 1															; \
	fi																	; \
	echo "$(GREEN)==CGI-TEST== All 10 tests passed!$(COR)"

######################################################################## Siege #
siege: $(NAME)
	@\
	echo "$(GRAY)==SIEGE== Starting stress test...$(COR)"				; \
	pkill -x $(NAME) || true ; \
	sleep 1 ; \
	rm -f server.pid ; \
	./$(NAME) & echo $$! > server.pid ; \
	sleep 2 ; \
	trap '' INT TERM ; \
	siege -b -t 30S -c 50 http://127.0.0.1:8080/ ; \
	RET=$$? ; \
	trap - INT TERM ; \
	kill $$(cat server.pid) 2>/dev/null ; \
	rm -f server.pid ; \
	exit $$RET

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

