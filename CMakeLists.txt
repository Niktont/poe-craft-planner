cmake_minimum_required(VERSION 3.22)

project(CraftPlanner VERSION 0.5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PLANNER_NO_BROWSER "Build without browser" OFF)

if (PLANNER_NO_BROWSER)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Sql)
endif()

find_package(Boost REQUIRED)

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

qt_standard_project_setup()

add_subdirectory(DataUpdater EXCLUDE_FROM_ALL)

set(KEYBOARD_AUTO_TYPE_NO_EXCEPTIONS TRUE)
add_subdirectory(libs/keyboard-auto-type)

option(QHOTKEY_INSTALL "Enable install rule" OFF)
add_subdirectory(libs/QHotkey)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Plan.h src/Plan.cpp
    src/Step.h src/Step.cpp
    src/PlanModel.h src/PlanModel.cpp
    src/PlanItem.h src/PlanItem.cpp
    src/StepWidget.h src/StepWidget.cpp
    src/PlanWidget.h src/PlanWidget.cpp
    src/ExchangeItemData.h
    src/TradeItemData.h src/TradeItemData.cpp
    src/StepItem.h src/StepItem.cpp
    src/StepItemModel.h src/StepItemModel.cpp
    src/CustomItemData.h
    src/StepItemView.h src/StepItemView.cpp
    src/Settings.h src/Settings.cpp
    src/TradeRequestManager.h src/TradeRequestManager.cpp
    src/TradeRequestKey.h src/TradeRequestKey.cpp
    src/TradeRequestCache.h src/TradeRequestCache.cpp
    src/TradeRequestData.h src/TradeRequestData.cpp
    src/ExchangeRequestCache.h src/ExchangeRequestCache.cpp
    src/ExchangeData.h src/ExchangeData.cpp
    src/CurrencyCost.h
    src/Game.h
    src/Database.h src/Database.cpp
    src/InitializationDialog.h src/InitializationDialog.cpp
    src/ExchangeRequestManager.h src/ExchangeRequestManager.cpp
    src/ExchangeCostData.h src/ExchangeCostData.cpp
    src/CurrencyCost.cpp
    src/HashFunctions.h
    src/StepItemDelegate.h src/StepItemDelegate.cpp
    src/TradeCostData.h src/TradeCostData.cpp
    src/RequestEditDialog.h src/RequestEditDialog.cpp
    src/PlanTreeView.h src/PlanTreeView.cpp
    src/Game.cpp
    src/ItemTime.h
    src/UpdateCostDialog.h src/UpdateCostDialog.cpp
    src/CostWidget.h src/CostWidget.cpp
    src/ItemCost.h src/ItemCost.cpp
    src/StepItemData.h
    src/StepItemsWidget.h src/StepItemsWidget.cpp
    src/ImportException.h
    src/ImportOverwriteModel.h src/ImportOverwriteModel.cpp
    src/ImportOverwriteDialog.h src/ImportOverwriteDialog.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    src/DescriptionEdit.h src/DescriptionEdit.cpp
    src/ShoppingDialog.h src/ShoppingDialog.cpp
    src/ShoppingModel.h src/ShoppingModel.cpp
    src/ShoppingItem.h
    src/ShoppingView.h src/ShoppingView.cpp
    src/HotkeyEdit.h src/HotkeyEdit.cpp
    src/ShoppingSetupDialog.h src/ShoppingSetupDialog.cpp
    src/TradeRequestView.h src/TradeRequestView.cpp
    src/SearchesDialog.h src/SearchesDialog.cpp
    src/PlanItemData.h
    src/PlanSearchModel.h src/PlanSearchModel.cpp
    src/PlanSearchDialog.h src/PlanSearchDialog.cpp
    src/PlanSearchView.h src/PlanSearchView.cpp
    src/CustomCalculation.h src/CustomCalculation.cpp
    src/CustomExpression.h
    src/CustomParser.h src/CustomParser.cpp
    src/CustomResult.h src/CustomResult.cpp
    src/CustomVisitor.h src/CustomVisitor.cpp
    src/CustomEditDialog.h src/CustomEditDialog.cpp
    src/SettingsKey.h
    src/SnapshotModel.h src/SnapshotModel.cpp
    src/Snapshot.h
    src/SnapshotsDialog.h src/SnapshotsDialog.cpp
    src/SnapshotView.h src/SnapshotView.cpp
    src/QueryParser.h src/QueryParser.cpp
    src/StepCopyState.h src/StepCopyState.cpp
    src/StepItemCopyState.h src/StepItemCopyState.cpp
    src/AppState.h src/AppState.cpp
    src/PlanTitleWidget.h src/PlanTitleWidget.cpp
    src/MainWidget.h src/MainWidget.cpp
    src/ExchangeRequester.h src/ExchangeRequester.cpp
    src/TradeRequester.h src/TradeRequester.cpp
    src/ArithmeticExpression.h
    src/ArithmeticVisitor.h src/ArithmeticVisitor.cpp
    src/ArithmeticParser.h src/ArithmeticParser.cpp
    src/ArithmeticCalculation.h src/ArithmeticCalculation.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /wd4267)
else()
    # target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(APP_NAME "PoE Craft Planner")
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}" APP_NAME="${APP_NAME}")

if (PLANNER_NO_BROWSER)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql Boost::headers keyboard-auto-type qhotkey
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLANNER_NO_BROWSER)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/install/PoE-Craft-Planner-no-browser)
else()
    target_link_libraries(${PROJECT_NAME}
        PRIVATE Qt6::Widgets Qt6::WebEngineWidgets Qt6::Sql Boost::headers keyboard-auto-type qhotkey
    )
    target_sources(${PROJECT_NAME} PRIVATE
        src/WebPage.h src/WebPage.cpp
        src/WebViewDialog.h src/WebViewDialog.cpp
    )
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/install/PoE-Craft-Planner)
endif()

# if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
#     set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/install)
# endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/additional.sqlite3
            ${CMAKE_CURRENT_BINARY_DIR}/additional.sqlite3
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    OUTPUT_NAME ${APP_NAME}
)

if(WIN32)
    set_property(
        TARGET ${PROJECT_NAME}
        APPEND PROPERTY
            SOURCES $<TARGET_PROPERTY:${PROJECT_NAME},OUTPUT_NAME>.exe.manifest
    )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION "."
    RUNTIME DESTINATION "."
)

install(FILES additional.sqlite3 LICENSE
    DESTINATION "."
)
install(FILES libs/keyboard-auto-type/LICENSE.md
    DESTINATION "."
    RENAME LICENSE-keyboard-auto-type
)
install(FILES libs/QHotkey/LICENSE
    DESTINATION "."
    RENAME LICENSE-qhotkey
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS
        --libdir "."
        --no-compiler-runtime
        --no-system-d3d-compiler
        --no-system-dxc-compiler
        --no-opengl-sw
)

install(SCRIPT ${deploy_script})
