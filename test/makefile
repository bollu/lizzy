targets = parse-fn

all: $(targets)

$(targets): %: %.rs
	make -C ../build/
	../build/rascal $<




