BUILD_DIR := build

.PHONY: all run clean rebuild

all:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -j

run: all
	./$(BUILD_DIR)/minishell

clean:
	rm -rf $(BUILD_DIR) install

rebuild: clean all