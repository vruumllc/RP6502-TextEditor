cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(MY-RP6502-PROJECT)
add_executable(te)
rp6502_executable(te)
target_sources(te PRIVATE
src/ezpsg.c
src/doc.c
src/display.c
src/textbox.c
src/statusbar.c
src/file_ops.c
src/button.c
src/panel.c
src/msg_dlg.c
src/file_dlg.c
src/actions.c
src/menu.c
src/keyboard.c
src/mouse.c
src/main.c
)
