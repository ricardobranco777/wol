BIN=wol
FILES=$(BIN)

.PHONY: all
all: pylint mypy black

.PHONY: pylint
pylint:
	@pylint $(FILES)

.PHONY: mypy
mypy:
	@mypy $(FILES)

.PHONY: black
black:
	@black --check $(FILES)

.PHONY: install
install:
	install -m 0755 $(BIN) /usr/local/bin/ 2>/dev/null || install -m 0755 $(BIN) $(HOME)/bin/

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(BIN) 2>/dev/null || rm -f $(HOME)/bin/$(BIN)
