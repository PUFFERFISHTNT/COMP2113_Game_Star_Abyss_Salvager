CXX ?= g++
OBJDIR := obj
TARGET := star_abyss_salvager

APP_SOURCES := \
	src/tui_main.cpp \
	src/tui_app.cpp \
	src/tui_game.cpp \
	src/engine/game_battle.cpp \
	src/engine/game_card_services.cpp \
	src/engine/game_card_json.cpp \
	src/engine/game_content.cpp \
	src/engine/game_encounter.cpp \
	src/engine/game_info.cpp \
	src/engine/game_reward.cpp \
	src/engine/game_route.cpp \
	src/engine/game_save.cpp \
	src/ui/ui_art.cpp \
	src/ui/ui_input.cpp \
	src/ui/ui_localization.cpp \
	src/ui/ui_page_battle.cpp \
	src/ui/ui_page_encounter.cpp \
	src/ui/ui_page_info.cpp \
	src/ui/ui_page_main_menu.cpp \
	src/ui/ui_page_map.cpp \
	src/ui/ui_page_reward.cpp \
	src/ui/ui_page_setup.cpp \
	src/ui/ui_page_summary.cpp \
	src/ui/ui_panels.cpp \
	src/ui/ui_save_dialog.cpp \
	src/ui/ui_sync.cpp

FTXUI_SCREEN_SOURCES := \
	vendor/FTXUI/src/ftxui/screen/box.cpp \
	vendor/FTXUI/src/ftxui/screen/color.cpp \
	vendor/FTXUI/src/ftxui/screen/color_info.cpp \
	vendor/FTXUI/src/ftxui/screen/surface.cpp \
	vendor/FTXUI/src/ftxui/screen/screen.cpp \
	vendor/FTXUI/src/ftxui/screen/string.cpp \
	vendor/FTXUI/src/ftxui/screen/terminal.cpp

FTXUI_DOM_SOURCES := \
	vendor/FTXUI/src/ftxui/dom/automerge.cpp \
	vendor/FTXUI/src/ftxui/dom/selection_style.cpp \
	vendor/FTXUI/src/ftxui/dom/blink.cpp \
	vendor/FTXUI/src/ftxui/dom/bold.cpp \
	vendor/FTXUI/src/ftxui/dom/border.cpp \
	vendor/FTXUI/src/ftxui/dom/box_helper.cpp \
	vendor/FTXUI/src/ftxui/dom/canvas.cpp \
	vendor/FTXUI/src/ftxui/dom/clear_under.cpp \
	vendor/FTXUI/src/ftxui/dom/color.cpp \
	vendor/FTXUI/src/ftxui/dom/composite_decorator.cpp \
	vendor/FTXUI/src/ftxui/dom/dbox.cpp \
	vendor/FTXUI/src/ftxui/dom/dim.cpp \
	vendor/FTXUI/src/ftxui/dom/flex.cpp \
	vendor/FTXUI/src/ftxui/dom/flexbox.cpp \
	vendor/FTXUI/src/ftxui/dom/flexbox_config.cpp \
	vendor/FTXUI/src/ftxui/dom/flexbox_helper.cpp \
	vendor/FTXUI/src/ftxui/dom/focus.cpp \
	vendor/FTXUI/src/ftxui/dom/frame.cpp \
	vendor/FTXUI/src/ftxui/dom/gauge.cpp \
	vendor/FTXUI/src/ftxui/dom/graph.cpp \
	vendor/FTXUI/src/ftxui/dom/gridbox.cpp \
	vendor/FTXUI/src/ftxui/dom/hbox.cpp \
	vendor/FTXUI/src/ftxui/dom/hyperlink.cpp \
	vendor/FTXUI/src/ftxui/dom/inverted.cpp \
	vendor/FTXUI/src/ftxui/dom/italic.cpp \
	vendor/FTXUI/src/ftxui/dom/linear_gradient.cpp \
	vendor/FTXUI/src/ftxui/dom/node.cpp \
	vendor/FTXUI/src/ftxui/dom/node_decorator.cpp \
	vendor/FTXUI/src/ftxui/dom/paragraph.cpp \
	vendor/FTXUI/src/ftxui/dom/reflect.cpp \
	vendor/FTXUI/src/ftxui/dom/scroll_indicator.cpp \
	vendor/FTXUI/src/ftxui/dom/selection.cpp \
	vendor/FTXUI/src/ftxui/dom/separator.cpp \
	vendor/FTXUI/src/ftxui/dom/size.cpp \
	vendor/FTXUI/src/ftxui/dom/spinner.cpp \
	vendor/FTXUI/src/ftxui/dom/strikethrough.cpp \
	vendor/FTXUI/src/ftxui/dom/table.cpp \
	vendor/FTXUI/src/ftxui/dom/text.cpp \
	vendor/FTXUI/src/ftxui/dom/underlined.cpp \
	vendor/FTXUI/src/ftxui/dom/underlined_double.cpp \
	vendor/FTXUI/src/ftxui/dom/util.cpp \
	vendor/FTXUI/src/ftxui/dom/vbox.cpp

FTXUI_COMPONENT_SOURCES := \
	vendor/FTXUI/src/ftxui/component/animation.cpp \
	vendor/FTXUI/src/ftxui/component/app.cpp \
	vendor/FTXUI/src/ftxui/component/button.cpp \
	vendor/FTXUI/src/ftxui/component/catch_event.cpp \
	vendor/FTXUI/src/ftxui/component/checkbox.cpp \
	vendor/FTXUI/src/ftxui/component/collapsible.cpp \
	vendor/FTXUI/src/ftxui/component/component.cpp \
	vendor/FTXUI/src/ftxui/component/component_options.cpp \
	vendor/FTXUI/src/ftxui/component/container.cpp \
	vendor/FTXUI/src/ftxui/component/dropdown.cpp \
	vendor/FTXUI/src/ftxui/component/event.cpp \
	vendor/FTXUI/src/ftxui/component/hoverable.cpp \
	vendor/FTXUI/src/ftxui/component/input.cpp \
	vendor/FTXUI/src/ftxui/component/loop.cpp \
	vendor/FTXUI/src/ftxui/component/maybe.cpp \
	vendor/FTXUI/src/ftxui/component/menu.cpp \
	vendor/FTXUI/src/ftxui/component/modal.cpp \
	vendor/FTXUI/src/ftxui/component/radiobox.cpp \
	vendor/FTXUI/src/ftxui/component/renderer.cpp \
	vendor/FTXUI/src/ftxui/component/resizable_split.cpp \
	vendor/FTXUI/src/ftxui/component/slider.cpp \
	vendor/FTXUI/src/ftxui/component/task.cpp \
	vendor/FTXUI/src/ftxui/component/task_queue.cpp \
	vendor/FTXUI/src/ftxui/component/task_runner.cpp \
	vendor/FTXUI/src/ftxui/component/terminal_input_parser.cpp \
	vendor/FTXUI/src/ftxui/component/util.cpp \
	vendor/FTXUI/src/ftxui/component/window.cpp

ALL_SOURCES := $(APP_SOURCES) $(FTXUI_SCREEN_SOURCES) $(FTXUI_DOM_SOURCES) $(FTXUI_COMPONENT_SOURCES)
OBJECTS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(ALL_SOURCES))

APP_CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 -Iinclude -isystem vendor/FTXUI/include
FTXUI_CXXFLAGS := -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -isystem vendor/FTXUI/include -Ivendor/FTXUI/src
LDFLAGS :=
LDLIBS := -pthread

ifneq (,$(findstring clang,$(shell $(CXX) --version 2>/dev/null)))
LDLIBS +=
else
LDLIBS += -lstdc++fs
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/src/%.o: src/%.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(APP_CXXFLAGS) -c $< -o $@

$(OBJDIR)/vendor/FTXUI/src/%.o: vendor/FTXUI/src/%.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(FTXUI_CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf "$(OBJDIR)" "$(TARGET)"
