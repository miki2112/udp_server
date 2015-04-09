
# Build the source, documentation, and tests.
all: deliver server

# Build the source.
deliver:
	cd deliver && make clean depend build

# Generate the documentation.
server:
	cd server && make clean build

# Build the tests

# Delete generated files.
clean:
	cd deliver && make -s clean
	cd server && make -s clean

.PHONY: all clean deliver server
